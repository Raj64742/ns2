#include <stdlib.h>
#include <math.h>
#include "tclcl.h"
#include "packet.h"
#include "ip.h"
#include "tcp.h"
#include "flags.h"
#include "random.h"
#include "template.h"
#include "nilist.h"
#include "tcp-int.h"
#include "chost.h"
#include "tcp-session.h"

/*
 * We separate TCP functionality into two parts: that having to do with providing
 * a reliable, ordered byte-stream service, and that having to do with congestion
 * control and loss recovery. The former is done on a per-connection basis and is
 * implemented as part of IntTcpAgent ("integrated TCP"). The latter is done in an 
 * integrated fashion across multiple TCP connections, and is implemented as part 
 * of TcpSessionAgent ("TCP session"). TcpSessionAgent is derived from CorresHost 
 * ("correspondent host"), which keeps track of the state of all TCP (TCP/Int) 
 * connections to a host that it is corresponding with.
 *
 * The motivation for this separation of functionality is to make an ensemble of
 * connection more well-behaved than a set of independent TCP connections and improve
 * the chances of losses being recovered via data-driven techniques (rather than
 * via timeouts). At the same time, we do not introduce any unnecessary between the
 * logically-independent byte-streams that the set of connections represents. This is
 * in contrast to the coupling that is inherent in the multiplexing at the application
 * layer of multiple byte-streams onto a single TCP connection.
 */

static class TcpSessionClass : public TclClass {
public:
	TcpSessionClass() : TclClass("Agent/TCP/Session") {}
	TclObject* create(int, const char*const*) {
		return (new TcpSessionAgent()); 
	}
} class_tcpsession;

TcpSessionAgent::TcpSessionAgent() : CorresHost(), 
	rtx_timer_(this), burstsnd_timer_(this), sessionSeqno_(0),
	last_send_time_(-1)
{
	bind("proxyopt_", &proxyopt_);
	sessionList_.append(this);
}

void
SessionRtxTimer::expire(Event*)
{
	a_->timeout(TCP_TIMER_RTX);
}

void
SessionBurstSndTimer::expire(Event*)
{
	a_->timeout(TCP_TIMER_BURSTSND);
}

void
TcpSessionAgent::reset_rtx_timer(int mild, int backoff = 1)
{
	if (backoff)
		rtt_backoff();
	set_rtx_timer();
	rtt_active_ = 0;
}

void
TcpSessionAgent::set_rtx_timer()
{
	rtx_timer_.resched(rtt_timeout());
}

void
TcpSessionAgent::cancel_rtx_timer()
{
	rtx_timer_.force_cancel();
}

void
TcpSessionAgent::newack(Packet *pkt) 
{
	double now = Scheduler::instance().clock();
	Islist_iter<Segment> seg_iter(seglist_);
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	hdr_flags *fh = (hdr_flags *)pkt->access(off_flags_);

	if (!fh->no_ts_) {
		/* if the timestamp option is being used */
		if (ts_option_)
			rtt_update(now - tcph->ts_echo());
		/* if segment being timed just got acked */
		if (rtt_active_ && rtt_seg_ == NULL) {
			t_backoff_ = 1;
			rtt_active_ = 0;
			if (!ts_option_)
				rtt_update(now - tcph->ts_echo());
		}
	}
	if (seg_iter.count() > 0)
		set_rtx_timer();
	else
		cancel_rtx_timer();
}

void
TcpSessionAgent::timeout(int tno)
{
	if (tno == TCP_TIMER_BURSTSND)
		send_much(NULL,0,0);
	else if (tno == TCP_TIMER_RTX) {
		Islist_iter<Segment> seg_iter(seglist_);
		Segment *curseg;
		Islist_iter<IntTcpAgent> conn_iter(conns_);
		IntTcpAgent *curconn;

		if (seg_iter.count() == 0 && !slow_start_restart_) {
			return;
		}
		recover_ = sessionSeqno_ - 1;
		if (seg_iter.count() == 0 && restart_bugfix_) {
			closecwnd(3);
			reset_rtx_timer(0,0);
		}
		else {
			closecwnd(0);
			reset_rtx_timer(0,1);
		}
		ownd_ = 0;
		while ((curconn = conn_iter()) != NULL) {
			curconn->t_seqno_ = curconn->highest_ack_ + 1;
		}
		while ((curseg = seg_iter()) != NULL) {
			/* XXX exclude packets sent "recently"? */
			curseg->size_ = 0;
		}

		send_much(NULL, 0, TCP_REASON_TIMEOUT);
	}
	else
		printf("TcpSessionAgent::timeout(): ignoring unknown timer %d\n", tno);
}

void 
TcpSessionAgent::add_pkts(int size, int seqno, int sessionSeqno, int daddr, int dport,
			  int sport, double ts, IntTcpAgent *sender)
{
	/*
	 * set rtx timer afresh either if it is not set now or if there are no data 
	 * packets outstanding at this time
	 */
	if (!(rtx_timer_.status() == TIMER_PENDING) || seglist_.count() == 0)
		set_rtx_timer();
	last_seg_sent_ = CorresHost::add_pkts(size, seqno, sessionSeqno, daddr, dport, sport, ts, sender);
}
		

int
TcpSessionAgent::window()
{
	if (maxcwnd_ == 0)
		return (int(cwnd_));
	else
		return (int(min(cwnd_,maxcwnd_)));
}

void
TcpSessionAgent::send_much(IntTcpAgent *agent, int force, int reason) 
{
	int npackets = 0;
	Islist_iter<Segment> seg_iter(seglist_);

	/* no outstanding data and idle time >= t_rtxcur_ */
	if ((seg_iter.count() == 0) && (last_send_time_ != -1) &&
	    (Scheduler::instance().clock() - last_send_time_ >= t_rtxcur_)) {
		if (restart_bugfix_)
			closecwnd(3);
		else
			closecwnd(0);
	}
	if (reason != TCP_REASON_TIMEOUT && !force && 
	    burstsnd_timer_.status() == TIMER_PENDING)
		return;
	while (ok_to_snd(size_) || (reason == TCP_REASON_TIMEOUT) || 
	       (force && agent)) {
		if (force && agent) {
			agent->send_one(sessionSeqno_++);
			npackets++;
		}
		else {
			IntTcpAgent *sender = who_to_snd(ROUND_ROBIN);
			if (sender) {
				/* if retransmission */
				/* XXX we pick random conn even if rtx timeout */
				if (sender->t_seqno_ < sender->maxseq_) {
					int i = findSessionSeqno(sender, sender->t_seqno_);
					removeSessionSeqno(i);
					sender->send_one(i);
				}
				else {
					sender->send_one(sessionSeqno_++);
					if (!rtt_active_) {
						rtt_active_ = 1;
						rtt_seg_ = last_seg_sent_;
					}
				}
				npackets++;
			}
			else
				break;
		}
		reason = 0;
		force = 0;
		if (maxburst_ && npackets == maxburst_) {
			if (ok_to_snd(size_))
				burstsnd_timer_.resched(t_exact_srtt_*maxburst_/window());
			break;
		}
	}
	if (npackets > 0)
		last_send_time_ = Scheduler::instance().clock();
}

void
TcpSessionAgent::recv(IntTcpAgent *agent, Packet *pkt, int amt_data_acked)
{
	hdr_tcp *tcph = (hdr_tcp *) pkt->access(off_tcp_);

	if (((hdr_flags*)pkt->access(off_flags_))->ecn_ && ecn_)
		quench(1, agent, tcph->seqno());
	if (amt_data_acked > 0) {
		int i = count_bytes_acked_ ? amt_data_acked:1;
		while (i-- > 0)
			opencwnd(size_);
	}
	clean_segs(size_, pkt, agent, sessionSeqno_);
	if (amt_data_acked > 0)
		newack(pkt);
	Packet::free(pkt);
	send_much(NULL,0,0);
}
	
void
TcpSessionAgent::setflags(Packet *pkt)
{
	hdr_flags *hf = (hdr_flags *) pkt->access(off_flags_);
	if (ecn_)
		hf->ecn_capable_ = 1;
}

int
TcpSessionAgent::findSessionSeqno(IntTcpAgent *sender, int seqno)
{
	Islist_iter<Segment> seg_iter(seglist_);
	Segment *cur;
	int min = sessionSeqno_;
	
	while ((cur = seg_iter()) != NULL) {
		if (sender == cur->sender_ && cur->seqno_ >= seqno && 
		    cur->sessionSeqno_ < min)
			min = cur->sessionSeqno_;
	}
	if (min == sessionSeqno_) {
		printf("In TcpSessionAgent::findSessionSeqno: search unsuccessful\n");
		min = sessionSeqno_ - 1;
	}
	return (min);
}


void
TcpSessionAgent::removeSessionSeqno(int sessionSeqno) 
{
	Islist_iter<Segment> seg_iter(seglist_);
	Segment *cur, *prev=NULL;
	
	while ((cur = seg_iter()) != NULL) {
		if (cur->sessionSeqno_ == sessionSeqno) {
			seglist_.remove(cur, prev);
			ownd_-= cur->size_;
			return;
		}
		prev = cur;
	}
	printf("In removeSessionSeqno(): unable to find segment with sessionSeqno = %d\n", sessionSeqno);
}

void
TcpSessionAgent::quench(int how, IntTcpAgent *sender, int seqno)
{
	int i = findSessionSeqno(sender,seqno);

	if (i > recover_) {
		recover_ = sessionSeqno_ - 1;
		closecwnd(how);
	}
}


			


#include "tcp.h"
#include "ip.h"
#include "flags.h"
#include "random.h"
#include "channel.h"

#define TCP_TIMER_BURSTSND 2

class TcpAsymAgent : public TcpAgent {
public:
	TcpAsymAgent();
	virtual void recv(Packet *pkt, Handler*);
	virtual void send(int force, int reason, int maxburst);
protected:
	int off_tcpasym_;
	int ecn_to_echo_;
	virtual void output(int seqno, int reason);
};

static class TcpAsymClass : public TclClass {
public:
	TcpAsymClass() : TclClass("Agent/TCP/Asym") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new TcpAsymAgent());
	} 
} class_tcpasym;

TcpAsymAgent::TcpAsymAgent() : TcpAgent(), ecn_to_echo_(0)
{
	bind("off_tcpasym_", &off_tcpasym_);
}

/*
 * main reception path - should only see acks, otherwise the
 * network connections are misconfigured
 */
void TcpAsymAgent::recv(Packet *pkt, Handler*)
{
	int i;
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	hdr_tcpasym *tcpha = (hdr_tcpasym*)pkt->access(off_tcpasym_);
	hdr_ip* iph = (hdr_ip*)pkt->access(off_ip_);
#ifdef notdef
	if (pkt->type_ != PT_ACK) {
		Tcl::instance().evalf("%s error \"received non-ack\"",
				      name());
		Packet::free(pkt);
		return;
	}
#endif
	ts_peer_ = tcph->ts();

	if (((hdr_flags*)pkt->access(off_flags_))->ecn_)
		quench(1);
	if (((hdr_flags*)pkt->access(off_flags_))->ecn_to_echo_)
		ecn_to_echo_ = 1;
	if (tcph->seqno() > last_ack_) {
		int ackcount = tcph->seqno() - last_ack_;
		newack(pkt);
		for (i=0; i<ackcount; i++)
			opencwnd();
	} else if (tcph->seqno() == last_ack_) {
		if (++dupacks() == NUMDUPACKS) {
                   /* The line below, for "bug_fix_" true, avoids
		    * problems with multiple fast retransmits in one
		    * window of data. 
		    */
		   	if (!bug_fix_ || highest_ack() > recover_) {
				recover_ = maxseq();
				recover_cause_ = 1;
				closecwnd(0);
				reset_rtx_timer(0);
			}
			else if (ecn_ && recover_cause_ != 1) {
				closecwnd(2);
				reset_rtx_timer(0);
			}
		}
	}
	Packet::free(pkt);
#ifdef notdef
	if (trace_)
		plot();
#endif
	/*
	 * Try to send more data.
	 */
	send(0, 0, maxburst_);
}


void TcpAsymAgent::output(int seqno, int reason)
{
	Packet* p = allocpkt();
	hdr_tcp *tcph = (hdr_tcp*)p->access(off_tcp_);
	hdr_tcpasym *tcpha = (hdr_tcpasym*)p->access(off_tcpasym_);
	hdr_flags *flagsh = (hdr_flags*)p->access(off_flags_);

	double now = Scheduler::instance().clock();
	tcph->seqno() = seqno;
	tcph->ts() = now;
	tcph->ts_echo() = ts_peer_;
	tcph->reason() = reason;

	tcpha->win() == min(highest_ack()+window(), curseq_) - t_seqno();
	tcpha->highest_ack() = highest_ack();
	tcpha->max_left_to_send() = curseq_ - highest_ack();

	flagsh->ecn_ = ecn_to_echo_;
	ecn_to_echo_ = 0;

	Connector::send(p, 0);
	if (seqno > maxseq()) {
		maxseq() = seqno;
		if (!rtt_active_) {
			rtt_active_ = 1;
			if (seqno > rtt_seq_)
				rtt_seq_ = seqno;
		}
	}
	if (!pending_[TCP_TIMER_RTX])
		/* No timer pending.  Schedule one. */
		set_rtx_timer();
}

/*
 * Try to send as much data as the window will allow.  The link layer will 
 * do the buffering; we ask the application layer for the size of the packets.
 */
void TcpAsymAgent::send(int force, int reason, int maxburst)
{
	int win = window();
	int npackets = 0;

	if (!force && pending_[TCP_TIMER_DELSND])
		return;
	while (t_seqno() <= highest_ack() + win && t_seqno() < curseq_) {
		if (overhead_ == 0 || force) {
			output(t_seqno()++, reason);
			npackets++;
		} else if (!pending_[TCP_TIMER_DELSND]) {
			/*
			 * Set a delayed send timeout.
			 */
			sched(Random::uniform(overhead_), TCP_TIMER_DELSND);
			return;
		}
		if (maxburst && npackets == maxburst)
			break;
	}
	/* 
	 * If there is still data to be sent and there is space in the
	 * window, set a timer to schedule the next burst.
	 */
	if (t_seqno() <= highest_ack() + win && t_seqno() < curseq_) {
		if (pending_[TCP_TIMER_BURSTSND])
			cancel(TCP_TIMER_BURSTSND);
		sched((t_srtt()>>3)*tcp_tick_*maxburst/cwnd(), TCP_TIMER_BURSTSND);
	}
}


#include "tcp.h"
#include "random.h"

#define TCP_TIMER_BURSTSND 2

class TcpAsymAgent : public TcpAgent {
public:
	TcpAsymAgent();
	virtual void recv(Packet *pkt);
	virtual void send(int force, int reason, int maxburst);
protected:
	virtual void output(int seqno, int reason);
	virtual void timeout(int tno);
};

static class TcpAsymClass : public TclCLass {
public:
	TcpAsymClass() : TclClass("Agent/TCP/Asym") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new TcpAsymAgent());
	} 
} class_tcpasym;

TcpAsymAgent::TcpAsymAgent() : TcpAgent()
{
}

/*
 * main reception path - should only see acks, otherwise the
 * network connections are misconfigured
 */
void TcpAgent::recv(Packet *pkt, Handler*)
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
	if (((hdr_flags*)pkt->access(off_flags_))->ecn_)
		quench(1);
	if (tcph->seqno() > last_ack_) {
		newack(pkt);
		if (tcpha->ackcount()) {
			for (i=0; i<tcpha->ackcount(); i++)
				opencwnd();
		}
		else
			opencwnd();
	} else if (tcph->seqno() == last_ack_) {
		if (++dupacks_ == NUMDUPACKS) {
                   /* The line below, for "bug_fix_" true, avoids
		    * problems with multiple fast retransmits in one
		    * window of data. 
		    */
		   	if (!bug_fix_ || highest_ack_ > recover_) {
				recover_ = maxseq_;
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


void TcpAgent::output(int seqno, int reason)
{
	Packet* p = allocpkt();
	hdr_tcp *tcph = (hdr_tcp*)p->access(off_tcp_);
	hdr_tcpasym *tcpha = (hdr_tcpasym*)pkt->access(off_tcpasym_);
	double now = Scheduler::instance().clock();
	tcph->seqno() = seqno;
	tcph->ts() = now;
	tcph->reason() = reason;

	tcpha->win() == min(highest_ack_+window(), curseq_) - seqno_;
	tcpha->highest_ack() = highest_ack_;
	tcpha->max_left_to_send() = curseq_ - highest_ack_;

	Connector::send(p, 0);
	if (seqno > maxseq_) {
		maxseq_ = seqno;
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
void TcpAgent::send(int force, int reason, int maxburst)
{
	int win = window();
	int npackets = 0;

	if (!force && pending_[TCP_TIMER_DELSND])
		return;
	while (t_seqno_ <= highest_ack_ + win && t_seqno_ < curseq_) {
		if (overhead_ == 0 || force) {
			output(t_seqno_++, reason);
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
}


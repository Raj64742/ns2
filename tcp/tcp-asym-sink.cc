#include "tcp-sink.h"

class TcpAsymSink : public DelAckSink {
public:
	TcpAsymSink(Acker*);
	virtual void recv(Packet* pkt, Handler* h);
protected:
	virtual void add_to_ack(Packet* pkt);
	int off_tcpasym_;
	int delackcount_;       /* the number of consecutive packets that have
				   not been acked yet */
	int maxdelack_;         /* the maximum extent to which acks can be
				   delayed */
	int delackfactor_;      /* the dynamically varying limit on the extent
				   to which acks can be delayed */
	int delacklim_;         /* limit on the extent of del ack based on the
				   sender's window */
	double ts_ecn_;         /* the time when an ECN was received last */
	double ts_decrease_;    /* the time when delackfactor_ was decreased last */
	double highest_ts_echo_;/* the highest timestamp echoed by the peer */
};	

static class TcpAsymSinkClass : public TclClass {
public:
	TcpAsymSinkClass() : TclClass("Agent/TCPSink/Asym") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new TcpAsymSink(new Acker));
	}
} class_tcpasymsink;

TcpAsymSink::TcpAsymSink(Acker* acker) : delackcount_(0), delackfactor_(1), delacklim_(0), ts_ecn_(0), ts_decrease_(0), DelAckSink(acker)
{
	bind("maxdelack_", &maxdelack_);
	bind("off_tcpasym_", &off_tcpasym_);
}

/* Add fields to the ack */
void TcpAsymSink::add_to_ack(Packet* pkt) 
{
	hdr_tcpasym *tha = (hdr_tcpasym*)pkt->access(off_tcpasym_);
	tha->ackcount() = delackcount_;
}

void TcpAsymSink::recv(Packet* pkt, Handler* h) 
{
	int olddelackfactor = delackfactor_;
	int olddelacklim = delacklim_; 
	int max_sender_can_send = 0;
	hdr_flags *fh = (hdr_flags*)pkt->access(off_flags_); 
	hdr_tcp *th = (hdr_tcp*)pkt->access(off_tcp_);
	hdr_tcpasym *tha = (hdr_tcpasym*)pkt->access(off_tcpasym_);
	double now = Scheduler::instance().clock();

	acker_->update(th->seqno());

	highest_ts_echo_ = max(highest_ts_echo_, th->ts_echo());
	/* 
	 * if we receive an ECN and haven't received one in the past
	 * round-trip, double delackfactor_ 
	 */
	if (fh->ecn_ && highest_ts_echo_ >= ts_ecn_) {
		delackfactor_ = min(2*delackfactor_, maxdelack_);
		ts_ecn_ = now;
	}
	/*
	 * else if we haven't received an ECN in the past round trip and
	 * haven't (linearly) decreased delackfactor_ in the past round
	 * trip, we decrease delackfactor_ by 1
	 */
	else if (highest_ts_echo_ >= ts_ecn_ && highest_ts_echo_ >= ts_decrease_) {
		delackfactor_ = max(delackfactor_ - 1, 1);
		ts_decrease_ = now;
	}

	/*
         * If this is the next packet in sequence,
         * we can consider delaying the ack. Set
         * max_ackcount based on sender's window.
         */
        if (th->seqno() == acker_->Seqno()) {
		max_sender_can_send = (int) min(tha->win()+acker_->Seqno()-tha->highest_ack(), tha->max_left_to_send());
		delacklim_ = min(maxdelack_, max_sender_can_send/3); /* XXXX */ 
	}
	else
		delacklim_ = 0;

	if (delackfactor_ < delacklim_) 
		delacklim_ = delackfactor_;

	if (channel_ && (olddelackfactor != delackfactor_ || olddelacklim != delacklim_)) {
		char wrk[500];
		int n;

		/* we print src and dst in reverse order to conform to sender side */
		sprintf(wrk, "time: %-6.3f saddr: %-2d sport: %-2d daddr: %-2d dport: %-2d dafactor: %2d dalim: %2d\n", now, dst_/256, dst_%256, addr_/256, addr_%256, delackfactor_, delacklim_);
		n = strlen(wrk);
		wrk[n] = '\n';
		wrk[n+1] = 0;
		(void)Tcl_Write(channel_, wrk, n+1);
		wrk[n] = 0;
	}
		
	delackcount_++;
	if (delackcount_ < delacklim_) { /* it is not yet time to send an ack */
		/* set delayed ack timer if it is not already set */
		if (!pending_[DELAY_TIMER]) {
			save_ = pkt;
			sched(interval_, DELAY_TIMER);
		}
		else {
			hdr_tcp *sth = (hdr_tcp*)save_->access(off_tcp_);
			/* save the pkt with the more recent timestamp */
			if (th->ts() > sth->ts()) {
				Packet::free(save_);
				save_ = pkt;
			}
		}
		return;
	}
	else { /* send back an ack now */
		if (pending_[DELAY_TIMER]) {
			cancel(DELAY_TIMER);
			Packet::free(save_);
			save_ = 0;
		}
		ack(pkt);
		delackcount_ = 0;
		Packet::free(pkt);
	}
}


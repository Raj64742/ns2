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
	int delacklimit_;       /* the dynamically varying limit on the extent
				   to which acks can be delayed */
	double ts_ecn_;         /* the time when an ECN was received last */
	double ts_decrease_;    /* the time when delacklimit_ was decreased last */
	double highest_ts_echo_;/* the highest timestamp echoed by the peer */
};	

static class TcpAsymSinkClass : public TclClass {
public:
	TcpAsymSinkClass() : TclClass("Agent/TCPSink/Asym") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new TcpAsymSink(new Acker));
	}
} class_tcpasymsink;

TcpAsymSink::TcpAsymSink(Acker* acker) : delackcount_(0), delacklimit_(1), ts_ecn_(0), ts_decrease_(0), DelAckSink(acker)
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
	int delacklim = 0;
	int max_sender_can_send = 0;
	hdr_flags *fh = (hdr_flags*)pkt->access(off_flags_); 
	hdr_tcp *th = (hdr_tcp*)pkt->access(off_tcp_);
	hdr_tcpasym *tha = (hdr_tcpasym*)pkt->access(off_tcpasym_);
	double now = Scheduler::instance().clock();

	acker_->update(th->seqno());

	highest_ts_echo_ = max(highest_ts_echo_, th->ts_echo());
	/* 
	 * if we receive an ECN and haven't received one in the past
	 * round-trip, double delacklimit_ 
	 */
	if (fh->ecn_ && highest_ts_echo_ >= ts_ecn_) {
		delacklimit_ = min(2*delacklimit_, maxdelack_);
		ts_ecn_ = now;
	}
	/*
	 * else if we haven't received an ECN in the past round trip and
	 * haven't (linearly) decreased delacklimit_ in the past round
	 * trip, we decrease delacklimit_ by 1
	 */
	else if (highest_ts_echo_ >= ts_ecn_ && highest_ts_echo_ >= ts_decrease_) {
		delacklimit_ = max(delacklimit_ - 1, 1);
		ts_decrease_ = now;
	}

	/*
         * If this is the next packet in sequence,
         * we can consider delaying the ack. Set
         * max_ackcount based on sender's window.
         */
        if (th->seqno() == acker_->Seqno()) {
		max_sender_can_send = (int) min(tha->win()+acker_->Seqno()-tha->highest_ack(), tha->max_left_to_send());
		delacklim = min(maxdelack_, max_sender_can_send/3); /* XXXX */ 
	}
	else
		delacklim = 0;

	if (delacklimit_ < delacklim) 
		delacklim = delacklimit_;

	delackcount_++;
	if (delackcount_ < delacklim) { /* it is not yet time to send an ack */
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


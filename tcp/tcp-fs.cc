/*
 * tcp-fs: TCP with "fast start", a procedure for avoiding the penalty of
 * slow start when a connection resumes after a pause. The connection tries
 * to reuse values old values of variables such as cwnd_, ssthresh_, srtt_,
 * etc., with suitable modifications. The same procedure as used in tcp-asym
 * is used to clock out packets until ack clocking kicks in. A connection 
 * doing fast start protects itself and other connections in the network against 
 * the adverse consequences of stale information by (a) marking all packets sent 
 * during fast start as low priority for the purposes of a priority-drop router, 
 * and (b) quickly detecting the loss of several fast start packets and falling 
 * back to a regular slow start, almost as if a fast start had never been attempted
 * in the first place.
 * 
 * Contributed by Venkat Padmanabhan (padmanab@cs.berkeley.edu), 
 * Daedalus Research Group, U.C.Berkeley
 */


#include "tcp.h"
#include "ip.h"
#include "flags.h"
#include "random.h"
#include "template.h"
#include "tcp-fack.h"

class ResetTimer : public TimerHandler {
public: 
	ResetTimer(TcpAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	TcpAgent *a_;
};

void ResetTimer::expire(Event *e) {
	a_->timeout(TCP_TIMER_RESET);
}

/* TCP-FS with Tahoe */
class TcpFsAgent : public virtual TcpAgent {
public:
	TcpFsAgent() : t_exact_srtt_(0), t_exact_rttvar_(0), last_recv_time_(0), 
		fs_startseq_(0), fs_endseq_(0), fs_mode_(0), reset_timer_(this) 
	{
		bind_bool("fast_loss_recov_", &fast_loss_recov_);
	}

	/* helper functions */
	virtual void output_helper(Packet* pkt);
	virtual void recv_helper(Packet* pkt);
	virtual void send_helper(int maxburst);
	virtual void send_idle_helper();
	virtual void recv_newack_helper(Packet* pkt);
	virtual void partialnewack_helper(Packet* pkt) {};

	virtual void set_rtx_timer();
	virtual void timeout_nonrtx(int tno);
	virtual void timeout_nonrtx_helper(int tno);
	double rtt_exact_timeout() { return (t_exact_srtt_ + 4*t_exact_rttvar_);}
protected:
	double t_exact_srtt_;
	double t_exact_rttvar_;
	double last_recv_time_;
	int fs_startseq_;
	int fs_endseq_;
	int fs_mode_;
	int fast_loss_recov_;
	ResetTimer reset_timer_;
};

static class TcpFsClass : public TclClass {
public:
	TcpFsClass() : TclClass("Agent/TCP/FS") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new TcpFsAgent());
	}
} class_tcpfs;

/* TCP-FS with Reno */
class RenoTcpFsAgent : public RenoTcpAgent, public TcpFsAgent {
public:
	RenoTcpFsAgent() : RenoTcpAgent(), TcpFsAgent() {}

	/* helper functions */
	virtual void output_helper(Packet* pkt) {TcpFsAgent::output_helper(pkt);}
	virtual void recv_helper(Packet* pkt) {TcpFsAgent::recv_helper(pkt);}
	virtual void send_helper(int maxburst) {TcpFsAgent::send_helper(maxburst);}
	virtual void send_idle_helper() {TcpFsAgent::send_idle_helper();}
	virtual void recv_newack_helper(Packet* pkt) {TcpFsAgent::recv_newack_helper(pkt);}

	virtual void set_rtx_timer() {TcpFsAgent::set_rtx_timer();}
	virtual void timeout_nonrtx(int tno) {TcpFsAgent::timeout_nonrtx(tno);}
	virtual void timeout_nonrtx_helper(int tno);
};

static class RenoTcpFsClass : public TclClass {
public:
	RenoTcpFsClass() : TclClass("Agent/TCP/Reno/FS") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new RenoTcpFsAgent());
	}
} class_renotcpfs;	

/* TCP-FS with NewReno */
class NewRenoTcpFsAgent : public NewRenoTcpAgent, public TcpFsAgent {
public:
	NewRenoTcpFsAgent() : NewRenoTcpAgent(), TcpFsAgent() {}

	/* helper functions */
	virtual void output_helper(Packet* pkt) {TcpFsAgent::output_helper(pkt);}
	virtual void recv_helper(Packet* pkt) {TcpFsAgent::recv_helper(pkt);}
	virtual void send_helper(int maxburst) {TcpFsAgent::send_helper(maxburst);}
	virtual void send_idle_helper() {TcpFsAgent::send_idle_helper();}
	virtual void recv_newack_helper(Packet* pkt) {TcpFsAgent::recv_newack_helper(pkt);}
	virtual void partialnewack_helper(Packet* pkt);

	virtual void set_rtx_timer() {TcpFsAgent::set_rtx_timer();}
	virtual void timeout_nonrtx(int tno) {TcpFsAgent::timeout_nonrtx(tno);}
	virtual void timeout_nonrtx_helper(int tno);
};

static class NewRenoTcpFsClass : public TclClass {
public:
	NewRenoTcpFsClass() : TclClass("Agent/TCP/Newreno/FS") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new NewRenoTcpFsAgent());
	}
} class_newrenotcpfs;	

/* TCP-FS with Fack */
class FackTcpFsAgent : public FackTcpAgent, public TcpFsAgent {
public:
	FackTcpFsAgent() : FackTcpAgent(), TcpFsAgent() {}

	/* helper functions */
	virtual void output_helper(Packet* pkt) {TcpFsAgent::output_helper(pkt);}
	virtual void recv_helper(Packet* pkt) {TcpFsAgent::recv_helper(pkt);}
	virtual void send_helper(int maxburst);
	virtual void send_idle_helper() {TcpFsAgent::send_idle_helper();}
	virtual void recv_newack_helper(Packet* pkt) {TcpFsAgent::recv_newack_helper(pkt);}
	virtual void set_rtx_timer() {TcpFsAgent::set_rtx_timer();}
	virtual void timeout_nonrtx(int tno) {TcpFsAgent::timeout_nonrtx(tno);}
	virtual void timeout_nonrtx_helper(int tno);
};

static class FackTcpFsClass : public TclClass {
public:
	FackTcpFsClass() : TclClass("Agent/TCP/Fack/FS") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new FackTcpFsAgent());
	}
} class_facktcpfs;	


/* mark packets sent as part of fast start */
void
TcpFsAgent::output_helper(Packet *pkt)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	double now = Scheduler::instance().clock();
	double idle_time = now - last_recv_time_;
	double timeout = ((t_srtt_ >> 3) + t_rttvar_) * tcp_tick_ ;
	maxseq_ = max(maxseq_, highest_ack_);

	/* 
	 * if the connection has been idle (with no outstanding data) for long 
	 * enough, we enter the fast start phase. We compute the start and end
	 * sequence numbers that define the fast start phase. Note that the
	 * first packet to be sent next is not included because it would have
	 * been sent even with regular slow start.
	 */
	if ((idle_time > timeout) && (maxseq_ == highest_ack_) && (cwnd_ > 1)){
		/*
		 * We set cwnd_ to a "safe" value: cwnd_/2 if the connection
		 * was in slow start before the start of the idle period and
		 * cwnd_-1 if it was in congestion avoidance phase.
		 */
		if (cwnd_ < ssthresh_+1)
			cwnd_ = int(cwnd_/2);
		else
			cwnd_ -= 1;
		/* set the start and end seq. nos. for the fast start phase */
		fs_startseq_ = highest_ack_+2;
		fs_endseq_ = highest_ack_+window()+1;
		fs_mode_ = 1;
	}
	/* initially set fs_ flag to 0 */
	((hdr_flags*)pkt->access(off_flags_))->fs_ = 0;
        /* check if packet belongs to the fast start phase. */
	if (tcph->seqno() >= fs_startseq_ && tcph->seqno() < fs_endseq_ && fs_mode_) {
		/* if not a retransmission, mark the packet */
		if (tcph->seqno() > maxseq_) {
			((hdr_flags*)pkt->access(off_flags_))->fs_ = 1;
			/* ((hdr_cmn*)pkt->access(off_cmn_))->size() = 0; */ /* XXXX temporary -- only for testing */
		}
	}
}

/* update last_output_time_ */
void
TcpFsAgent::recv_helper(Packet *pkt) 
{
	double now = Scheduler::instance().clock();
	last_recv_time_ = now;
}

/* schedule the next burst of data (of size at most maxburst) */
void 
TcpFsAgent::send_helper(int maxburst) 
{
	/* 
	 * If there is still data to be sent and there is space in the
	 * window, set a timer to schedule the next burst. Note that
	 * we wouldn't get here if TCP_TIMER_BURSTSEND were pending,
	 * so we do not need an explicit check here.
	 */
	if (t_seqno_ <= highest_ack_ + window() && t_seqno_ < curseq_) {
		burstsnd_timer_.resched(t_exact_srtt_*maxburst/window());
	}
}

/* schedule the next burst of data (of size at most maxburst) */
void 
FackTcpFsAgent::send_helper(int maxburst) 
{
	/* 
	 * If there is still data to be sent and there is space in the
	 * window, set a timer to schedule the next burst. Note that
	 * we wouldn't get here if TCP_TIMER_BURSTSEND were pending,
	 * so we do not need an explicit check here.
	 */
	if ((t_seqno_ <= fack_ + window() - retran_data_) && (!timeout_) && (t_seqno_ < curseq_)) {
		burstsnd_timer_.resched(t_exact_srtt_*maxburst/window());
	}
}

/* do appropriate processing depending on the length of idle time */
void
TcpFsAgent::send_idle_helper() 
{
	double now = Scheduler::instance().clock();
	double idle_time = now - last_recv_time_;
}

/* update srtt estimate */
void
TcpFsAgent::recv_newack_helper(Packet *pkt) 
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	double tao = Scheduler::instance().clock() - tcph->ts_echo();
	double g = 1/8; /* gain used for smoothing rtt */
	double h = 1/4; /* gain used for smoothing rttvar */
	double delta;

	newack(pkt);
	maxseq_ = max(maxseq_, highest_ack_);
	if (t_exact_srtt_ != 0) {
		delta = tao - t_exact_srtt_;
		if (delta < 0)
			delta = -delta;
		/* update the fine-grained estimate of the smoothed RTT */
		if (t_exact_srtt_ != 0) 
			t_exact_srtt_ = g*tao + (1-g)*t_exact_srtt_;
		else
			t_exact_srtt_ = tao;
		/* update the fine-grained estimate of mean deviation in RTT */
		delta -= t_exact_rttvar_;
		t_exact_rttvar_ += h*delta;
	}
	else {
		t_exact_srtt_ = tao;
		t_exact_rttvar_ = tao/2;
	}
	/* grow cwnd */
	opencwnd();
	/* check if we are out of fast start mode */
	if (fs_mode_ && (highest_ack_ >= fs_endseq_-1)) 
		fs_mode_ = 0;
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}
}

void
NewRenoTcpFsAgent::partialnewack_helper(Packet* pkt)
{
	partialnewack(pkt);
	/* Do this because we may have retracted maxseq_ */
	maxseq_ = max(maxseq_, highest_ack_);
	if (fs_mode_ && fast_loss_recov_) {
		/* 
		 * A partial new ack implies that more than one packet has been lost
		 * in the window. Rather than recover one loss per RTT, we get out of
		 * fast start mode and do a slow start (no rtx timeout, though).
		 */
		timeout_nonrtx(TCP_TIMER_RESET);
	}
	else {
		output(last_ack_ + 1, 0);
	}
}

void 
TcpFsAgent::set_rtx_timer()
{
	if (rtx_timer_.status() == TIMER_PENDING)
		rtx_timer_.cancel();
	if (reset_timer_.status() == TIMER_PENDING)
		reset_timer_.cancel();
	if (fs_mode_ && fast_loss_recov_)
		reset_timer_.resched(rtt_exact_timeout());
	else 
		rtx_timer_.resched(rtt_timeout());
}

void 
TcpFsAgent::timeout_nonrtx(int tno) 
{
	if (tno == TCP_TIMER_RESET) {
		fs_mode_ = 0;  /* out of fast start mode */
		dupacks_ = 0;  /* just to be safe */
		if (highest_ack_ == maxseq_ && !slow_start_restart_) {
			/*
			 * TCP option:
			 * If no outstanding data, then don't do anything.
			 */
			return;
		};
		/* 
		 * If the pkt sent just before the fast start phase has not
		 * gotten through, treat this as a regular rtx timeout.
		 */
		if (highest_ack_ < fs_startseq_-1) {
			maxseq_ = fs_startseq_ - 1;
			recover_ = maxseq_;
			timeout(TCP_TIMER_RTX);
		}
		/* otherwise decrease window size to 1 but don't back off rtx timer */
		else {
			if (highest_ack_ > last_ack_)
				last_ack_ = highest_ack_;
			maxseq_ = last_ack_;
			recover_ = maxseq_;
			recover_cause_ = RECOVER_TIMEOUT;
			closecwnd(3);
			timeout_nonrtx_helper(tno);
		}
	}
	else {
		TcpAgent::timeout_nonrtx(tno);
	}
}

void 
TcpFsAgent::timeout_nonrtx_helper(int tno)
{
	if (tno == TCP_TIMER_RESET) {
		reset_rtx_timer(0,0);
		send_much(0, TCP_REASON_TIMEOUT, maxburst_);
	}
}

void 
RenoTcpFsAgent::timeout_nonrtx_helper(int tno)
{
	if (tno == TCP_TIMER_RESET) {
		dupwnd_ = 0;
		dupacks_ = 0;
		TcpFsAgent::timeout_nonrtx_helper(tno);
	}
}		

void 
NewRenoTcpFsAgent::timeout_nonrtx_helper(int tno)
{
	if (tno == TCP_TIMER_RESET) {
		dupwnd_ = 0;
		dupacks_ = 0;
		TcpFsAgent::timeout_nonrtx_helper(tno);
	}
}

void 
FackTcpFsAgent::timeout_nonrtx_helper(int tno)
{
	if (tno == TCP_TIMER_RESET) {
		timeout_ = FALSE;
		retran_data_ = 0;
		fack_ = last_ack_;
		t_seqno_ = last_ack_ + 1;
		reset_rtx_timer(TCP_REASON_TIMEOUT, 0);
		send_much(0, TCP_REASON_TIMEOUT);
	}
}

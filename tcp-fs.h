#ifndef ns_tcp_fs_h
#define ns_tcp_fs_h

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

/* TCP-FS with Tahoe */
class TcpFsAgent : public virtual TcpAgent {
public:
	TcpFsAgent() : t_exact_srtt_(0), t_exact_rttvar_(0), last_recv_time_(0), 
		fs_startseq_(0), fs_endseq_(0), fs_mode_(0), count_acks_(0),
		reset_timer_(this) 
	{
		bind_bool("fast_loss_recov_", &fast_loss_recov_);
		bind_bool("fast_reset_timer_", &fast_reset_timer_);
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
	int fast_reset_timer_;
	int count_acks_;
	ResetTimer reset_timer_;
};

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

/* TCP-FS with NewReno */
class NewRenoTcpFsAgent : public virtual NewRenoTcpAgent, public TcpFsAgent {
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

#endif

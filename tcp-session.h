#ifndef nc_tcp_session_h
#define ns_tcp_session_h

#include "agent.h"
#include "packet.h"
#include "timer-handler.h"
#include "chost.h"

#define FINE_ROUND_ROBIN 1
#define COARSE_ROUND_ROBIN 2
#define RANDOM 3

class TcpSessionAgent;

class SessionRtxTimer : public TimerHandler {
public: 
	SessionRtxTimer(TcpSessionAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	TcpSessionAgent *a_;
};

class SessionBurstSndTimer : public TimerHandler {
public: 
	SessionBurstSndTimer(TcpSessionAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	TcpSessionAgent *a_;
};

/*
 * This class implements the notion of a TCP session. It integrates congestion 
 * control and loss detection functionality across the one or more IntTcp 
 * connections that comprise the session.
 */
class TcpSessionAgent : public CorresHost {
public:
	TcpSessionAgent();
	int command(int argc, const char*const* argv);
	void reset_rtx_timer(int mild, int backoff = 1); /* XXX mild needed ? */
	void set_rtx_timer();
	void cancel_rtx_timer();
	void newack(Packet *pkt);
	void timeout(int tno);
	virtual Segment* add_pkts(int size, int seqno, int sessionSeqno, int daddr, 
		      int dport, int sport, double ts, IntTcpAgent *sender); 
	virtual void add_agent(IntTcpAgent *agent, int size, double winMult,
		       int winInc, int ssthresh);
	int window();
	void set_weight(IntTcpAgent *tcp, int wt);
	void reset_dyn_weights();
	IntTcpAgent *who_to_snd(int how);
	void send_much(IntTcpAgent *agent, int force, int reason); 
	void recv(IntTcpAgent *agent, Packet *pkt, int amt_data_acked);
	void setflags(Packet *pkt);
	int findSessionSeqno(IntTcpAgent *sender, int seqno);
	void removeSessionSeqno(int sessionSeqno);
	void quench(int how, IntTcpAgent *sender, int seqno);
	virtual void traceVar(TracedVar* v);

	inline nsaddr_t& addr() {return addr_;}
	inline nsaddr_t& dst() {return dst_;}
	static class Islist<TcpSessionAgent> sessionList_;

protected:
	SessionRtxTimer rtx_timer_;
	SessionBurstSndTimer burstsnd_timer_;
	int sessionSeqno_;
	double last_send_time_;
	Segment *last_seg_sent_;
	IntTcpAgent *curConn_;
	int numConsecSegs_;
	int schedDisp_;
	int wtSum_;
	int dynWtSum_;
};

#endif

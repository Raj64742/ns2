#ifndef nc_tcp_session_h
#define ns_tcp_session_h

#include "agent.h"
#include "packet.h"
#include "timer-handler.h"
#include "chost.h"

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
 * This class implements the notion of a TCP session. It integrates congestion control
 * and loss detection functionality across the one or more IntTcp connections that
 * comprise the session.
 */
class TcpSessionAgent : public CorresHost {
public:
	TcpSessionAgent();
/*	TcpSessionAgent(int dst);*/
	void reset_rtx_timer(int mild, int backoff = 1); /* XXX mild needed ? */
	void set_rtx_timer();
	void cancel_rtx_timer();
	void newack(Packet *pkt);
	void timeout(int tno);
	void add_pkts(int size, int seqno, int sessionSeqno, int daddr, 
		      int dport, int sport, double ts, IntTcpAgent *sender); 
	int window();
	void send_much(IntTcpAgent *agent, int force, int reason); 
	void recv(IntTcpAgent *agent, Packet *pkt, int amt_data_acked);
	void setflags(Packet *pkt);
	int findSessionSeqno(IntTcpAgent *sender, int seqno);
	void removeSessionSeqno(int sessionSeqno);
	void quench(int how, IntTcpAgent *sender, int seqno);

	inline nsaddr_t& addr() {return addr_;}
	inline nsaddr_t& dst() {return dst_;}
	static class Islist<TcpSessionAgent> sessionList_;

protected:
	SessionRtxTimer rtx_timer_;
	SessionBurstSndTimer burstsnd_timer_;
/*	int maxburst_;
	int size_;
	int ecn_;*/
	int sessionSeqno_;
	double last_send_time_;
	Segment *last_seg_sent_;
/*	double t_exact_srtt_;
	int slow_start_restart_;
	int restart_bugfix_;
	int count_acks_;*/
};

#endif

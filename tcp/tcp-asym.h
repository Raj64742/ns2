#ifndef ns_tcp_asym_h
#define ns_tcp_asym_h
#include "tcp.h"
#include "ip.h"
#include "flags.h"
#include "random.h"
#include "template.h"

/* TCP Asym with Tahoe */
class TcpAsymAgent : public virtual TcpAgent {
public:
	TcpAsymAgent();

	/* helper functions */
	virtual void output_helper(Packet* p);
	virtual void send_helper(int maxburst);
	virtual void recv_helper(Packet* p);
	virtual void recv_newack_helper(Packet* pkt);
	virtual void traceAll();
	virtual void traceVar(TracedVar* v);
protected:
	int off_tcpasym_;
	int ecn_to_echo_;
	TracedDouble t_exact_srtt_;
/*	TracedDouble avg_win_; */
	double g_;		/* gain used for exact_srtt_ smoothing */
	int damp_;
};

/* TCP Asym with Reno */
class TcpRenoAsymAgent : public RenoTcpAgent,
			 public TcpAsymAgent {
public:
	 TcpRenoAsymAgent() : RenoTcpAgent(), TcpAsymAgent() { }

	/* helper functions */
	virtual void output_helper(Packet* p) { TcpAsymAgent::output_helper(p); }
	virtual void send_helper(int maxburst) { TcpAsymAgent::send_helper(maxburst); }
	virtual void recv_helper(Packet* p) { TcpAsymAgent::recv_helper(p); }
	virtual void recv_newack_helper(Packet* pkt) { TcpAsymAgent::recv_newack_helper(pkt); }

};

/* TCP Asym with New Reno */
class NewRenoTcpAsymAgent : public virtual NewRenoTcpAgent,
			    public TcpAsymAgent {
public:
	 NewRenoTcpAsymAgent() : NewRenoTcpAgent(), TcpAsymAgent() { }

	/* helper functions */
	virtual void output_helper(Packet* p) { TcpAsymAgent::output_helper(p); }
	virtual void send_helper(int maxburst) { TcpAsymAgent::send_helper(maxburst); }
	virtual void recv_helper(Packet* p) { TcpAsymAgent::recv_helper(p); }
	virtual void recv_newack_helper(Packet* pkt) { TcpAsymAgent::recv_newack_helper(pkt); }

};

#endif

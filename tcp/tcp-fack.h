#include "tcp.h"
#include "scoreboard.h"

/* TCP Fack */
class FackTcpAgent : public virtual TcpAgent {
 public:
	FackTcpAgent();
	virtual void recv(Packet *pkt, Handler*);
	virtual void timeout(int tno);
	virtual void closecwnd(int how);
	virtual void opencwnd();
	virtual int window();
	void oldack (Packet* pkt);
	int maxsack (Packet* pkt); 
	void plot();
	virtual void send_much(int force, int reason, int maxburst = 0);
	virtual void recv_newack_helper(Packet* pkt);
 protected:
	u_char timeout_;        /* flag: sent pkt from timeout; */
	u_char fastrecov_;      /* flag: in fast recovery */
	double wintrim_;
	double wintrimmult_;
	int rampdown_;
	int fack_;
	int retran_data_;
	int ss_div4_;
	ScoreBoard scb_;
};


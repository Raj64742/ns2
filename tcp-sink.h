#include <math.h>
#include "packet.h"
#include "ip.h"
#include "tcp.h"
#include "agent.h"
#include "flags.h"
#include "biconnector.h"

/* max window size */
#define MWS 1024
#define MWM (MWS-1)
/* For Tahoe TCP, the "window" parameter, representing the receiver's
 * advertised window, should be less than MWM.  For Reno TCP, the
 * "window" parameter should be less than MWM/2.
 */

class Acker {
public:
	Acker();
	virtual ~Acker() {}
	void update(int seqno);
	inline int Seqno() const { return (next_ - 1); }
	virtual void append_ack(hdr_cmn*, hdr_tcp*, int oldSeqno) const;
protected:
	int next_;		/* next packet expected  */
	int maxseen_;		/* max packet number seen */
	int seen_[MWS];		/* array of packets seen  */
};

class TcpSink : public Agent {
public:
	TcpSink(Acker*);
	void recv(Packet* pkt, Handler*);
protected:
	void ack(Packet*);
	virtual void add_to_ack(Packet* pkt);  
	Acker* acker_;
	int off_tcp_;
};

#define DELAY_TIMER 0

class DelAckSink : public TcpSink {
public:
	DelAckSink(Acker* acker);
	void recv(Packet* pkt, Handler*);
protected:
	virtual void timeout(int tno);
	Packet* save_;		/* place to stash packet while delaying */
	double interval_;
};


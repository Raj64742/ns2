/*
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the MASH Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "scheduler.h"
#include "packet.h"
#include "ip.h"
#include "tcp.h"
#include "baseLL.h"

/* Snoop states */
#define SNOOP_ACTIVE    0x01	/* connection active */
#define SNOOP_CLOSED    0x02	/* connection closed */
#define SNOOP_NOACK     0x04	/* no ack seen yet */
#define SNOOP_FULL      0x08	/* snoop cache full */
#define SNOOP_HIGHWATER 0x10	/* snoop highwater mark reached */
#define SNOOP_RTTFLAG   0x20	/* can compute RTT if this is set */
#define SNOOP_ALIVE     0x40	/* connection has been alive past 1 sec */
#define SNOOP_SL_REXMT  0x80	/* for persist timeout */

#define SNOOP_MAXWIND   100	/* XXX */
#define SNOOP_MAXCONN   64	/* XXX */
#define SNOOP_MIN_TIMO  0.500	/* in seconds */
#define SNOOP_MAX_RXMIT 10	/* quite arbitrary at this point */
#define SNOOP_PROPAGATE 0
#define SNOOP_SUPPRESS   1

#define SNOOP_MAKEHANDLER 1
#define SNOOP_TAIL 1

struct hdr_snoop {
	int seqno_;
	int numRxmit_;
	int senderRxmit_;
	double sndTime_;

	inline int& seqno() { return seqno_; }
	inline int& numRxmit() { return numRxmit_; }
	inline int& senderRxmit() { return senderRxmit_; }
	inline double& sndTime() { return sndTime_; }
};

class SnoopHeaderClass : public PacketHeaderClass {
public:
        SnoopHeaderClass() : PacketHeaderClass("PacketHeader/Snoop",
						sizeof(hdr_snoop)) {}
} class_snoophdr;

class SnoopRxmitHandler;
class SnoopPersistHandler;

class Snoop : public BaseLL {
	friend SnoopRxmitHandler;
	friend SnoopPersistHandler;
  public:
	Snoop(int makeHandler=0);
	int  command(int argc, const char*const* argv);
	void recv(Packet *, Handler *);	/* control of snoop actions */
	void handle(Event *);	/* control of snoop actions */
	void snoop_rxmit(Packet *);
	inline int next(int i) { return (i+1) % SNOOP_MAXWIND; }
	inline int prev(int i) { return ((i == 0) ? SNOOP_MAXWIND-1 : i-1); };

  protected:
	void snoop_data_(Packet *);
	int  snoop_ack_(Packet *);
	double snoop_cleanbufs_(int);
	void snoop_rtt_(double);
	int insert_(Packet *);
	inline int empty_(){return bufhead_==buftail_ &&!(fstate_&SNOOP_FULL);}
	void savepkt_(Packet *, int, int);
	void update_state_();
	inline double timeout_() { 
		if (2 * srtt_ > SNOOP_MIN_TIMO) 
			return 2 * srtt_;
		else 
			return SNOOP_MIN_TIMO;
	}
	Handler  *callback_;
	SnoopRxmitHandler *rxmitHandler_; /* used in rexmissions */
	SnoopPersistHandler *persistHandler_; /* for connection (in)activity */
	int      snoopDisable_;	/* disable snoop for this mobile */
	int      connId_;	/* which connection -- unique from dst:port */
	u_short  fstate_;	/* state of connection */
	u_short  lastWin_;	/* last flow control window size from sender */
	nsaddr_t destAddr_;	/* identifies mobile host */
	int      lastSeen_;	/* first byte of last packet buffered */
	int      lastSize_;	/* size of last cached segment */
        int      lastAck_;	/* last byte recd. by mh for sure */
	int      expNextAck_;	/* expected next ack after dup sequence */
	short    expDupacks_;	/* expected number of dup acks */
	double   srtt_;		/* smoothed rtt estimate */
	double   rttvar_;	/* linear deviation */
	short    bufhead_;	/* next pkt goes here */
	short    buftail_;	/* first unack'd pkt */
	Event    *toutPending_;	/* # pending timeouts */
	Packet   *pkts_[SNOOP_MAXWIND]; /* ringbuf of cached mbufs */
	
	int      off_snoop_;	/* snoop header offset */
	int      off_tcp_;	/* snoop header offset */
};

class SnoopRxmitHandler : public Handler {
  public:
	SnoopRxmitHandler(Snoop *s) : snoop_(s) {}
	void handle(Event *event);
  protected:
	Snoop *snoop_;
};

class SnoopPersistHandler : public Handler {
  public:
	SnoopPersistHandler(Snoop *s) : snoop_(s) {}
	void handle(Event *);
  protected:
	Snoop *snoop_;
};

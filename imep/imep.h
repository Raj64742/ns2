/* -*- c++ -*-
   imep.h
   $Id: imep.h,v 1.1 1999/08/03 04:12:35 yaxu Exp $
   */

#ifndef __imep_h__
#define __imep_h__

#include <stdarg.h>
#include <sys/types.h>

#include <config.h>
#include <agent.h>
#include <drop-tail.h>
#include <trace.h>

#include <list.h>
#include <rtproto/rtproto.h>
#include <imep/rxmit_queue.h>
#include <imep/dest_queue.h>
#include <imep/imep_stats.h>
#include <imep/imep_spec.h>

// ======================================================================
// ======================================================================

#define IMEP_HDR_LEN	512

#define BEACON_HDR_LEN	(IP_HDR_LEN + sizeof(struct hdr_imep))

#define HELLO_HDR_LEN	(IP_HDR_LEN + sizeof(struct hdr_imep) + \
			sizeof(struct imep_hello_block))

#define ACK_HDR_LEN	(IP_HDR_LEN + sizeof(struct hdr_imep) + \
			sizeof(struct imep_ack_block))

#define INT32_T(X)	*((int32_t*) &(X))
#define U_INT16_T(X)	*((u_int16_t*) &(X))
#define U_INT32_T(X)	*((u_int32_t*) &(X))

// ======================================================================
// ======================================================================

class imepAgent;

enum imepTimerType {
	BEACON_TIMER = 0x01,
	CONTROL_TIMER = 0x02,
	REXMIT_TIMER = 0x03,
	INCOMING_TIMER = 0x04,
};

class imepTimer : public Handler {
public:
        imepTimer(imepAgent* a, imepTimerType t) : busy_(0), agent(a), type_(t) {}
        void	handle(Event*);
	void	start(double time);
	void	cancel();
	int	busy() { return busy_; }
        double  timeLeft();
private:
	Event		intr;
	int		busy_;
        imepAgent	*agent;
	imepTimerType	type_;
};

// ======================================================================
// ======================================================================

class imepLink {
	friend class imepAgent; // so the LIST macros work
public:
	imepLink(nsaddr_t index) : index_(index), last_echo_(-BEACON_PERIOD) {}

	nsaddr_t&	index() { return index_; }
	u_int32_t&  	status() { return status_;}
	u_int8_t&       lastSeq() { return last_seq_;}
	u_int8_t&       lastSeqValid() { return last_seq_;}
	double&		in_expire() { return in_expire_; }
	double&		out_expire() { return out_expire_; }
	double&         lastEcho() { return last_echo_; } 

protected:
	LIST_ENTRY(imepLink) link;

private:
	nsaddr_t	index_;
	u_int32_t	status_;
	double      	in_expire_;
	double		out_expire_;
	double          last_echo_;
        u_int8_t        last_seq_;  // greatest seq heard from index_
        bool            last_seq_valid;
};

// ======================================================================
// ======================================================================

class imepAgent : public Agent {
	friend class imepTimer;
 public:
	imepAgent(nsaddr_t index);

	void recv(Packet *p, Handler *h);
	int command(int argc, const char*const* argv);

	// ============================================================
	// The IMEP API (imep-api.cc)

	void imepRegister(rtAgent *rt);

	void imepGetLinkStatus(nsaddr_t index, u_int32_t& status);
	void imepSetLinkInStatus(nsaddr_t index);
	void imepSetLinkOutStatus(nsaddr_t index);
	void imepSetLinkBiStatus(nsaddr_t index);
	void imepSetLinkDownStatus(nsaddr_t index);

	void imepPacketUndeliverable(Packet *p);
	// called by the MAC layer when it is unable to deliver
	// a packet to its next hop.

 	void imepGetBiLinks(int*& nblist, int& nbcnt);
	// called by the routing layer to get a complete list of all
	// neighbors.

        ImepStat stats;

 protected:
        void Terminate();

	// ============================================================
	// Timer stuff
	void handlerTimer(imepTimerType t);

 private:
	imepTimer		beaconTimer;
	imepTimer		controlTimer;
	imepTimer		rexmitTimer;
	imepTimer		incomingTimer;

	void handlerBeaconTimer(void);
	void handlerControlTimer(void);
	void handlerReXmitTimer(void);
	void handlerIncomingTimer(void);
	// When this timer fires, holes in the sequence space
	// are ignored and objects are delivered to the upper
	// layers.

	// ============================================================
	// Packet processing functions

	void recv_incoming(Packet *p);
	void recv_outgoing(Packet *p);

	void imep_beacon_input(Packet *p);
	void imep_ack_input(Packet *p);
	void imep_hello_input(Packet *p);
	void imep_object_input(Packet *p);
        void imep_object_process(Packet *p);
        void imep_ack_object(Packet *p);

	void imep_input(Packet *p);
	void imep_output(Packet *p);

	void sendBeacon(void);
	void sendHello(nsaddr_t index);
	void sendAck(nsaddr_t index, u_int32_t seqno);
        
	// ============================================================
	// Utility routines

	imepLink* findLink(nsaddr_t index);

	void purgeLink(void);
	// purge all expired links from the cache.

        void purgeReXmitQ(nsaddr_t index);
        // remove index from any response lists in the rexmit q

	Packet* findObjectSequence(u_int8_t seqno);
	// find the packet corresponding to sequence number "seqno".

	void removeObjectResponse(Packet *p, nsaddr_t index);
	// remove IP address "index" from the object response list of "p".

	struct imep_ack_block* findAckBlock(Packet *p);
	struct imep_hello_block* findHelloBlock(Packet *p);
	struct imep_object_block* findObjectBlock(Packet *p);
	struct imep_response* findResponseList(Packet *p);

	void aggregateAckBlock(Packet *p);
	void aggregateHelloBlock(Packet *p);
	void aggregateObjectBlock(Packet *p);
	void createResponseList(Packet *p);
        int getResponseListSize();

	void scheduleReXmit(Packet *p);
	void scheduleIncoming(Packet *p, u_int32_t s);

	u_int8_t nextSequence() {
		controlSequence++;
		return (controlSequence & 0xFF);
	}

	inline int initialized() {
		return recvtarget_ && sendtarget_ && rtagent_ && logtarget_;
	}

	void imep_dump_header(Packet *p);
public:
	void trace(char* fmt, ...);
private:

	void log_neighbor_list(void);
        char* dumpResponseList(Packet *p);

	// ============================================================
	// Routing Protocol Specific Routines

	void toraCreateHeader(Packet *p, char* dst, int length);
	void toraExtractHeader(Packet *p, char* dst);
	int toraHeaderLength(struct hdr_tora* t);

	// ============================================================

	nsaddr_t	ipaddr;
	// IP Address of this node.

	int		off_IMEP_;
	// offset of the IMEP header in a packet.

	int off_TORA_;
	// offset of the TORA header in a packet.

	u_int32_t	controlSequence;

	NsObject	*recvtarget_;
	// classifier

	NsObject	*sendtarget_;
	// The link layer below the IMEP layer.

	rtAgent		*rtagent_;
	// pointer to the "upper layer" routing agent.

	Trace		*logtarget_;
	// allows the IMEP agent to write info to the log file.

	LIST_HEAD(,imepLink) imepLinkHead;
	// List of links to neighbors and corresponding status

	// ============================================================
	// Awaiting Broadcast (AB) Buffer
	PacketQueue ackQueue;
	PacketQueue helloQueue;
	PacketQueue objectQueue;

	// ============================================================
	// Waiting for ACK Buffer
	ReXmitQ rexmitq;

        // ============================================================
	// Waiting for delivery to the upper layer Buffer
	dstQueue incomingQ;
};

#endif /* __imep_h__ */

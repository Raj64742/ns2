/* -*- c++ -*-
   rexmit_queue.h
   $Id: dest_queue.h,v 1.1 1999/08/03 04:12:34 yaxu Exp $
   */

#ifndef __dest_queue_h__
#define __dest_queue_h__

#include <packet.h>
#include <list.h>

#define ILLEGAL_SEQ 257

typedef double Time;

class txent;
class dstent;
class imepAgent;

//////////////////////////////////////////////////////////////////////
// Transmit Queue Entry
class txent {
	friend class dstent;
	friend class dstQueue;
protected:
	txent(double e, u_int32_t s, Packet *p);

	LIST_ENTRY(txent) link;
	// public so that I can use the list macros

	double expire() { return expire_; }
	u_int32_t seqno() { return seqno_; }
	Packet *pkt() { return pkt_; }

private:
	double expire_;		// time "p" expires and must be sent
	u_int32_t seqno_;	// sequence number of the packet "p".
	Packet *pkt_;
};

LIST_HEAD(txent_head, txent);

//////////////////////////////////////////////////////////////////////
// Destination Queue Entry
class dstent {
	friend class dstQueue;
protected:
	dstent(nsaddr_t index);

	LIST_ENTRY(dstent) link;

	void addEntry(double e, u_int32_t s, Packet *p);
	// add's a packet to the txentHead list.

	void delEntry(txent *t);
	// remove's a packet (transmit entry) from the txentHead list.

	txent* findEntry(u_int32_t s);
	// locates a packet with sequence "s" in the txentHead list.
	txent* findFirstEntry(void);

	nsaddr_t ipaddr() { return ipaddr_; }
	u_int32_t& seqno() { return seqno_; }

	inline double expire() {
		txent *t;
		double min = 0.0;

		for(t = txentHead.lh_first; t; t = t->link.le_next) {
			if(min == 0.0 || (t->expire() && t->expire() < min))
				min = t->expire();
		}
		return min;
	}
private:
	nsaddr_t ipaddr_;	// ip address of this destination
	txent_head txentHead;	// sorted by sequence number
	u_int32_t seqno_;	// last sequence number sent up the stack
};

LIST_HEAD(dstend_head, dstent);

//////////////////////////////////////////////////////////////////////
// Destination Queue
class dstQueue {
public:
	dstQueue(imepAgent *a, nsaddr_t index);

	void addEntry(nsaddr_t dst, double e, u_int32_t s, Packet *p);
	// add's a packet to the list.

	Packet* getPacket(nsaddr_t dst, u_int32_t seqno);
	// returns the packet with the following sequence nubmer "seqno".`
        // removes packet from queue

	Time getNextExpire();
	// returns the time that the next packet will expire.

	Packet* getNextPacket(u_int32_t& seqno);
	// returns "expired" packets (in sequence order, if any).

        void deleteDst(nsaddr_t dst);
        // remove and free all packets from dst

	void dumpAll(void);

private:
	dstent* findEntry(nsaddr_t dst);
	
	dstend_head dstentHead;		// head of the destination list
	imepAgent *agent_;
	nsaddr_t ipaddr_;
};

#endif // __dest_queue_h__

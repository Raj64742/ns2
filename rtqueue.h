/* -*- c++ -*-
   $Id: rtqueue.h,v 1.2 1999/09/09 03:25:24 salehi Exp $
*/

#ifndef __ifqueue_h__
#define __ifqueue_h_

#include <packet.h>
#include <agent.h>

/*
 * The maximum number of packets that we allow a routing protocol to buffer.
 */
#define RTQ_MAX_LEN     64      // packets

/*
 *  The maximum period of time that a routing protocol is allowed to buffer
 *  a packet for.
 */
#define RTQ_TIMEOUT     30	// seconds

class rtqueue : public Connector {
 public:
        rtqueue();

        void            recv(Packet *, Handler*) { abort(); }

        void            enque(Packet *p);

	inline int      command(int argc, const char * const* argv) 
	  { return Connector::command(argc, argv); }

        /*
         *  Returns a packet from the head of the queue.
         */
        Packet*         deque(void);

        /*
         * Returns a packet for destination "D".
         */
        Packet*         deque(nsaddr_t dst);

 private:
        Packet*         remove_head();
        void            purge(void);
	void		findPacketWithDst(nsaddr_t dst, Packet*& p, 
					  Packet*& prev);
	void		verifyQueue(void);

        Packet          *head_;
        Packet          *tail_;

        int             len_;

        int             limit_;
        double          timeout_;

};

#endif /* __ifqueue_h__ */

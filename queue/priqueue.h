/* -*- c++ -*-
   priqueue.h
   
   A simple priority queue with a remove packet function
*/
#ifndef _priqueue_h
#define _priqueue_h

#include <object.h>
#include <queue.h>
#include <drop-tail.h>
#include <packet.h>
#include <list.h>

class PriQueue;
typedef int (*PacketFilter)(Packet *, void *);

LIST_HEAD(PriQueue_List, PriQueue);

class PriQueue : public DropTail {
public:
        PriQueue();

        int     command(int argc, const char*const* argv);
        void    recv(Packet *p, Handler *h);

        void    recvHighPriority(Packet *, Handler *);
        // insert packet at front of queue

        void filter(PacketFilter filter, void * data);
        // apply filter to each packet in queue, 
        // - if filter returns 0 leave packet in queue
        // - if filter returns 1 remove packet from queue

        Packet* filter(nsaddr_t id);

	void	Terminate(void);
private:
        int Prefer_Routing_Protocols;
 
	/*
	 * A global list of Interface Queues.  I use this list to iterate
	 * over all of the queues at the end of the simulation and flush
	 * their contents. - josh
	 */
public:
	LIST_ENTRY(PriQueue) link;
	static struct PriQueue_List prhead;
};

#endif _priqueue_h

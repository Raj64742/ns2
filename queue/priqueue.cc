/* -*- c++ -*-
   priqueue.cc
   
   A simple priority queue with a remove packet function
   $Id: priqueue.cc,v 1.1 1998/12/08 19:14:18 haldar Exp $
   */

#include <object.h>
#include <queue.h>
#include <drop-tail.h>
#include <packet.h>
//#include <cmu/cmu-trace.h>

#include "priqueue.h"

typedef int (*PacketFilter)(Packet *, void *);

PriQueue_List PriQueue::prhead = { 0 };

static class PriQueueClass : public TclClass {
public:
  PriQueueClass() : TclClass("Queue/DropTail/PriQueue") {}
  TclObject* create(int, const char*const*) {
    return (new PriQueue);
  }
} class_PriQueue;


PriQueue::PriQueue() : DropTail()
{
        bind("Prefer_Routing_Protocols", &Prefer_Routing_Protocols);
	LIST_INSERT_HEAD(&prhead, this, link);
}

int
PriQueue::command(int argc, const char*const* argv)
{
  if (argc == 2 && strcasecmp(argv[1], "reset") == 0)
    {
      Terminate();
      //FALL-THROUGH to give parents a chance to reset
    }
  return DropTail::command(argc, argv);
}

void
PriQueue::recv(Packet *p, Handler *h)
{
        struct hdr_cmn *ch = HDR_CMN(p);

        if(Prefer_Routing_Protocols) {

                switch(ch->ptype()) {
		case PT_DSR:
		case PT_MESSAGE:
                case PT_TORA:
                case PT_AODV:
                        recvHighPriority(p, h);
                        break;

                default:
                        Queue::recv(p, h);
                }
        }
        else {
                Queue::recv(p, h);
        }
}


void 
PriQueue::recvHighPriority(Packet *p, Handler *)
  // insert packet at front of queue
{
	q_->enqueHead(p);
	if (q_->length() >= qlim_)
    {
      Packet *to_drop = q_->lookup(q_->length()-1);
      q_->remove(to_drop);
      drop(to_drop);
    }
  
  if (!blocked_) {
    /*
     * We're not blocked.  Get a packet and send it on.
     * We perform an extra check because the queue
     * might drop the packet even if it was
     * previously empty!  (e.g., RED can do this.)
     */
    p = deque();
    if (p != 0) {
      blocked_ = 1;
      target_->recv(p, &qh_);
    }
  } 
}
 
void 
PriQueue::filter(PacketFilter filter, void * data)
  // apply filter to each packet in queue, 
  // - if filter returns 0 leave packet in queue
  // - if filter returns 1 remove packet from queue
{
  int i = 0;
  while (i < q_->length())
    {
      Packet *p = q_->lookup(i);
      if (filter(p,data))
	{
	  q_->remove(p); // decrements q len
	}
      else i++;
    }
}

Packet*
PriQueue::filter(nsaddr_t id)
{
	Packet *p = 0;
	Packet *pp = 0;
	struct hdr_cmn *ch;

	for(p = q_->head(); p; p = p->next_) {
		ch = HDR_CMN(p);
		if(ch->next_hop() == id)
			break;
		pp = p;
	}

	/*
	 * Deque Packet
	 */
	if(p) {
		if(pp == 0)
			q_->remove(p);
		else
			q_->remove(p, pp);
	}
	return p;
}

/*
 * Called at the end of the simulation to purge the IFQ.
 */
void
PriQueue::Terminate()
{
	Packet *p;
	while((p = deque())) {
		//drop(p, DROP_END_OF_SIMULATION);
		drop(p);
		
	}
}





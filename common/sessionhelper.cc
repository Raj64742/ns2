/*
 * sessionhelper.cc
 * Copyright (C) 1997 by USC/ISI
 * All rights reserved.                                            
 *                                                                
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 * 
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/sessionhelper.cc,v 1.3 1997/08/11 17:39:38 polly Exp $ (USC/ISI)";
#endif

#include "packet.h"

struct dstobj {
  double bw;
  double delay;
  nsaddr_t addr;
  NsObject *obj;
  dstobj *next;
};
  

class SessionHelper : public Connector {
 public:
	SessionHelper();
	int command(int, const char*const*);
	void recv(Packet*, Handler*);
 protected:
        nsaddr_t src_;
        dstobj *dstobj_;
};

static class SessionHelperClass : public TclClass {
public:
	SessionHelperClass() : TclClass("SessionHelper") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new SessionHelper());
	}
} class_sessionhelper;

SessionHelper::SessionHelper() : dstobj_(0)
{
}

void SessionHelper::recv(Packet* pkt, Handler*)
{
  dstobj *tmpdst = dstobj_;
  Scheduler& s = Scheduler::instance();
  hdr_cmn* th = (hdr_cmn*)pkt->access(off_cmn_);
  //printf (" size %d\n", th->size());

  while (tmpdst != 0) {
    //printf ("%d ", tmpdst->addr);
    if (tmpdst->bw == 0) {
      s.schedule(tmpdst->obj, pkt->copy(), tmpdst->delay);
    } else {
      //printf("schedule delay %f\n", th->size()/bw + delay);
      s.schedule(tmpdst->obj, pkt->copy(), th->size()/tmpdst->bw + tmpdst->delay);
    }
    tmpdst = tmpdst->next;
  }
  Packet::free(pkt);

}

/*
 * $proc handler $handler
 * $proc send $msg
 */
int SessionHelper::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "set-node") == 0) {
		        int src = atoi(argv[2]);
                        src_ = src;
			//printf("set node %d\n", src_);
			return (TCL_OK);
		}
	} else if (argc == 6) {
	  if (strcmp(argv[1], "add-dst") == 0) {
	    dstobj *tmp = new dstobj;
	    tmp->bw = atof(argv[2]);
	    tmp->delay = atof(argv[3]);
	    tmp->addr = atoi(argv[4]);
	    tmp->obj = (NsObject*)TclObject::lookup(argv[5]);
	    //printf ("addr = %d, argv3 = %s, obj = %d\n", tmp->addr, argv[3], tmp->obj);
	    tmp->next = dstobj_;
	    dstobj_ = tmp;
	    return (TCL_OK);
	  }
	}
	return (Connector::command(argc, argv));
}





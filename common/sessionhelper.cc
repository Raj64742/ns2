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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/sessionhelper.cc,v 1.5 1997/11/04 00:13:21 polly Exp $ (USC/ISI)";
#endif

#include "Tcl.h"
#include "ip.h"
#include "packet.h"
#include "connector.h"
#include "errmodel.h"

struct dstobj {
	double bw;
	double delay;
        int ttl;
        int dropped;
	nsaddr_t addr;
	NsObject *obj;
	dstobj *next;
};

struct rcv_depobj {
	dstobj *obj;
	rcv_depobj *next;
};

  
struct loss_depobj {
	ErrorModel *obj;
        loss_depobj *loss_dep;
        rcv_depobj *rcv_dep;
	loss_depobj *next;
};

class SessionHelper : public Connector {
public:
	SessionHelper();
	int command(int, const char*const*);
	void recv(Packet*, Handler*);
protected:
        void get_dropped(loss_depobj*, Packet*);
        void mark_dropped(loss_depobj*);
        void clear_dropped();
        dstobj* find_dstobj(NsObject*);
        loss_depobj* find_loss_depobj(ErrorModel*);
        void show_dstobj();
        void show_loss_depobj(loss_depobj*);
        nsaddr_t src_;
        dstobj *dstobj_;
        loss_depobj *loss_dependency_;
        int off_ip_;
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
        bind("off_ip_", &off_ip_);
	loss_dependency_ = new loss_depobj;
	loss_dependency_->obj = 0;
	loss_dependency_->loss_dep = 0;
	loss_dependency_->rcv_dep = 0;
	loss_dependency_->next = 0;
}

void SessionHelper::recv(Packet* pkt, Handler*)
{
	dstobj *tmpdst = dstobj_;
	Scheduler& s = Scheduler::instance();
	hdr_cmn* th = (hdr_cmn*)pkt->access(off_cmn_);
	hdr_ip* iph = (hdr_ip*)pkt->access(off_ip_);

	//printf (" size %d\n", th->size());

	get_dropped(loss_dependency_->loss_dep, pkt);

	while (tmpdst != 0) {
	  if (!tmpdst->dropped) {
                int ttl = iph->ttl() - tmpdst->ttl;
                if (ttl > 0) {
		  Packet* tmppkt = pkt->copy();
		  hdr_ip* tmpiph = (hdr_ip*)tmppkt->access(off_ip_);
		  tmpiph->ttl() = ttl;
		  //printf ("ttl=%d\n", tmpiph->ttl());
		  //printf ("%d ", tmpdst->addr);
		  if (tmpdst->bw == 0) {
		    s.schedule(tmpdst->obj, tmppkt, tmpdst->delay);
		  } else {
		    //printf("schedule delay %f\n", th->size()/bw + delay);
		    s.schedule(tmpdst->obj, tmppkt, th->size()*8/tmpdst->bw + tmpdst->delay);
		  }
		} else {
		  //printf("ttl exceeded\n");
		}
	  }
	  tmpdst = tmpdst->next;
	}
	clear_dropped();
	Packet::free(pkt);

}

void SessionHelper::get_dropped(loss_depobj* loss_dep, Packet* pkt)
{
  if (loss_dep != 0) 
    if (loss_dep->obj != 0) {
      if (loss_dep->obj->corrupt(pkt)) {
	mark_dropped(loss_dep);
      } else {
	get_dropped(loss_dep->loss_dep, pkt);
      }
      get_dropped(loss_dep->next, pkt);
    }
}

void SessionHelper::mark_dropped(loss_depobj* loss_dep)
{
  if (loss_dep != 0) {
    rcv_depobj *tmprcv_dep = loss_dep->rcv_dep;
    loss_depobj *tmploss_dep = loss_dep->loss_dep;

    while (tmprcv_dep != 0) {
      tmprcv_dep->obj->dropped = 1;
      tmprcv_dep = tmprcv_dep->next;
    }

    while (tmploss_dep != 0) {
      mark_dropped(tmploss_dep);
      tmploss_dep = tmploss_dep->next;
    }
  }
}

void SessionHelper::clear_dropped()
{
	dstobj *tmpdst = dstobj_;
	while (tmpdst != 0) {
	        tmpdst->dropped = 0;
		tmpdst = tmpdst->next;
	}
}

dstobj* SessionHelper::find_dstobj(NsObject* obj) {
	dstobj *tmpdst = dstobj_;
	while (tmpdst != 0) {
	  if (tmpdst->obj == obj) return (tmpdst);
	  tmpdst = tmpdst->next;
	}
	return 0;
}

loss_depobj* SessionHelper::find_loss_depobj(ErrorModel* err) {
	struct stackobj {
	  loss_depobj *loss_obj;
	  stackobj *next;
	};

	if (!loss_dependency_) return 0;

        stackobj *top = new stackobj;
	top->loss_obj = loss_dependency_;
	top->next = 0;

	while (top != 0) {
	  if (top->loss_obj->obj == err) return (top->loss_obj);
	  loss_depobj *tmploss = top->loss_obj->loss_dep;
	  stackobj *befreed = top;
	  top = top->next;
	  free(befreed);
	  while (tmploss != 0) {
	    stackobj *new_element = new stackobj;
	    new_element->loss_obj = tmploss;
	    new_element->next = top;
	    top = new_element;
	    tmploss = tmploss->next;
	  }
	}
	return 0;
}

void SessionHelper::show_dstobj() {
	dstobj *tmpdst = dstobj_;
	while (tmpdst != 0) {
	  printf("bw:%.2f, delay:%.2f, ttl:%d, dropped:%d, addr:%d, obj:%s\n", tmpdst->bw, tmpdst->delay, tmpdst->ttl, tmpdst->dropped, tmpdst->addr, tmpdst->obj->name());
	  tmpdst = tmpdst->next;
	}
}

void SessionHelper::show_loss_depobj(loss_depobj *loss_obj) {

	loss_depobj *tmploss = loss_obj->loss_dep;
	rcv_depobj *tmprcv = loss_obj->rcv_dep;

	while (tmprcv != 0) {
	  printf("%d ", tmprcv->obj->addr);
	  tmprcv = tmprcv->next;
	}
	while (tmploss != 0) {
	  printf("(%s: ", tmploss->obj->name());
	  show_loss_depobj(tmploss);
	  tmploss = tmploss->next;
	}

	if (loss_obj == loss_dependency_) {
	  printf("\n");
	} else {
	  printf(")");
	}
}


int SessionHelper::command(int argc, const char*const* argv)
{
        if (argc == 2) {
		if (strcmp(argv[1], "show-loss-depobj") == 0) {
                        show_loss_depobj(loss_dependency_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "show-dstobj") == 0) {
                        show_dstobj();
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "set-node") == 0) {
		        int src = atoi(argv[2]);
                        src_ = src;
			//printf("set node %d\n", src_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "update-loss-top") == 0) {
			loss_depobj *tmploss = (loss_depobj*)(atoi(argv[2]));
			tmploss->next = loss_dependency_->loss_dep;
			loss_dependency_->loss_dep = tmploss;
			return (TCL_OK);
		}
	} else if (argc == 4) {
	  Tcl& tcl = Tcl::instance();
		if (strcmp(argv[1], "update-loss-rcv") == 0) {
			ErrorModel *tmperr = (ErrorModel*)TclObject::lookup(argv[2]);
			NsObject *tmpobj = (NsObject*)TclObject::lookup(argv[3]);
			//printf("errmodel %s, agent %s\n", tmperr->name(), tmpobj->name());
			loss_depobj *tmploss = find_loss_depobj(tmperr);
			//printf ("%d, loss_dependency_ %d\n", tmploss, loss_dependency_);
			if (!tmploss) {
			  tmploss = new loss_depobj;
			  tmploss->obj = tmperr;
			  tmploss->next = 0;
			  tmploss->rcv_dep = 0;
			  tmploss->loss_dep = 0;
			  tcl.resultf("%d", tmploss);
			} else {
			  tcl.result("0");
			}
		        rcv_depobj *tmprcv = new rcv_depobj;
			tmprcv->obj = find_dstobj(tmpobj);
			tmprcv->next = tmploss->rcv_dep;
			tmploss->rcv_dep = tmprcv;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "update-loss-loss") == 0) {
			ErrorModel *tmperrparent = (ErrorModel*)TclObject::lookup(argv[2]);
			loss_depobj *tmplossparent = find_loss_depobj(tmperrparent);
			loss_depobj *tmplosschild = (loss_depobj*)(atoi(argv[3]));
			if (!tmplossparent) {
			  tmplossparent = new loss_depobj;
			  tmplossparent->obj = tmperrparent;
			  tmplossparent->next = 0;
			  tmplossparent->loss_dep = 0;
			  tmplossparent->rcv_dep = 0;
			  tcl.resultf("%d", tmplossparent);
			} else {
			  tcl.result("0");
			}
			tmplosschild->next = tmplossparent->loss_dep;
			tmplossparent->loss_dep = tmplosschild;
			return (TCL_OK);
		}
	} else if (argc == 7) {
		if (strcmp(argv[1], "add-dst") == 0) {
			dstobj *tmp = new dstobj;
			tmp->bw = atof(argv[2]);
			tmp->delay = atof(argv[3]);
			tmp->ttl = atoi(argv[4]);
			tmp->addr = atoi(argv[5]);
			tmp->obj = (NsObject*)TclObject::lookup(argv[6]);
			//printf ("addr = %d, argv3 = %s, obj = %d, ttl=%d\n", tmp->addr, argv[3], tmp->obj, tmp->ttl);
			tmp->next = dstobj_;
			dstobj_ = tmp;
			return (TCL_OK);
		}
	}
	return (Connector::command(argc, argv));
}


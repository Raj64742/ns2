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
	int command(int argc, const char*const* argv);
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
  static int tmp;
  //printf("recv pkt %d", tmp++);

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
	Tcl& tcl = Tcl::instance();
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






/* disclaimers go here */

#include "xcp.h"

static class XCPClass : public TclClass {
public:
  XCPClass() : TclClass("Queue/XCP") {}
  TclObject* create(int, const char*const*) {
    return (new XCPWrapQ);
  }
} class_xcp_queue;

XCPWrapQ::XCPWrapQ():xcpq_(NULL)
{
  bind("maxVirQ_", &maxVirQ_);
  routerId_ = next_router++;
}
 
void XCPWrapQ::setVirtualQueues() {

  for (int n=0; n < maxVirQ_; n++) {
    xcpq_[n]->routerId(routerId_); 
    wrrTemp_[n] = 0;
  }
  
  qToDq_ = 0;
  queueWeight_[XCPQ] = 0.5;
  queueWeight_[TCPQ] = 0.5;
  queueWeight_[OTHERQ] = 0;
  
  // setup timers for xcp queue only
  xcpq_[XCPQ]->setupTimer();
  
}


int XCPWrapQ::command(int argc, const char*const* argv)
{
  Tcl& tcl = Tcl::instance();

  // if (argc == 2) {
  //     if (strcmp(argv[1], "get-xcpq") == 0) {
  //       tcl.result((&xcpq_)->name());
  //       return TCL_OK;
  //     }
  //   }
  
  if (argc >= 3) {	
    if (strcmp(argv[1], "set-virQ") == 0) {
      xcpq_ = new XCPQueue*[maxVirQ_]; 
      for (int n=0,c=2; n < maxVirQ_; n++) {
	xcpq_[n] = (XCPQueue *)(TclObject::lookup(argv[c++]));
	if (xcpq_[n] == NULL) {
	  tcl.add_errorf("Wrong xcp virtual queues %s\n",argv[c-1]);
	  return TCL_ERROR;
	}
      }
      setVirtualQueues();
      return TCL_OK;
    }

    if (strcmp(argv[1], "set-link-capacity-Kbytes") == 0) {
      double link_capacity_Kbytes = strtod(argv[2],0)/2;
      
      if (link_capacity_Kbytes < 0.0) 
	{printf("Error: BW < 0"); abort();};

      for (int n=0; n < MAX_QNUM; n++) {
	xcpq_[n]->setBW(link_capacity_Kbytes); // ?????
	xcpq_[n]->limit(limit());
	xcpq_[n]->config();
      }
      return TCL_OK;
    }
    
    if (strcmp(argv[1], "attach") == 0) {
      
      int mode;
      const char* id = argv[2];
      queue_trace_file_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);

      if (queue_trace_file_ == 0) {
	tcl.resultf("queue.cc: trace-drops: can't attach %s for writing", id);
	return (TCL_ERROR);
      }

      for (int n=0; n < MAX_QNUM; n++)
	xcpq_[n]->setChannel(queue_trace_file_);
    
      return (TCL_OK);
    }
  }
  return (Queue::command(argc, argv));
}


void XCPWrapQ::recv(Packet* p, Handler* h)
{
  mark(p);
  total_packet_arrivals_ = total_packet_arrivals_ + 1;

  enque(p);
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
      deliver_to_target(p);
    }
  }
}


void XCPWrapQ::deliver_to_target(Packet* p)
{
  output_traffic_ += (8. * hdr_cmn::access(p)->size());
  target_->recv(p, &qh_);
}


/*
void XCPWrapQ::everyRTT ()
{
 char wrk[500];
 double t = Scheduler::instance().clock();

	// Update drops
	if(trace_drops_ && queue_trace_file_){
		int n;	
		sprintf(wrk, "d %g %d", t, drops_);
		n = strlen(wrk);
		wrk[n] = '\n'; 
		wrk[n+1] = 0;
		(void)Tcl_Write(queue_trace_file_, wrk, n+1);
		drops_=0;
	}

	// Update sample the current queue size
	if(trace_curq_ && queue_trace_file_){
		sprintf(wrk, "c  %g %d", t,length());
		int n = strlen(wrk);
		wrk[n] = '\n'; 
		wrk[n+1] = 0;
		(void)Tcl_Write(queue_trace_file_, wrk, n+1);
	}

	// Update utilization 
	if((output_link_capacity_ > 0) && queue_trace_file_){
		utilization_ = output_traffic_ /(output_link_capacity_ * effective_rtt_) ;
		sprintf(wrk, "u  %g %g", t, utilization_);
		int n = strlen(wrk);
		wrk[n] = '\n'; 
		wrk[n+1] = 0;
		(void)Tcl_Write(queue_trace_file_, wrk, n+1);
		output_traffic_ = 0;
	}
	rtt_timer_.resched(effective_rtt_);
}
*/

/*
void XCPWrapQ::trace(TracedVar* v)
{
  char wrk[500], *p;

  if (((p = strstr(v->name(), "ave")) == NULL) &&
      ((p = strstr(v->name(), "prob")) == NULL) &&
      ((p = strstr(v->name(), "curq")) == NULL) &&
      ((p = strstr(v->name(), "cur_max_p"))==NULL) ) {
    fprintf(stderr, "RED:unknown trace var %s\n",
	    v->name());
    return;
  }

  if (tchan_) {
    int n;
    double t = Scheduler::instance().clock();
		// XXX: be compatible with nsv1 RED trace entries
    if (strstr(v->name(), "curq") != NULL) {
      sprintf(wrk, "Q %g %d", t, int(*((TracedInt*) v)));
    } else {
      sprintf(wrk, "%c %g %g", *p, t,
	      double(*((TracedDouble*) v)));
    }
    n = strlen(wrk);
    wrk[n] = '\n'; 
    wrk[n+1] = 0;
    (void)Tcl_Write(tchan_, wrk, n+1);
  }
  return; 
}
*/

void XCPWrapQ::trace_var(char * var_name, double var)
{
  char wrk[500];
  double now = Scheduler::instance().clock();

  if (queue_trace_file_) {
    int n;
    sprintf(wrk, "%s %g %g",var_name, now, var);
    n = strlen(wrk);
    wrk[n] = '\n'; 
    wrk[n+1] = 0;
    (void)Tcl_Write(queue_trace_file_, wrk, n+1);
  }
  return; 
}

void XCPWrapQ::addQueueWeights(int queueNum, int weight) {
  if (queueNum < MAX_QNUM)
    queueWeight_[queueNum] = weight;
  else {
    fprintf(stderr, "Queue number is out of range.\n");
  }
}

int XCPWrapQ::queueToDeque()
{
  int i = 0;
  
  if (wrrTemp_[qToDq_] <= 0) {
    qToDq_ = ((qToDq_ + 1) % maxVirQ_);
    wrrTemp_[qToDq_] = queueWeight_[qToDq_] - 1;
  } else {
    wrrTemp_[qToDq_] = wrrTemp_[qToDq_] -1;
  }
  while ((i < maxVirQ_) && (xcpq_[qToDq_]->length() == 0)) {
    wrrTemp_[qToDq_] = 0;
    qToDq_ = ((qToDq_ + 1) % maxVirQ_);
    wrrTemp_[qToDq_] = queueWeight_[qToDq_] - 1;
    i++;
  }
  return (qToDq_);
}

int XCPWrapQ::queueToEnque(int cp)
{
  int n;
  switch (cp) {
  case CP_XCP:
    return (n = XCPQ);
    
  case CP_TCP:
    return (n = TCPQ);
    
  case CP_OTHER:
    return (n = OTHERQ);

  default:
    fprintf(stderr, "Unknown codepoint %d\n", cp);
    exit(1);
  }
}


// Extracts the code point marking from packet header.
int XCPWrapQ::getCodePt(Packet *p) {

  hdr_ip* iph = hdr_ip::access(p);
  return(iph->prio());
}

Packet* XCPWrapQ::deque()
{
  Packet *p = NULL;
  int n = queueToDeque();
  // Deque a packet from the underlying queue:
  p = xcpq_[n]->deque();
  return(p);
}


void XCPWrapQ::enque(Packet* pkt)
{
  int codePt;
  
  codePt = getCodePt(pkt);
  int n = queueToEnque(codePt);
  
  xcpq_[n]->enque(pkt);
}

int XCPWrapQ::mark(Packet *p) {
  
  int codePt;
  hdr_cmn* cmnh = hdr_cmn::access(p);
  hdr_cctcp *cctcph = hdr_cctcp::access(p);
  hdr_ip *iph = hdr_ip::access(p);

  if ((codePt = iph->prio_) > 0)
    return codePt;

  else {
    if (cctcph->ccenabled_ > 0) 
      codePt = CP_XCP;
    else 
      if (cmnh->ptype() == PT_TCP || cmnh->ptype() == PT_ACK)
	codePt = CP_TCP;
      else // for others
	codePt = CP_OTHER;
    
    iph->prio_ = codePt;
    return codePt;
  }
}
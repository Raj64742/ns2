/* This is the port of the initial SRM implementation in ns-1 by
   Kannan Varadahan <kannan@isi.edu>. Most of the code is in C++. Since
   then, there has been another implementation. The new implemenatation
   has better c++/tcl separation. Check later releases for new
   implementation. This code has been checked in for archival reasons.
   -Puneet Sharma <puneet@isi.edu>
*/

/* TODO :

   1. Add stop function for the agent and sender
   2. Add beter logging support
   3. Add support for adaptive session messages period
   4. Rearrange for better c++/tcl separation
   3. Local Revcovery
   4. Due to new packet format the class field is missing, so currently
   it does not distinguish various srm packets.
   N. Clean-up temporary hacks, add comments

*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "Tcl.h"
#include "agent.h"
#include "packet.h"
#include "ip.h"
#include "cbr.h"
#include "random.h"
#include "srm.h"


static class SRMHeaderClass : public PacketHeaderClass {
public:
        SRMHeaderClass() : PacketHeaderClass("PacketHeader/SRM",
					     sizeof(hdr_srm)) {}
} class_srmhdr;


static class SRMAgentClass : public TclClass {
public:
	SRMAgentClass() : TclClass("Agent/SRM") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new SRMAgent());
	}
} class_srm_agent;

static class SRMSourceClass : public TclClass {
public:
	SRMSourceClass() : TclClass("SRMSource") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new SRMSource(atoi(argv[4])));
	}
} class_srm_source;

/* SRM event class */
class srmEvent:public Event{
 public:
  srmEvent(SRMAgent*v,int t,int s,int i)
    :ag_(v),msgtype_(t),sender_(s),msgid_(i){}
  SRMAgent*ag_;
  int msgtype_;
  int sender_;
  int msgid_;
};



static class srmEventHandler:public Handler{
 public:
  void handle(Event*e){
    srmEvent*sev= (srmEvent*)e;
    switch(sev->msgtype_){
    case SRM_DATA:
      sev->ag_->send_data();
      break;
    case SRM_RQST:
      sev->ag_->send_rqst(sev->sender_,sev->msgid_);
      break;
    case SRM_REPR:
      sev->ag_->send_repr(sev->sender_,sev->msgid_);
      break;
    case SRM_SESS:
      /* Session messages are currently not handled as srmevents */
      break;
    default:
      
      break;
    }
    delete e;
  }
}srmEvent_handler;


SRMSource::SRMSource(u_int32_t srcid): next_(0)
{
}


/* Returns pointer to the message info block for given msg */
msg_info* SRMSource::get_msg_info(int msgid)
{
  msg_info**mp;
  
  if(!cache_||cache_->id_!=msgid){
    for(mp= &messages_;*mp&&(*mp)->id_<msgid;mp= &(*mp)->next_)
      ;
    if(!*mp||(*mp)->id_!=msgid){
      msg_info*mnew= new msg_info;
      mnew->sender_= this;
      mnew->id_= msgid;
      mnew->status_= SRM_UNKNOWN;
      mnew->backoff_const_= 1;
      mnew->ignore_backoff_ = 0;
      mnew->ev_= 0;
      mnew->next_= *mp;
      *mp= mnew;
    }
  }else{
    mp= &cache_;
  }
  return cache_= *mp;
}


/* Returns status of the message */
int SRMSource::status(int msgid){
  msg_info*m= get_msg_info(msgid);
  return m->status_;
}

/* Changes the status of the message */
void SRMSource::status(int msgid,int status){
  msg_info*m= get_msg_info(msgid);
  m->status_= status;
}

/* Set event related to the message */
void SRMSource::set_event(int msgid,Event*ev){
  msg_info*m= get_msg_info(msgid);
  m->ev_= ev;
}

/* Set the requesting/repair source for the message */
void SRMSource::set_rsrc(int msgid, int r_src) {
  msg_info*m= get_msg_info(msgid);
  m->r_src_= r_src;
}

/* Return the requesting/repair source for the message */
int SRMSource::rsrc(int msgid) {
  msg_info*m= get_msg_info(msgid);
  return m->r_src_;
}

/* Cancel any event scheduled for this message */
void SRMSource::cancel_event(int msgid){

  msg_info*m= get_msg_info(msgid);
  /* If there is some event and it is scheduled at a later time, cancel it */
  if (m && m->ev_ && (m->ev_->time_ > NOW)) {
    Scheduler::instance().cancel(m->ev_) ;
  }
  m->ev_= 0;
}

/* Returns the backoff constant */
int SRMSource::backoff(int msgid){
  msg_info*m= get_msg_info(msgid);
  return m->backoff_const_;
}

/* Reset the backoff constant */
void SRMSource::reset_backoff(int msgid){
  msg_info*m= get_msg_info(msgid);
  m->backoff_const_= 1;
  m->ignore_backoff_= 0;
}	

/* Increment the backoff constant */
void SRMSource::incr_backoff(int msgid){
  msg_info*m= get_msg_info(msgid);
  m->backoff_const_*= 2;
}

/* Return the ignore/backoff timer value */
double SRMSource::backoff_timer(int msgid){
  msg_info*m= get_msg_info(msgid);
  return m->ignore_backoff_;
}

/* Set the ignore/backoff timer value */
void SRMSource::backoff_timer(int msgid,double t){
  msg_info*m= get_msg_info(msgid);
  m->ignore_backoff_= NOW + t;
}

/* Update the status of the various messages */
void SRMSource::update_vars()
{
  double now= NOW;
  msg_info**mp= &messages_;

  while(*mp&&(*mp)->id_<=received_+1){
    msg_info*m= *mp;
    switch(m->status_){
    case SRM_REPR_PENDING:
      assert(m->ev_);
      break;
    case SRM_REPR_WAIT:
      assert(m->ignore_backoff_);
      if(m->ignore_backoff_<now)
	m->ignore_backoff_= 0;
    case SRM_DATA_RECEIVED:
      break;
    default:
      assert(m->ev_);
      return;
    }
    if(m->id_==received_+1)
      received_++;
    if(m->ev_==0&&m->ignore_backoff_==0){
      if(cache_==m)
	cache_= 0;
      *mp= m->next_;
      delete m;
    }else{
      mp= &m->next_;
    }
  }
}


/* Get the state of a particular sender, if no state then create relevant state */
SRMSource *SRMAgent::get_sender_state(int sender){
  SRMSource *snp;
  for(snp= allsrcs_;snp;snp= snp->next_)
    if(snp->src_==sender)
      break;
  if(!snp){
    snp= new SRMSource(sender);
    snp->src_= sender;
    snp->received_= snp->sent_= snp->sess_ctr_sent_= -1;
    snp->messages_= snp->cache_= 0;
    snp->distance_ = 1;
    snp->recvt_ = snp->sendt_ = 0;
    snp->next_= allsrcs_;
    allsrcs_= snp;
  }
  return snp;
}



/* Constructor for SRM Agent */
SRMAgent::SRMAgent() 
	: Agent(PT_SRM), allsrcs_(0),seq_no_(-1),session_ctr_(0), trafgen_(0)
{
  type_ = PT_SRM;
  /* These can also be manipulated from tcl scripts */
  C_[1] = C1;
  C_[2] = C2;
  D_[1] = D1;
  D_[2] = D2;
  max_packets_ = MAX_PKTS; 
  bind("packet-size", &size_);
  bind("seqno", &seq_no_);
  bind("maxpkts", &max_packets_);
  bind("c1", &(C_[1]));
  bind("c2", &(C_[2]));
  bind("d1", &(D_[1]));
  bind("d2", &(D_[2]));
  bind("off_ip_", &off_ip_);
  bind("off_cmn_", &off_cmn_);
  bind("off_srm_", &off_srm_);
}


/* Start the agent by scheduling first session message */
void SRMAgent::start()
{
  double delay;
  running_ = 1;
  delay = SRM_SESSION_DELAY*Random::uniform(0.8,1.2);
  sched(delay,	SRM_SESSION_TIMER);
}

void SRMAgent::start_sender()
{
  double delay;
  /* If this member is a source/sender schedule data packet */
  /* Later add support for various source models */

  if (! running_) {
    printf(" can't start sender before starting the srm agent \n");
    return;
  }

  if (trafgen_) {
    trafgen_->init();
    delay = trafgen_->next_interval(size_);
    printf("%f: Node %d, Agent %d scheduling to send first data packet at %f\n",
	   NOW, (addr_>> 8), localsrc_->src_, NOW+delay);
    (void)schedule_send_event(delay,SRM_DATA,0,0);
  } else {   /* No traffic generator has been attach, do default */
    delay= Random::exponential()+1;
    printf("%f: Node %d, Agent %d scheduling to send first data packet at %f\n",
	   NOW, (addr_>> 8), localsrc_->src_, NOW+delay);
    (void)schedule_send_event(delay,SRM_DATA,0,0);
  }
  sender_running_ = 1;

}





int SRMAgent::command(int argc,const char*const*argv)
{
  int sender;
  Tcl& tcl = Tcl::instance();

  if(argc==2){
    if(strcmp(argv[1],"sender")==0){
      start_sender();
      return TCL_OK;
    }
    if(strcmp(argv[1],"start_")==0){
      start();
      return TCL_OK;
    }
  }
  if (argc == 3) {
    if (strcmp(argv[1], "localsrc") == 0) {
      sender = atoi(argv[2]);
      localsrc_ = (SRMSource *)get_sender_state(sender);
      return (TCL_OK);
    }
    if (strcmp(argv[1], "attach-traffic") == 0) {
      trafgen_ = (TrafficGenerator *)TclObject::lookup(argv[2]);
      if (trafgen_ == 0) {
	tcl.resultf("no such traffic generator %s", argv[2]);
	return(TCL_ERROR);
      }
      return(TCL_OK);
    }	
  }

  return Agent::command(argc,argv);
}


inline srmEvent*SRMAgent::schedule_send_event(double time,int type,int sndr,int id){
  srmEvent*sev= new srmEvent(this,type,sndr,id);
 Scheduler::instance().schedule(&srmEvent_handler,sev,time);
  return sev;
}

inline void SRMAgent::send_packet(int type,int sndr,int id, int size){
  Packet*p= Agent::allocpkt();
  hdr_srm *srmh = (hdr_srm*)p->access(off_srm_);
  hdr_cmn *th = (hdr_cmn*)p->access(off_cmn_);
  srmh->srcid() = localsrc_->src_; /* Source of the control message */

/* This field is no longer valid */
/*   p->class_ = type; */

  if (size) 
    th->size() = size;
  srmh->type() = type;         /* Type of srm message */
  srmh->sender() = sndr;       /* Sender of the related data message */
  srmh->id()= id;             /* Message id */
  transmit(p,type,sndr,id);
}

inline void SRMAgent::transmit(Packet*p,int type,int sndr,int id){

#ifdef trace_yes
  if(trace_){
    sprintf(trace_->buffer(),
	    "%f: node %d sends %s/%d ADU <%d, %d>",
	  NOW,
	    p->src_,srmNames[type],p->uid_,sndr,id);
    trace_->dump();
  }
#endif trace_yes

  target_->recv(p);
}

/* Send data packet and schedule the next */
void SRMAgent::send_data()
{
  double delay;
  if(max_packets_&&seq_no_< max_packets_){
    seq_no_++;
    seq_no_++;
    send_packet(SRM_DATA,localsrc_->src_,seq_no_,size_); 
    if (trafgen_) {
      delay = trafgen_->next_interval(size_);
    } else {
      delay= Random::exponential();
    }
    (void)schedule_send_event(delay,SRM_DATA,0,0); 
  }
}


void SRMAgent::recv_data(int sender,int msgid)
{
  SRMSource *snp;

  snp= get_sender_state(sender);
  if(snp->sent_<msgid)
    snp->sent_= msgid;
  snp->cancel_event(msgid);
  snp->reset_backoff(msgid);
  if(snp->status(msgid)==SRM_REPR_PENDING||
     snp->status(msgid)==SRM_REPR_WAIT){
    printf("%f: Node %d, Agent %d  ignore request for s:%d, m:%d till %f\n",
	   NOW, (addr_ >> 8), localsrc_->src_, sender, msgid, NOW+3*snp->distance_);
    snp->backoff_timer(msgid,3*snp->distance_);
    snp->status(msgid,SRM_REPR_WAIT);
  }else{
    if(snp->status(msgid)==SRM_RQST_PENDING) {
      printf("%f:Node %d, Agent %d cancelling request for src:%d, msgid:%d\n",NOW, (addr_ >> 8), localsrc_->src_, sender, msgid);
    snp->cancel_event(msgid);
    }
    snp->status(msgid,SRM_DATA_RECEIVED);
  }
  snp->update_vars();

  for(int i= snp->received_+1;i<snp->sent_;i++)
    if(snp->status(i)==SRM_UNKNOWN) {
      printf("%f:Node %d, Agent %d detected loss for s:%d, m:%d\n",NOW,  (addr_ >> 8), localsrc_->src_, sender, i);
      request(snp->src_,i);
    }
}


void SRMAgent::request(int sender,int msgid)
{
  srmEvent*sev;
  SRMSource *snp;
  double delay;

  snp= get_sender_state(sender);
  delay= (C_[1]+C_[2]*Random::uniform(0,1))*snp->distance_*
    snp->backoff(msgid);
  printf("%f:Node %d, Agent %d scheduling request for s:%d, m:%d, at %f\n",NOW, (addr_ >> 8), localsrc_->src_, sender, msgid,delay+NOW);
  sev= schedule_send_event(delay,SRM_RQST,sender,msgid);
  snp->set_event(msgid,(Event *)sev);
  printf("%f: Node %d, Agent %d ignore request backoff for s:%d, m:%d till %f\n",
	 NOW, (addr_ >> 8), localsrc_->src_, sender, msgid, NOW + delay/2);
  snp->backoff_timer(msgid,(delay/2));
  snp->incr_backoff(msgid);
  snp->status(msgid,SRM_RQST_PENDING);
}

void SRMAgent::send_rqst(int sender,int msgid)
{
  printf("%f:Node %d, Agent %d sending request for src:%d, msgid:%d\n",NOW, (addr_ >> 8), localsrc_->src_, sender, msgid);
  send_packet(SRM_RQST,sender,msgid, sizeof(session_info)); 
  /* The event has already triggered, don't necessarily need to cancel it */
  (get_sender_state(sender))->cancel_event(msgid);
  request(sender,msgid);
}

void SRMAgent::recv_rqst(int sender, int msgid, int rqst_src)
{
  SRMSource *snp;


  printf("%f:Node %d, Agent %d recving request for src:%d, msgid:%d\n",NOW, (addr_ >> 8), localsrc_->src_, sender, msgid);
  snp= get_sender_state(sender);
  switch(snp->status(msgid)){
  case SRM_UNKNOWN:
/*    if(sender!=localsrc_->src_&&msgid>snp->received_){   
      request(sender,msgid);
    break;
    }
*/
    /* Currently assume that src receives requests for valid msgid's only */
    if(sender == localsrc_->src_ || msgid <= snp->received_) {
      repair(sender, msgid, rqst_src);
    } else {
      /* Need to get rid of this msginfo struct */
    }
    break;
    /* Do not send requests when you receive requests for greater sno */
  case SRM_DATA_RECEIVED:
    repair(sender,msgid, rqst_src);
    break;
  case SRM_REPR_PENDING:
  case SRM_REPR_WAIT:
    break;
  case SRM_RQST_PENDING:
    printf("%f:Node %d, Agent %d cancelling request for src:%d, msgid:%d\n",NOW, (addr_ >> 8), localsrc_->src_, sender, msgid);
    snp->cancel_event(msgid);
    if(NOW >= snp->backoff_timer(msgid)){
      snp->incr_backoff(msgid);
    }
    request(sender,msgid);
    break;
  }
}

void SRMAgent::repair(int sender,int msgid, int rqst_src)
{
  srmEvent*sev;
  SRMSource *snp;
  double delay;
  double distance;

  snp = get_sender_state(rqst_src);
  snp->set_rsrc(msgid, rqst_src);
  distance = snp->distance_;
  snp= get_sender_state(sender);
  delay= (D_[1]+D_[2]*Random::uniform(0,1))*distance*snp->backoff(msgid);
  printf("%f:Node %d, Agent %d scheduling repair for s:%d, m:%d, after %f\n",NOW, (addr_ >> 8), localsrc_->src_, sender, msgid,delay+NOW);
  sev= schedule_send_event(delay,SRM_REPR,sender,msgid);
  snp->set_event(msgid,(Event *)sev);
  snp->status(msgid,SRM_REPR_PENDING);
}

void SRMAgent::send_repr(int sender,int msgid)
{
  SRMSource *snp, *rsnp;
  int r_src;
  double delay;

  printf("%f:Node %d, Agent %d sending repair for src:%d, msgid:%d\n",NOW, (addr_ >> 8), localsrc_->src_, sender, msgid);
  send_packet(SRM_REPR,sender,msgid, size_); /*Repair is same size as data, might need some changes*/
  snp= get_sender_state(sender);
  /* No need to cancel as event is already triggered */
  snp->cancel_event(msgid);
  snp->reset_backoff(msgid);
  r_src = snp->rsrc(msgid);
  rsnp = get_sender_state(r_src);
  delay = 3*rsnp->distance_;
  printf("%f: Node %d, Agent %d ignore request for s:%d, m:%d till %f\n",
	 NOW, (addr_ >> 8), localsrc_->src_, sender, msgid, NOW+delay);
  snp->backoff_timer(msgid,delay);
  snp->status(msgid,SRM_REPR_WAIT);  
}

void SRMAgent::recv_repr(int sender, int msgid)
{
  printf("%f:Node %d, Agent %d receiving repair for src:%d, msgid:%d\n",NOW, (addr_ >> 8), localsrc_->src_, sender, msgid);
  recv_data(sender, msgid);
}

void SRMAgent::send_session()
{

/*  printf("Sending session message\n");
  print_groupstate(); */

  Packet*p= Agent::allocpkt();
  hdr_srm *msg = (hdr_srm *)p->access(off_srm_);
  hdr_cmn *th = (hdr_cmn *)p->access(off_cmn_);
  session_info *s_info;

  msg->type()= SRM_SESS;
  msg->srcid() = localsrc_->src_;
  msg->id() = seq_no_;
  msg->session_ctr() = ++session_ctr_;
  msg->stime() = (int)(Scheduler::instance().clock()*1000);
  msg->rtime() = (int)(Scheduler::instance().clock()*1000);
  localsrc_->sendt_ = msg->stime();
  msg->num_entries() =0;
  s_info = (session_info *)msg->var_data();
  /* Currently assume that all the info can fit in var data.. */
  for(SRMSource*snp= allsrcs_;snp;snp= snp->next_){
    s_info->srcid_= snp->src_;
    s_info->id_= snp->sent_;
    s_info->session_ctr_= snp->sess_ctr_sent_;
    s_info->rtime_= snp->recvt_;
    s_info->stime_= snp->sendt_;
    s_info++;
    msg->num_entries()++;
  }

/*  Not a valid field anymore */
/*  p->class_ = SRM_SESS;   */

  th->size() = (msg->num_entries()+1)*sizeof(session_info); 
  transmit(p,SRM_SESS,localsrc_->src_,seq_no_);
}

/* Sending of session messages is handeled by timeouts */
void SRMAgent::timeout(int tno)
{
  assert(tno==SRM_SESSION_TIMER);
  send_session();

  sched(SRM_SESSION_DELAY*Random::uniform(0.8,1.2),
	SRM_SESSION_TIMER);

}

void SRMAgent::recv_sess(hdr_srm *msg)
{
  SRMSource *snp, *sender_snp;
  session_info *s_info;
  int msgid,sender,sessctr,stime,rtime,now, num_entries;

  
  GET_SESSION_INFO1;
  sender_snp = snp = get_sender_state(sender);
  now= 0;
  if(snp->sess_ctr_sent_<sessctr){
    now= (int)(NOW*1000);
    snp->sess_ctr_sent_= sessctr;
    if(msgid>snp->sent_) /* I think this should be commented */
      snp->sent_= msgid;
    snp->recvt_= now;    /* Why not request packets here also?? I will encounter that thing later also */
    snp->sendt_= stime;
  }
  if (sender == localsrc_->src_) { /* Local session message */
    return;
  }
  s_info = (session_info *)msg->var_data();
  num_entries = msg->num_entries();
  while(num_entries){
    GET_SESSION_INFO;
    snp= get_sender_state(sender);
    if((sender==localsrc_->src_)&&(now!=0)){
      int rtt= (now-stime)-(sender_snp->sendt_-rtime);
      sender_snp->distance_= (double)rtt/2/1000;
        /* To print the rtt information */
/*      printf("rtt from %d to %d is %d, distance is %f\n",localsrc_->src_, sender_snp->src_, rtt, sender_snp->distance_); */
    }else if(snp->sent_<=msgid){
      int first= snp->sent_+1;
      int last= msgid+1;
      for(int i= first;i<last;i++)
	if(i>=0&&snp->status(i)==SRM_UNKNOWN)
	   request(snp->src_,i);

      snp->sent_= msgid;
    }else{

    }
    s_info++;
    num_entries--;
  }
  print_groupstate();
}

void SRMAgent::print_groupstate()
{

#ifdef trace_yes
  SRMSource *member;

  printf("Member %d:: Groupstate at %f\n",localsrc_->src_, NOW);
  for(member=allsrcs_;member;member = member->next_) {
    printf("Member %d::rcv %d, snd %d, ses %d, rt %d, st %d, dst %f\n",
	   member->src_, member->received_, member->sent_, 
	   member->sess_ctr_sent_, member->recvt_, member->sendt_,
	   member->distance_);
  }
#endif trace_yes
}

void SRMAgent::recv(Packet*p, Handler*)
{
  hdr_srm *msg= (hdr_srm *)p->access(off_srm_);
  int sender, msgid;



#ifdef trace_yes
  if(trace_){
  }
#endif trace_yes

  if(msg->srcid() != localsrc_->src_) {
    sender = msg->sender();
    msgid = msg->id();
    switch(msg->type()){
    case SRM_DATA:
      recv_data(sender, msgid);
      break;
    case SRM_RQST:
      recv_rqst(sender, msgid, msg->srcid());
      break;
    case SRM_REPR:
      recv_repr(sender, msgid);
      break;
    case SRM_SESS:
      recv_sess(msg);
      break;
    default:
      
      break;
    }
  }
}


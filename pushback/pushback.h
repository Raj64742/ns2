#ifndef ns_pushback_h
#define ns_pushback_h

#include "config.h"
#include "packet.h"
#include "ip.h"
#include "agent.h"
#include "address.h"
#include "node.h"
#include "route.h"
#include "timer-handler.h"

#include "pushback-constants.h"
#include "pushback-message.h"

class PushbackQueue;
class PushbackEventList;
class IdentStruct;
class RateLimitSession;

struct hdr_pushback {
  
  PushbackMessage * msg_;
  
  static int offset_;
  inline static int& offset() {
    return offset_;
  }
  inline static hdr_pushback * access(Packet *p) {
    return (hdr_pushback *) p->access(offset_);
  }
};

// a structure to store details of each link on the node.
struct queue_rec {

  PushbackQueue * pbq_;             //pointer to the queue object
  IdentStruct * idTree_;              // this queues prefix tree

  // other required variables go here

};

class PushbackEvent;
class PushbackTimer;
  
class PushbackAgent : public Agent {

 public:
  PushbackAgent();
  int command(int argc, const char*const* argv);
  void reportDrop(int qid, Packet * p);
  void timeout(PushbackEvent * event);
  void identifyAggregate(int qid, double arrRate, double linkBW);
  void resetDropLog(int qid);
  void recv(Packet *p, Handler *);

  int last_index_;
  int intResult_;
  int debugLevel;
  char prnMsg[500];  //hopefully long enough
  
  static int mergerAccept(int count, int bits, int bitsDiff);
  void printMsg(char *msg, int msgLevel);

 protected:
  int enable_pushback_;
  queue_rec queue_list_[MAX_QUEUES];
  
  Node * node_;
  RouteLogic * rtLogic_;
  PushbackTimer * timer_;
  
  void initialUpdate(RateLimitSession * rls);
  void pushbackCheck(RateLimitSession * rls);
  void pushbackStatus(RateLimitSession* rls);
  void pushbackRefresh(int qid);
  void pushbackCancel(RateLimitSession* rls);

  void processPushbackRequest(PushbackRequestMessage * msg);
  void processPushbackStatus(PushbackStatusMessage *msg);
  void processPushbackRefresh(PushbackRefreshMessage *msg);
  void processPushbackCancel(PushbackCancelMessage *msg);

  void refreshUpstreamLimits(RateLimitSession * rls);
  int getQID(int sender);
  int checkQID(int qid);
  void sendMsg(PushbackMessage * msg);
};



class PushbackEvent {

 public:
  double time_;
  int eventID_;
  RateLimitSession * rls_;
  int qid_;
  
  PushbackEvent * next_;
  
  PushbackEvent(double delay, int eventID, RateLimitSession * rls) {
    time_ = Scheduler::instance().clock() + delay;
    eventID_ = eventID;
    rls_ = rls;
    qid_= -1; //rls->localQID_;
    next_=NULL;
  }

  PushbackEvent(double delay, int eventID, int qid) {
    time_ = Scheduler::instance().clock() + delay;
    eventID_ = eventID;
    rls_=NULL;
    qid_=qid;
    next_=NULL;
  }
  
  void setSucc(PushbackEvent * event) {
    next_=event;
  }

  static char * type(PushbackEvent * event) {
    switch (event->eventID_) {
    case PUSHBACK_CHECK_EVENT: return "CHECK";
    case PUSHBACK_REFRESH_EVENT: return "REFRESH";
    case PUSHBACK_STATUS_EVENT: return "STATUS";
    case INITIAL_UPDATE_EVENT: return "INITIAL UPDATE";
    default: return "UNKNOWN";
    }
  }
};

class PushbackTimer: public TimerHandler {
  
 public: 
  PushbackEvent * firstEvent_;
  
  PushbackTimer(PushbackAgent * agent): firstEvent_(NULL) { agent_ = agent;}
  void insert(PushbackEvent * event);
  void cancelStatus(RateLimitSession * rls);
  void removeEvents(RateLimitSession * rls);
  int containsRefresh(int qid);

 protected:
  PushbackAgent * agent_;
  virtual void expire(Event *e);
  void schedule();
  void sanityCheck();
};

#endif 
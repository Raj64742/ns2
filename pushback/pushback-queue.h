#ifndef ns_pushback_queue_h
#define ns_pushback_queue_h

#include "red.h"
#include "queue-monitor.h"
#include "timer-handler.h"
#include "pushback-constants.h"

class RateLimitSessionList;
class RateEstimator;

class PushbackQueueTimer;
class PushbackAgent;

class PushbackQueue: public REDQueue {

 public:
  PushbackQueue( const char* const);
  
  virtual void reportDrop(Packet *pkt);
  void enque(Packet *p);
  //  Packet * deque() {return NULL;}

  PushbackAgent * pushback_; 
  int command(int, const char*const*);
  void timeout(int);
  double getBW();
  double getRate();
  double getDropRate();

  RateLimitSessionList * rlsList_;
  RateEstimator * rateEstimator_;

 protected: 
  int pushbackID_;
  int src_;
  int dst_;
  int rate_limiting_;
  EDQueueMonitor * qmon_;
  PushbackQueueTimer * timer_;

};

class PushbackQueueTimer: public TimerHandler {

 public:
  PushbackQueueTimer(PushbackQueue * queue) {queue_ = queue;}

  virtual void
  expire(Event *e) {
    //    printf("PBQTimer: expiry at %g\n", Scheduler::instance().clock());
    queue_->timeout(0);
  }
   
 protected:
  PushbackQueue * queue_;
  
 
};

#endif

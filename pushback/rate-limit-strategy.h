#ifndef ns_rate_limit_strategy_h
#define ns_rate_limit_strategy_h

#include "pushback-constants.h"
#include "packet.h"
#include "rate-estimator.h"
#include "timer-handler.h"

//container class for rate limiter
class RateLimiter {
 public:  
  virtual int rateLimit(Packet * p, double estRate, double targetRate, 
			int mine, int lowDemand)=0;
  virtual void reset()=0;
};

class PacketTypeLog;

class RateLimitStrategy {
 public:
  double target_rate_;          //predefined flow rate in bps 
  double reset_time_;           //time since the aggregate stats are being collected         
  
  RateEstimator * rateEstimator_; 
  RateLimiter * rateLimiter_;

  int ptype_;
  double ptype_share_;
  RateEstimator * ptypeRateEstimator_;
  RateLimiter * ptypeRateLimiter_;

  PacketTypeLog * ptypeLog_;

  RateLimitStrategy(double rate, int ptype, double share, double estimate);
  ~RateLimitStrategy();
  double process(Packet * p, int mine, int lowDemand);
  void restrictPacketType(int type, double share, double actual);
  
  double getDropRate();  
  double getArrivalRate();
  void reset();
};

class PacketTypeLog : public TimerHandler {
  
 public:
  int count_;
  int typeCount_[MAX_PACKET_TYPES];
  RateLimitStrategy * rlStrategy_;
  PacketTypeLog(RateLimitStrategy *);
  virtual ~PacketTypeLog();
  
  void log(Packet *);
  static int mapTypeToIndex(int type);
  static int  mapIndexToType(int index);
  static double mapTypeToShare(int type);
  
 protected:
  void expire(Event *e);
  
};
  
class TokenBucketRateLimiter: public RateLimiter {
 public:
  //the static parameters   
  double bucket_depth_;             //depth of the token bucket in bytes
   
  //the dynamic state
  double tbucket_;                 //number of tokens in the bucket; in bytes
  double time_last_token_;
  double total_passed_;
  double total_dropped_;
  
  TokenBucketRateLimiter();
  int rateLimit(Packet * p, double estRate, double targetRate, int mine, int lowDemand);
  void reset(); 
};

class PrefDropRateLimiter : public RateLimiter {
 public:
  //state for Pref Drop Implementation Goes Here
  
/*   virtual double process(Packet *p) { */
/*     //todo" stuff here */
/*     return 0; */
/*   } */
};


#endif

#ifndef ns_logging_data_struct_h
#define ns_logging_data_struct_h

#include "node.h"
#include "packet.h"
#include "route.h"
#include "rate-estimator.h"
#include "pushback-constants.h"

class LoggingDataStructNode {
 public:
  int nid_;      //the neighbors id.
  RateEstimator * rateEstimator_;

  int pushbackSent_;  //whether pushback message was sent to this neighbor
  double limit_;      //limit specified in the pushback message sent to it.
  
  //status details of this neighbor.
  int gotStatus_;   
  double statusArrivalRate_;
  int countLessReportedRate_;   //variable used to protect low senders
    
  int sentRefresh_; //whether we have sent a refresh to this neighbor yet.

  LoggingDataStructNode * next_;
  
  LoggingDataStructNode(int id, LoggingDataStructNode * next); 
  ~LoggingDataStructNode();
  
  void sentRefresh(double limit);
  void pushbackSent(double limit, double expectedStatusRate);
  void registerStatus(double arrRate);
  void log(Packet *pkt);
};


class LoggingDataStruct {

 public:
  LoggingDataStructNode * first_;
  int count_;       //number of members in this struct
  int myID_;        // id of the my node, needed to figure out who sent the pkt.

  RateEstimator * rateEstimator_;   // rate estimator for all bytes coming in for this RLS.

  double reset_time_;  //time when logging was last reset.

  //consolidated status details.
  int gotStatusAll_;
  double statusArrivalRateAll_;

  RouteLogic * rtLogic_;
  
  LoggingDataStruct(Node *, RouteLogic *, int sampleAddress, double estimate);
  ~LoggingDataStruct();

  void log(Packet * pkt);
  int consolidateStatus();
  void registerStatus(int sender, double arrRate);
  LoggingDataStructNode * getNodeByID(int id);
  void resetStatus();
};

#endif

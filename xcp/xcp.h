// Copyrights go here

#ifndef NS_XCP
#define NS_XCP

//#include <math.h>
#include "packet.h"
#include "queue.h"
#include "xcpq.h"

#define MAX_QNUM 3

enum {XCPQ=0, TCPQ=1, OTHERQ=2};
// code points for separating XCP/TCP flows
#define CP_XCP 10
#define CP_TCP 20
#define CP_OTHER 30


class XCPWrapQ : public Queue {
 public:
  XCPWrapQ();
  int command(int argc, const char*const* argv);
  void recv(Packet*, Handler*);

 protected:
  XCPQueue** xcpq_;      // array of xcp queue 
  unsigned int    routerId_;
  
  int qToDq_;                 // current queue being dequed
  double wrrTemp_[MAX_QNUM];     // state of queue being serviced
  double queueWeight_[MAX_QNUM]; // queue weight for each queue (dynamic)
  int maxVirQ_;             // num of queues used in xcp router

  // --- Tracing
  Tcl_Channel queue_trace_file_;
  double effective_rtt_;

  // Utilization
  double utilization_; /// computed every effective_rtt
  double output_traffic_ ; // in bits/sec
  double output_link_capacity_; // static value taken from the following delay element
  
  // Drops
  int trace_drops_;
  int drops_;
  int avg_drops_;
  double total_drops_ ;
  double total_packet_arrivals_;
  
  //---------- Current Queue Length
  int trace_curq_;

  void deliver_to_target(Packet* p);
  virtual void trace_var(char * var_name, double var);

  // Modified functions
  virtual void enque(Packet* pkt); // high level enque function
  virtual Packet* deque();         // high level deque function
    
  void addQueueWeights(int queueNum, int weight);
  int queueToDeque();              // returns qToDq
  int queueToEnque(int codePt);    // returns queue based on codept
  int mark(Packet *p);             // marks pkt based on flow type
  int getCodePt(Packet *p);        // returns codept in pkt hdr
  void setVirtualQueues();         // setup virtual queues(for xcp/tcp)
  //void everyRTT();
};

#endif //NS_XCP

#include "logging-data-struct.h"


// ########################## LoggingDataStruct Methods ####################

LoggingDataStruct::LoggingDataStruct(Node * node, RouteLogic * rtLogic, 
				     int sampleAddress, double estimate) {
  first_ = NULL;
  count_=0;
  myID_ = node->nodeid();
  rateEstimator_ = new RateEstimator(estimate);
  reset_time_ = Scheduler::instance().clock();
  gotStatusAll_ = 0;
  statusArrivalRateAll_=-1;
  rtLogic_ = rtLogic;
  
//   neighbor_list_item *np = node->neighbor_list.head;
//   for (; np; np = np->next) {
//     int nid = np->id;
//     int nextHopID = rtLogic_->lookup_flat(nid, sampleAddress);
//     if (nextHopID == node->nodeid()) {
//       LoggingDataStructNode * lgdsNode = new LoggingDataStructNode(nid, first_);
//       first_ = lgdsNode;
//       count_++;
//     }
//   }

  Node * nextNode = node->neighbor_list_.lh_first;
  while (nextNode != NULL) {
    int nid = nextNode->nodeid();
    int nextHopID = rtLogic_->lookup_flat(nid, sampleAddress);
     if (nextHopID == node->nodeid()) {
       LoggingDataStructNode * lgdsNode = new LoggingDataStructNode(nid, first_);
       first_ = lgdsNode;
       count_++;
     }
     nextNode = nextNode->neighbor_list_entry_.le_next;
  }
    
}

void
LoggingDataStruct::log(Packet * pkt) {

  rateEstimator_->estimateRate(pkt);

  hdr_ip * iph = hdr_ip::access(pkt);
  ns_addr_t src = iph->src();

  // there is a symmetry of routing assumption built into this computation.
  // if it does not hold, we need some explicit support to know 
  // which neighbor sent us this packet
  int neighborID = rtLogic_->lookup_flat(myID_,src.addr_);
#ifdef DEBUG_LGDS
  printf("LGDS: %d total = %g rate = %g src = %d neighnorID = %d\n", myID_, 
	 rateEstimator_->bytesArr_/500.0, rateEstimator_->estRate_, src.addr_, neighborID);
#endif  

  if (neighborID == myID_) return;
 
  LoggingDataStructNode * node = getNodeByID(neighborID);
  if (node == NULL) {
    fprintf(stdout,"LGDS: %d neighbor not found in the struct !!\n", myID_);
    exit(-1);
  }
  node->log(pkt);
}

//return -1 if not received status from a neighbor even once.
//return 0 if not received status from a neighbor in this round.
//return 1 o/w.  
int 
LoggingDataStruct::consolidateStatus() {

  double rate = 0;
  int all = 1;
  LoggingDataStructNode * node = first_;
  while (node != NULL) {
    if (!node->gotStatus_) {
      if (node->statusArrivalRate_<0) {
	//condition 1 above.
	printf("LGDS: Error: This should never happen now\n");
	exit(-1);
	//return -1;
      }
      else {
	all = 0;
      }
    }
    rate += node->statusArrivalRate_;
    node = node->next_;
  }

  gotStatusAll_ = all;
  statusArrivalRateAll_=rate;

  return all;
}

void
LoggingDataStruct::resetStatus() {
  LoggingDataStructNode * node = first_;
  while (node != NULL) {
    node->gotStatus_=0;
    node = node->next_;
  }
}
  
void
LoggingDataStruct::registerStatus(int sender, double arrRate) {
  
  LoggingDataStructNode * node = getNodeByID(sender);
  
  if (node == NULL) {
    printf("LGDS: sender not in my list (status processing)\n");
    exit(-1);
  }
  
  node->registerStatus(arrRate);
}

LoggingDataStructNode *
LoggingDataStruct::getNodeByID(int id) {
  
  LoggingDataStructNode * node = first_;
  while (node != NULL) {
    if (node->nid_ == id) {
      return node;
    }
    node = node->next_;
  }
  
  return NULL;
}

LoggingDataStruct::~LoggingDataStruct() {

  LoggingDataStructNode * node = first_;
  while (node!=NULL) {
    LoggingDataStructNode * next = node->next_;
    delete(node);
    node = next;
  }

  delete(rateEstimator_);
}

// ########################## LoggingDataStructNode Methods ####################

LoggingDataStructNode::LoggingDataStructNode(int id, LoggingDataStructNode * next) {
  
  printf("LGDSN: Setting up node for %d\n", id);
  nid_ = id;
  rateEstimator_ = new RateEstimator();

  pushbackSent_ = 0;
  limit_ = -1;
  gotStatus_ = 0; 
  statusArrivalRate_ = -1;
  countLessReportedRate_=0;
  sentRefresh_=0;
  next_ = next;
}

void
LoggingDataStructNode::pushbackSent(double limit, double expectedStatusRate) {
  pushbackSent_ = 1;
  limit_ = limit;
  statusArrivalRate_ = expectedStatusRate;
}

void
LoggingDataStructNode::sentRefresh(double limit) {
  sentRefresh_ = 1;
  limit_ = limit;
}

void
LoggingDataStructNode::registerStatus(double arrRate) {
  gotStatus_ = 1;
  
  //not doing precise math for floating imprecision
  if (limit_ < INFINITE_LIMIT - 1 ) {
    statusArrivalRate_ = arrRate;
    countLessReportedRate_=0;
    return;
  }

  if (arrRate >= statusArrivalRate_) {
    statusArrivalRate_ = arrRate;
    countLessReportedRate_=0;
    return;
  } else {
    countLessReportedRate_++;
    if (countLessReportedRate_ > 1) {
      statusArrivalRate_=(arrRate+statusArrivalRate_)/2;
      return;
    }
    else {
      //keep the old rate
    }
  }
}

void
LoggingDataStructNode::log(Packet * pkt) {
  rateEstimator_->estimateRate(pkt);
#ifdef DEBUG_LGDSN
  printf("LGDSN: for neighbor %d total = %g rate = %g \n", nid_, 
	 rateEstimator_->bytesArr_/500.0, rateEstimator_->estRate_);
#endif
}
  
LoggingDataStructNode::~LoggingDataStructNode() {
  delete(rateEstimator_);
}

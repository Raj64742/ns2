/******************************************************/
/* diff_header.h : Chalermek Intanagonwiwat  08/16/99 */
/******************************************************/

#ifndef ns_diff_header_h
#define ns_diff_header_h

#include "ip.h"

#define INTEREST      1
#define DATA          2
#define DATA_READY    3
#define DATA_REQUEST  4
#define POS_REINFORCE 5
#define NEG_REINFORCE 6
#define INHIBIT       7
#define TX_FAILED     8
#define DATA_STOP     9

#define MAX_ATTRIBUTE 3
#define MAX_NEIGHBORS 30
#define MAX_DATA_TYPE 30

#define ROUTING_PORT 255

#define ORIGINAL    100        
#define SUB_SAMPLED 1         

// For positive reinforcement in simple mode. 
// And for TX_FAILED of backtracking mode.

struct extra_info {
  ns_addr_t sender;            // For POS_REINFORCE and TX_FAILED
  unsigned int seq;           // FOR POS_REINFORCE and TX_FAILED
  int size;                   // For TX_FAILED only.
};


struct hdr_diff {
  unsigned char mess_type;
  unsigned int pk_num;
  ns_addr_t sender_id;
  nsaddr_t next_nodes[MAX_NEIGHBORS];
  int      num_next;
  unsigned int data_type;
  ns_addr_t forward_agent_id;

  struct extra_info info;

  double ts_;                       // Timestamp when pkt is generated.
  int    report_rate;               // For simple diffusion only.
  int    attr[MAX_ATTRIBUTE];

  static int offset_;
};



#endif

#ifndef ns_pushback_constants_h
#define ns_pushback_constants_h

/* #define DEBUG_LGDS  */
/* #define DEBUG_LGDSN  */
/* #define DEBUG_AS  */
/* #define DEBUG_RLSL */

#define NO_BITS 4
#define MAX_CLUSTER 20
#define CLUSTER_LEVEL 4
#define MIN_BITS_FOR_MERGER 2

// maximum no of queues on the node
#define MAX_QUEUES 10

#define MIN_TIME_TO_FREE 10
#define RATE_LIMIT_TIME_DEFAULT 30    //in seconds
#define DEFAULT_BUCKET_DEPTH  5000           //in bytes


#define PUSHBACK_CHECK_EVENT 1
#define PUSHBACK_REFRESH_EVENT 2
#define PUSHBACK_STATUS_EVENT 3
#define INITIAL_UPDATE_EVENT 4

#define PUSHBACK_CHECK_TIME  5   //in seconds
#define PUSHBACK_CYCLE_TIME 5
#define INITIAL_UPDATE_TIME 0.5

#define DROP_RATE_FOR_PUSHBACK 0.10

#define SUSTAINED_CONGESTION_PERIOD  2      //in seconds
#define SUSTAINED_CONGESTION_DROPRATE 0.10  //fraction
#define TARGET_DROPRATE 0.05  

#define MAX_PACKET_TYPES 4
#define PACKET_TYPE_TIMER 2

#define INFINITE_LIMIT 100e+6 //100Mbps

#endif

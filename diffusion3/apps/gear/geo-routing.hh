// *********************************************************
//
// geo-routing.hh  : GEAR Include File
// author          : Yan Yu
//
// $Id: geo-routing.hh,v 1.2 2002/03/20 22:49:39 haldar Exp $
//
// *********************************************************

#ifndef GEO_ROUTING_HH
#define GEO_ROUTING_HH

#include <list>

#ifndef USE_WINSNG2
#include <hash_map>
#else
#include <ext/hash_map>
#endif // !USE_WINSNG2

#ifdef NS_DIFFUSION
#include <mobilenode.h>
#endif // NS_DIFFUSION

#include "geo-attr.hh"
#include "diffapp.hh"

// Filter priorities for pre-processing and post-processing. The
// gradient's filter priority has to be between these two values in
// order for GEAR to work properly
#define GEOROUTING_FILTER_PRE_PRIORITY  8
#define GEOROUTING_FILTER_POST_PRIORITY 2

// Energy values for GEAR
#define	GEO_INITIAL_ENERGY              1
#define	GEO_UNIT_ENERGY_FOR_SEND    0.001
#define	GEO_UNIT_ENERGY_FOR_RECV    0.001

#define GEO_REQUEST             1
#define	GEO_REPLY               2
#define	GEO_H_VALUE_UPDATE      3

#define GEO_BEACON_PERIODIC_CHECK_TIMER		150

#define	TRUE	1
#define	FALSE	0
#define	FAIL	-1

//set the following two constant to be negative to avoid it coincidental with actual next hop
#define	BROADCAST 		-2
#define	BROADCAST_SUPPRESS	-3

// Send at most one neighbor beacon every GEO_BEACON_PERIOD seconds
#define	GEO_BEACON_PERIOD	100

#define GEO_NEIGHBOR_TIMEOUT	1100
//GEO_NEIGHBOR_TIMEOUT is the lifetime of neighbor table entry
//when a neighbor entry's age >= GEO_NEIGHBOR_TIMEOUT, it will be deleted.

#define GEO_NEIGHBOR_OUTDATE_TIMEOUT	(GEO_NEIGHBOR_TIMEOUT - 50)
//GEO_NEIGHBOR_OUTDATE_TIMEOUT  is the age when a neighbor entry is considered as outdated.

#define	GEO_NEI_REQ_GAP_THRE	1000
//generate at least one neighbor beaconing request every GEO_NEI_REQ_GAP_THRE seconds.

#define	GEO_H_VALUE_UPDATE_THRE	2

#define GEO_BEACON_GENERATE_DELAY   1500        // (msec) bw receive and forward
#define GEO_BEACON_GENERATE_JITTER  1000        // (msec) jitter


#define GEO_BEACON_REQUEST_DELAY   400        // (msec) bw receive and forward
#define GEO_BEACON_REQUEST_JITTER  200        // (msec) jitter

#define	GEO_GREEDY_MODE		1
#define	GEO_SEARCH_MODE		2


#define	INITIAL_ENERGY		1
#define	DEFAULT_VALID_PERIOD		10
#define FORWARD_TABLE_SIZE      100
#define	LEARNED_COST_TABLE_SIZE		1000
//learned cost is forwarding cache..  FORWARD_TABLE_SIZE * MAX_NUM_NEI;

#define	DEFAULT_SCOPE 100
#define UNICAST_ORI     1
#define UNICAST_SUB     2
#define	BROADCAST_TYPE  3
#define GREEDY_MODE     1
#define DFS_MODE        2
#define	MAX_INT         10000
#define	ABS(x)          ((x) >= 0 ? (x): -(x))

class TimerType {
public:
  int   which_timer;
  void  *param;

  TimerType(int _which_timer) : which_timer(_which_timer)
  {
    param = NULL;
  };

  ~TimerType() {};
};

class GeoLocation {
public:
  double x;
  double y;

  void operator= (GeoLocation p) {x = p.x; y = p.y;}
  void output() {diffPrint(DEBUG_IMPORTANT, "(%f, %f)", x, y );}
};

class Region {
public:
  GeoLocation center;
  double radius;

  void operator= (Region p) {center = p.center; radius= p.radius;}
  void output()
  {
    center.output();
    diffPrint(DEBUG_IMPORTANT, "-%f", radius);
  }
};

class Geo_Header {
public:
  int path_len;
  double scope;

  int pkt_type;  // BROADCAST, or UNICAST_ORI, or UNICAST_SUB
  Region dst_region;
  Region sub_region;
  int mode;      // GREEDY or SEARCH
  double greedy_failed_dist;
};
 
class Pkt_Header {
public:
  // Packet identification
  int32_t pkt_num;
  int32_t rdm_id;

  int32_t prev_hop;
  int path_len;
  double scope;

  int pkt_type;
  Region dst_region;
  Region sub_region;
  int mode;
  double greedy_failed_dist;
};

#define INITIAL_H_VALUE -1

class H_value {
public:
  double dst_x;
  double dst_y;
  double h_val;
};

class Neighbor_Entry {
public:
  int32_t id;

  double longitude;
  double latitude;
  double remaining_energy;
  struct timeval tv;
  double valid_period; // in seconds

  Neighbor_Entry() {
    valid_period = DEFAULT_VALID_PERIOD;
  }
};

class GeoRoutingFilter;

typedef	hash_map<int, Neighbor_Entry *> Neighbors_Hash;
typedef list<Pkt_Header *> Packets_List;

class GeoFilterReceive : public FilterCallback {
public:
  GeoRoutingFilter *app;

  GeoFilterReceive(GeoRoutingFilter *_app) : app(_app) {};
  void recv(Message *msg, handle h);
};

class GeoTimerReceive : public TimerCallbacks {
public:
  GeoRoutingFilter *app;

  GeoTimerReceive(GeoRoutingFilter *_app) : app(_app) {};
  int expire(handle hdl, void *p);
  void del(void *p);
};

class H_value_entry {
public:
  H_value_entry() {
    h_value = INITIAL_H_VALUE;
  }

  GeoLocation dst;
  double h_value;
};

//local forwarding table at each node
class H_value_table {
public:
  H_value_table() {num_entries = 0; first = -1; last = -1;}

  H_value_entry table[FORWARD_TABLE_SIZE];

  int RetrieveEntry(GeoLocation  *dst);
  inline int NumEntries() {return num_entries;}

  void AddEntry(GeoLocation dst, double h_val);
  bool UpdateEntry(GeoLocation dst, double h_val);
  //the return value of UpdateEntry is if there is significant change
  //compared to its old value, if there is(i.e., either the change
  //pass some threshold-GEO_H_VALUE_UPDATE_THRE or it is a new entry),
  //then return TRUE, o/w return FALSE;
  int Next(int i) {return ((i+1) % FORWARD_TABLE_SIZE);}
  int Last() {return last;}

private:
  int num_entries;
  int first;
  int last;
};

class Learned_cost_entry {
public:
  Learned_cost_entry() {
    l_cost_value = INITIAL_H_VALUE;
  }

  int node_id;
  GeoLocation dst;
  double l_cost_value;
};

// Local forwarding table
class Learned_cost_table {
public:
  Learned_cost_table() {num_entries = 0; first = -1; last = -1;}

  Learned_cost_entry table[LEARNED_COST_TABLE_SIZE];

  int RetrieveEntry(int neighbor_id, GeoLocation *dst);
  inline int NumEntries() {return num_entries;}

  void AddEntry(int neighbor_id, GeoLocation dst, double h_val);
  void UpdateEntry(int neighbor_id, GeoLocation dst, double h_val);
  int Next(int i) {return ((i+1) % LEARNED_COST_TABLE_SIZE);}
  int Last() {return last;}

private:
  int num_entries;
  int first;
  int last;
};

class GeoRoutingFilter : public DiffApp {
public:
#ifdef NS_DIFFUSION
  GeoRoutingFilter();
  int command(int argc, const char*const* argv);
#else
  GeoRoutingFilter(int argc, char **argv);
#endif // NS_DIFFUSION

  void run();
  void recv(Message *msg, handle h);
  int ProcessTimers(handle hdl, void *p);

protected:
  // General Variables
  handle preFilterHandle;
  handle postFilterHandle;
  int pkt_count;
  int rdm_id;

  // Keep track when last beacon was sent
  struct timeval beacon_tv;

  // Keep track when last neighbor request was sent
  struct timeval neighbor_req_tv;
  
  // Statistical data: location and remaining energy level
  double geo_longitude;
  double geo_latitude;
  int num_pkt_sent;
  int num_pkt_recv;
  double initial_energy;
  double unit_energy_for_send;
  double unit_energy_for_recv;

  // Hash table with neighbor information
  Neighbors_Hash neighbors_table;

  // List of messages currently being processed
  Packets_List message_list;

  // Forwarding table
  H_value_table h_value_table;
  Learned_cost_table learned_cost_table;
  
  // Receive Callback for the filter
  GeoFilterReceive *fcb;
  GeoTimerReceive *tcb;

  // Setup the filter
  handle setupPostFilter();
  handle setupPreFilter();

  // Message Processing functions
  void PreProcessMessage(Message *msg, handle h);
  void PostProcessMessage(Message *msg, handle h);

  // Timers
  void MessageTimeout(Message *msg);
  void InterestTimeout(Message *msg);
  void GeoBeaconTimeout();
  
  // Message processing functions
  Pkt_Header * PreProcessInterest(Message *msg);
  Pkt_Header * StripOutHeader(Message *msg);
  Pkt_Header * RetrievePacketHeader(Message *msg);
  bool ExtractLocation(Message *msg, float *x1, float *x2, float *y1, float *y2);
  void CopyGeoHeader(Message *msg, Geo_Header *geo_header, Pkt_Header *pkt_header);
  void TakeOutAttr(NRAttrVec *attrs, int32_t key);

  // Neighbors related functions
  bool CheckNeighborsInfo();
  void DeleteOldNeighbors();
  void SendNeighborRequest(int32_t neighbor_id, Message *msg);
  double GeoRemainingEnergy() {return INITIAL_ENERGY;}
  void UpdateNeighbor(int neighbor_id, double x, double y, double energy);

  // Cost estimation related functions
  double RetrieveLearnedCost(int neighbor_id, GeoLocation dst);
  double EstimateCost(int neighbor_id, GeoLocation dst);  
  double dist(GeoLocation p1, GeoLocation p2);

  // Routing related functions
  int32_t GeoFindNextHop(Message *msg);
  int GreedyNext(Geo_Header *pkt);
  int NavigateHole(Geo_Header *pkt);
  int FloodInsideRegion(Geo_Header *pkt);

  double RetrieveH_Value(GeoLocation dst);
  void BroadcastH_Value(GeoLocation dst, double  new_h, int to_id);

  // GetNodeLocation --> This will move to the library in the future
  void GetNodeLocation(double *x, double *y);
#ifdef NS_DIFFUSION
  // This will also go away in the future
  MobileNode *node_;
#endif // NS_DIFFUSION
};

double Distance(double x1, double y1, double x2, double y2);
bool same_location(GeoLocation src, GeoLocation dst);

#endif // GEO_ROUTING_HH


// *********************************************************
//
// geo-routing.cc : GEAR Filter
// author         : Yan Yu
//
// $Id: geo-routing.cc,v 1.3 2002/03/21 19:30:54 haldar Exp $
//
// *********************************************************

#include <math.h>
#include "geo-routing.hh"

#ifdef NS_DIFFUSION
static class GeoRoutingFilterClass : public TclClass {
public:
  GeoRoutingFilterClass() : TclClass("Application/DiffApp/GeoRoutingFilter") {}
  TclObject * create(int argc, const char*const* argv) {
    return(new GeoRoutingFilter());
  }
} class_geo_routing_filter;

int GeoRoutingFilter::command(int argc, const char*const* argv) {

  if (argc == 2) {
    if (strcmp(argv[1], "start") == 0) {
      run();
      return TCL_OK;
    }
  }
  if (argc == 3) {
    if (strcmp(argv[1], "node") == 0) {
      node_ = (MobileNode *)TclObject::lookup(argv[2]);
      return TCL_OK;
    }
  }

  return DiffApp::command(argc, argv);
}
#endif // NS_DIFFUSION

void GeoFilterReceive::recv(Message *msg, handle h)
{
  app->recv(msg, h);
}

int GeoTimerReceive::expire(handle hdl, void *p)
{
  return app->ProcessTimers(hdl, p);
}

//need to change this funciton later..!!!!!!!
void GeoTimerReceive::del(void *p)
{
  TimerType *timer;
  Message *msg;

  timer = (TimerType *) p;

  switch (timer->which_timer){

  case MESSAGE_SEND_TIMER:

    msg = ((Message *) timer->param);

    if (msg)
      delete msg;

    break;

  }

  delete timer;
}

int GeoRoutingFilter::ProcessTimers(handle hdl, void *p)
{
  TimerType *timer;
  Message *msg;
  int timeout = 0;

  timer = (TimerType *) p;
 
  switch (timer->which_timer){

  case MESSAGE_SEND_TIMER:

    msg = ((Message *) timer->param);

    MessageTimeout(msg);

    // Cancel Timer
    timeout = -1;

    break;

  case GEO_BEACON_PERIODIC_CHECK_TIMER:

    GeoBeaconTimeout();

    break;

  default:

    printf("Error: ProcessTimers received unknown timer %d !\n", timer->which_timer);

    break;
  }

  return timeout;
}

void GeoRoutingFilter::GeoBeaconTimeout()
{
  NRAttrVec *attrs;
  Message *beacon_msg;
  struct timeval tv;

  getTime(&tv);

  // This function sends out a beacon at least once every
  // GEO_NEI_REQ_GAP_THRE milliseconds.

  // We broadcast the request from time to time, in case a new
  // neighbor joins in and never gets a chance to get its
  // informaiton

  if ((tv.tv_sec - neighbor_req_tv.tv_sec) > GEO_NEI_REQ_GAP_THRE){
    // Sends a beacon to all neighbors
    attrs = new NRAttrVec;
    attrs->push_back(GeoBeaconTypeAttr.make(NRAttribute::IS, GEO_REQUEST));
    attrs->push_back(GeoLatitudeAttr.make(NRAttribute::IS, geo_latitude));
    attrs->push_back(GeoLongitudeAttr.make(NRAttribute::IS, geo_longitude));
    attrs->push_back(GeoRemainingEnergyAttr.make(NRAttribute::IS,
						 GeoRemainingEnergy()));

    beacon_msg = new Message(DIFFUSION_VERSION, DATA, 0, 0,
			     attrs->size(), pkt_count, rdm_id,
			     BROADCAST_ADDR, LOCALHOST_ADDR);
    // Don't forget to increment pkt_count
    pkt_count++;

    beacon_msg->msg_attr_vec = CopyAttrs(attrs);

    // For now, send message to all neighbors
    beacon_msg->last_hop = LOCALHOST_ADDR;
    beacon_msg->next_hop = BROADCAST_ADDR;

    diffPrint(DEBUG_IMPORTANT, "GEO: Broadcast neighbor info request..\n");
    ((DiffusionRouting *)dr)->sendMessage(beacon_msg, preFilterHandle);

    ClearAttrs(attrs);
    delete attrs;
    delete beacon_msg;
  }
  else
    diffPrint(DEBUG_IMPORTANT,
	      "GEO: Suppressed Broadcast neighbor info request..\n" );
}

void GeoRoutingFilter::MessageTimeout(Message *msg)
{
  // Send message
  diffPrint(DEBUG_IMPORTANT, "Message Timeout !\n");

  ((DiffusionRouting *)dr)->sendMessage(msg, preFilterHandle,
					GEOROUTING_FILTER_POST_PRIORITY);
}

void GeoRoutingFilter::recv(Message *msg, handle h)
{
  if (h == preFilterHandle){
    PreProcessMessage(msg, h);
    return;
  }

  if (h == postFilterHandle){
    PostProcessMessage(msg, h);
    return;
  }

  diffPrint(DEBUG_ALWAYS, "Error: Received a message for the unknown handle %d !\n", h);
}

void GeoRoutingFilter::PreProcessMessage(Message *msg, handle h)
{
  NRSimpleAttribute<void *> *geo_blob_attr_p = NULL;
  NRSimpleAttribute<int> *geo_beacon_type_p = NULL;
  NRSimpleAttribute<double> *geo_attr_p = NULL;
  double geo_energy = 0.0;
  double geo_x = 0.0;
  double geo_y = 0.0;
  double new_h = 0.0;
  int geo_beacon_type, geo_neighbor_id;
  H_value *h_val_p = NULL;
  H_value *h_val = NULL;
  struct timeval tv; 
  GeoLocation dst_loc;
  TimerType *timer = NULL;
  NRAttrVec *attrs = NULL;
  Message *beacon_msg = NULL;
  Pkt_Header *pkt_header = NULL;

  switch (msg->msg_type){

  case INTEREST:

    // Check if neighbors info is outdated
    if (CheckNeighborsInfo()){
      SendNeighborRequest(0, msg);
    }

    // Take out the header
    pkt_header = PreProcessInterest(msg);

    // If we have a packet header, add it to the message list
    if (pkt_header)
      message_list.push_back(pkt_header);

    ((DiffusionRouting *)dr)->sendMessage(msg, h);    

    break;

  case DATA:

    // Check if message is a beacon
    geo_beacon_type_p = GeoBeaconTypeAttr.find(msg->msg_attr_vec);
    if (geo_beacon_type_p){
      diffPrint(DEBUG_IMPORTANT, "GEO: Receive a beacon...\n");
      // Check if it's a beacon request or reply
      geo_beacon_type = geo_beacon_type_p->getVal();

      geo_attr_p = GeoLongitudeAttr.find(msg->msg_attr_vec);
      if (geo_attr_p){
	geo_x = geo_attr_p->getVal();
      }

      geo_attr_p = GeoLatitudeAttr.find(msg->msg_attr_vec);
      if (geo_attr_p){
	geo_y = geo_attr_p->getVal();
      }

      geo_attr_p = GeoRemainingEnergyAttr.find(msg->msg_attr_vec);
      if (geo_attr_p){
	geo_energy = geo_attr_p->getVal();
      }

      // Get neighbor id and time
      geo_neighbor_id = msg->last_hop;
      getTime(&tv);

      diffPrint(DEBUG_IMPORTANT, "GEO: %d - %f, %f, %f\n", geo_neighbor_id,
		geo_x, geo_y, geo_energy);

      UpdateNeighbor(geo_neighbor_id, geo_x, geo_y, geo_energy); 

      if (geo_beacon_type == GEO_REQUEST){
	diffPrint(DEBUG_IMPORTANT, "GEO: Received a beacon request..\n");
	// If we received a neighbor request, we have to generate a reply and send it
	// after some random time. But this has to be suppresed if we have recently
	// generated a beacon reply (since for now we use broadcast beacons)

	if (beacon_tv.tv_sec > 0){
	  // Send at most one neighbor beacon every GEO_BEACON_PERIOD seconds
	  if ((tv.tv_sec - beacon_tv.tv_sec) < GEO_BEACON_PERIOD){
	    diffPrint(DEBUG_IMPORTANT, "GEO: Ignore beacon request !\n");
	    break;
	  }
	}

	// Send out a beacon
	timer = new TimerType(MESSAGE_SEND_TIMER);
	attrs = new NRAttrVec;

	attrs->push_back(GeoLongitudeAttr.make(NRAttribute::IS, geo_longitude));
	attrs->push_back(GeoLatitudeAttr.make(NRAttribute::IS, geo_latitude));
	attrs->push_back(GeoRemainingEnergyAttr.make(NRAttribute::IS, GeoRemainingEnergy()));
	attrs->push_back(GeoBeaconTypeAttr.make(NRAttribute::IS, GEO_REPLY));

	// Add h_value BEGIN

	// First we extract out the h_value;
	geo_blob_attr_p = GeoH_valueAttr.find(msg->msg_attr_vec);

	if (geo_blob_attr_p){
	  h_val_p = (H_value *) geo_blob_attr_p->getVal();
	  dst_loc.x = h_val_p->dst_x;
	  dst_loc.y = h_val_p->dst_y;

	  // Retrieve h_value
	  diffPrint(DEBUG_IMPORTANT, "GEO: Request h-value in BEACON_REQUEST, (%lf, %lf)\n",
		    dst_loc.x, dst_loc.y); 

	  new_h = RetrieveH_Value(dst_loc);
	  if (new_h != INITIAL_H_VALUE){
	    // Put h_value in
	    
	    h_val = new H_value;
	    h_val->dst_x = dst_loc.x;
	    h_val->dst_y = dst_loc.y;
	    h_val->h_val = new_h;

	    diffPrint(DEBUG_IMPORTANT, "GEO: Generated h-value in BEACON_REPLY,(%lf, %lf):%lf\n",
		      dst_loc.x, dst_loc.y, new_h); 
	    attrs->push_back(GeoH_valueAttr.make(NRAttribute::IS, (void *) h_val, sizeof(H_value)));
	  }
	}

	// Add h_value END
	
	beacon_msg = new Message(DIFFUSION_VERSION, DATA, 0, 0, attrs->size(),
				 pkt_count, rdm_id, msg->last_hop, LOCALHOST_ADDR);

	// For now, send this message to broadcast
	beacon_msg->last_hop = LOCALHOST_ADDR;
	beacon_msg->next_hop = BROADCAST_ADDR;

	// Don't forget to update pkt_count
	pkt_count++;
	beacon_msg->msg_attr_vec = CopyAttrs(attrs);

	timer->param = (void *) CopyMessage(beacon_msg);

	((DiffusionRouting *)dr)->addTimer(GEO_BEACON_GENERATE_DELAY +
					   (int) ((GEO_BEACON_GENERATE_JITTER *
						   (getRand() * 1.0 / RAND_MAX) -
						   (GEO_BEACON_GENERATE_JITTER / 2))),
					   (void *) timer, tcb);

	ClearAttrs(attrs);
	delete attrs;
	delete beacon_msg;

	diffPrint(DEBUG_IMPORTANT, "GEO: Generated a beacon reply in response to beacon request !\n");
	diffPrint(DEBUG_IMPORTANT, "GEO: %f, %f, %f\n", geo_longitude, geo_latitude,
		  GeoRemainingEnergy());

	// Update beacon timestamp
	beacon_tv.tv_sec= tv.tv_sec;
	beacon_tv.tv_usec= tv.tv_usec;
      }
      else if ((geo_beacon_type == GEO_REPLY) || (geo_beacon_type == GEO_H_VALUE_UPDATE)){
	diffPrint(DEBUG_IMPORTANT, "GEO: Received a beacon reply/update ..\n");

	// Update h_value

	if (geo_beacon_type == GEO_REPLY)
	  diffPrint(DEBUG_IMPORTANT, "GEO: Received a beacon reply\n");
	if (geo_beacon_type == GEO_H_VALUE_UPDATE)
	  diffPrint(DEBUG_IMPORTANT, "GEO: Received a h_value update\n");

	// Extract the h_value out
	geo_blob_attr_p = GeoH_valueAttr.find(msg->msg_attr_vec);

	if (geo_blob_attr_p){
	  h_val_p = (H_value *) geo_blob_attr_p->getVal();

	  // Update learned cost value table
	  dst_loc.x = h_val_p->dst_x;
	  dst_loc.y = h_val_p->dst_y;

	  diffPrint(DEBUG_IMPORTANT, "GEO: Received h-value update %d: (%f, %f ): %f\n",
		    geo_neighbor_id, dst_loc.x, dst_loc.y, h_val_p->h_val);
	  learned_cost_table.UpdateEntry(geo_neighbor_id, dst_loc, h_val_p->h_val);
	}
      }
    }
    else{
      // If this is not a geo_beacon_type data, pass it directly to the next filter
      ((DiffusionRouting *)dr)->sendMessage(msg, h);
    }

    break;

  default:

    // Otherwise, just pass the message to the next filter
    ((DiffusionRouting *)dr)->sendMessage(msg, h);

    break;

  }
}

void GeoRoutingFilter::PostProcessMessage(Message *msg, handle h)
{
  Pkt_Header *pkt_header = NULL;
  Geo_Header *geo_header = NULL;
  int32_t next_hop;

  switch (msg->msg_type){

  case INTEREST:

    if (msg->next_hop != (int)LOCALHOST_ADDR){
      // Retrieve packet header from previous stage
      pkt_header = RetrievePacketHeader(msg);

      if (pkt_header){
	geo_header = new Geo_Header;
	CopyGeoHeader(msg, geo_header, pkt_header);
	diffPrint(DEBUG_IMPORTANT, "GEO: sizeof(Geo_Header): %d\n", sizeof(Geo_Header));

	// Add extra attribute to the interest message
	msg->msg_attr_vec->push_back(GeoHeaderAttr.make(NRAttribute::IS, (void *) geo_header,
							sizeof(Geo_Header)));

	// Delete both geo_header and pkt_header
	delete pkt_header;
	delete geo_header;

	DeleteOldNeighbors();

	next_hop = GeoFindNextHop(msg);
	msg->last_hop = LOCALHOST_ADDR;

	switch (next_hop){

	case BROADCAST:
	  diffPrint(DEBUG_IMPORTANT, "GEO: BROADCAST !\n");
	  msg->next_hop = BROADCAST_ADDR;
	  diffPrint(DEBUG_IMPORTANT, "GEO: Interest: %d\n", msg->last_hop);
	  ((DiffusionRouting *)dr)->sendMessage(msg, h);

	  break;

	case BROADCAST_SUPPRESS:

	  diffPrint(DEBUG_IMPORTANT, "GEO: BROADCAST_SUPPRESS !\n");

	  break;

	case  FAIL:

	  diffPrint(DEBUG_IMPORTANT, "GEO: NEXT HOP FAILS\n");

	  break;

	default:
	
	  msg->next_hop = next_hop;
	  diffPrint(DEBUG_IMPORTANT, "GEO: Next Hop: %d\n", next_hop);
	  diffPrint(DEBUG_IMPORTANT, "GEO: Interest: %d\n", msg->last_hop);
	  ((DiffusionRouting *)dr)->sendMessage(msg, h);

	  break;
	}
      }
      else{
	// This message has no packet header information, so we just forward it
	diffPrint(DEBUG_IMPORTANT, "GEO: Forwarding message without packet header info !\n");
	((DiffusionRouting *)dr)->sendMessage(msg, h);
      }
    }
    else{
      // This is a message from gradient to a local app
      ((DiffusionRouting *)dr)->sendMessage(msg, h);
    }

    break;

  default:

    // There should not be any other types of messages handled by this function
    diffPrint(DEBUG_ALWAYS, "GEO: Non-Interest message in post process stage !\n");
    ((DiffusionRouting *)dr)->sendMessage(msg, h);

    break;
  }
}

handle GeoRoutingFilter::setupPreFilter()
{
  NRAttrVec attrs;
  handle h;

  // This is a filter that matches everything coming in
  attrs.push_back(NRClassAttr.make(NRAttribute::IS, NRAttribute::INTEREST_CLASS));

  h = ((DiffusionRouting *)dr)->addFilter(&attrs, GEOROUTING_FILTER_PRE_PRIORITY, fcb);

  ClearAttrs(&attrs);
  return h;
}

handle GeoRoutingFilter::setupPostFilter()
{
  NRAttrVec attrs;
  handle h;

  // This is a filter that matches all interest messages
  attrs.push_back(NRClassAttr.make(NRAttribute::EQ, NRAttribute::INTEREST_CLASS));

  h = ((DiffusionRouting *)dr)->addFilter(&attrs, GEOROUTING_FILTER_POST_PRIORITY, fcb);

  ClearAttrs(&attrs);
  return h;
}

void GeoRoutingFilter::run()
{
#ifdef NS_DIFFUSION
  // Set up filters
  preFilterHandle = setupPreFilter();
  postFilterHandle = setupPostFilter();

  diffPrint(DEBUG_ALWAYS, "GEAR Received handle %d for pre Filter !\n", preFilterHandle);
  diffPrint(DEBUG_ALWAYS, "GEAR Received handle %d for post Filter !\n", postFilterHandle);
  diffPrint(DEBUG_ALWAYS, "GEAR Initialized !\n");
#else
  // We don't do anything
  while (1){
    sleep(1000);
  }
#endif // NS_DIFFUSION
}

#ifdef NS_DIFFUSION
GeoRoutingFilter::GeoRoutingFilter()
#else
GeoRoutingFilter::GeoRoutingFilter(int argc, char **argv)
#endif // NS_DIFFUSION
{
  struct timeval tv;

#ifndef NS_DIFFUSION
  // Parse command line options
  ParseCommandLine(argc, argv);
#endif // !NS_DIFFUSION

  // Initialize a few parameters
  initial_energy = GEO_INITIAL_ENERGY;
  unit_energy_for_send = GEO_UNIT_ENERGY_FOR_SEND;
  unit_energy_for_recv = GEO_UNIT_ENERGY_FOR_RECV;
  num_pkt_sent = 0;
  num_pkt_recv = 0;

  // Initialize beacon timestamp
  beacon_tv.tv_sec = 0; 
  beacon_tv.tv_usec = 0; 
  neighbor_req_tv.tv_sec = 0;
  neighbor_req_tv.tv_usec = 0;

  GetNodeLocation(&geo_longitude, &geo_latitude);

  diffPrint(DEBUG_ALWAYS, "GEAR: Location: (%f, %f)\n", geo_longitude, geo_latitude);

  getTime(&tv);
  setSeed(&tv);
  pkt_count = getRand();
  rdm_id = getRand();

#ifndef NS_DIFFUSION
  // Create Diffusion Routing class
  dr = NR::createNR(diffusion_port);
#endif // !NS_DIFFUSION

  fcb = new GeoFilterReceive(this);
  tcb = new GeoTimerReceive(this);

#ifndef NS_DIFFUSION
  // Set up filters
  preFilterHandle = setupPreFilter();
  postFilterHandle = setupPostFilter();

  diffPrint(DEBUG_ALWAYS, "GEAR Received handle %d for pre Filter !\n", preFilterHandle);
  diffPrint(DEBUG_ALWAYS, "GEAR Received handle %d for post Filter !\n", postFilterHandle);
  diffPrint(DEBUG_ALWAYS, "GEAR Initialized !\n");
#endif // !NS_DIFFUSION
}

void GeoRoutingFilter::GetNodeLocation(double *x, double *y)
{
#ifdef NS_DIFFUSION
  double z;

  node_->getLoc(x, y, &z);
#else
  char *x_env, *y_env;

  x_env = getenv("gear_x");
  y_env = getenv("gear_y");

  if (x_env && y_env){
    *x = atof(x_env);
    *y = atof(y_env);
  }
  else{
    diffPrint(DEBUG_ALWAYS, "Error: Cannot get location from (gear_x, gear_y) !\n");
    exit(-1);
  }
#endif // NS_DIFFUSION
}

void GeoRoutingFilter::SendNeighborRequest(int32_t neighbor_id, Message *msg)
{
  NRAttrVec *attrs;
  struct timeval tv;
  NRSimpleAttribute<void *> *geo_header_p;
  Geo_Header *geo_h_val_p;
  float x1, x2, y1, y2;
  Message *beacon_msg;
  TimerType *timer;
  H_value *h_val;
  GeoLocation dst;

  getTime(&tv);

  neighbor_req_tv.tv_sec = tv.tv_sec;
  neighbor_req_tv.tv_usec = tv.tv_usec;

  attrs = new NRAttrVec;
  attrs->push_back(GeoBeaconTypeAttr.make(NRAttribute::IS, GEO_REQUEST));
  attrs->push_back(GeoLatitudeAttr.make(NRAttribute::IS, geo_latitude));
  attrs->push_back(GeoLongitudeAttr.make(NRAttribute::IS, geo_longitude));
  attrs->push_back(GeoRemainingEnergyAttr.make(NRAttribute::IS, GeoRemainingEnergy()));
	
  // Also ask for h_value for this destination

  // H_VALUE BEGIN
  geo_header_p = GeoHeaderAttr.find(msg->msg_attr_vec);

  if (geo_header_p){
    // The message includes geo_header
    geo_h_val_p = (Geo_Header *)(geo_header_p->getVal());

    if (geo_h_val_p->pkt_type == UNICAST_ORI)
      dst = geo_h_val_p->dst_region.center;
    else
      if (geo_h_val_p->pkt_type == UNICAST_SUB)
	dst = geo_h_val_p->sub_region.center;
  }
  else{
    diffPrint(DEBUG_IMPORTANT, "GEO: A raw interest w/o geo_header coming from a local application !\n" );
    // This is a fresh interest coming from a local app. geo_header
    // has not been added to it yet..  We need to extract the target
    // location info from the location attribute in the original
    // interest..  Here, (x1, y1), (x2, y2) are border points

    if (ExtractLocation(msg, &x1, &x2, &y1, &y2)){
      dst.x = (x1 + x2) / 2;
      dst.y = (y1 + y2) / 2;

      diffPrint(DEBUG_IMPORTANT, "GEO: Extract location: %f, %f, %f, %f, (%f, %f)\n", x1, x2, y1, y2,
		dst.x, dst.y);

      // Ask for h_value
      h_val = new H_value;
      h_val->dst_x = dst.x;
      h_val->dst_y = dst.y;
      h_val->h_val = INITIAL_H_VALUE;

      diffPrint(DEBUG_IMPORTANT, "GEO: Asking for h-value (%lf, %lf): %lf\n",
		dst.x, dst.y, h_val->h_val);
      attrs->push_back(GeoH_valueAttr.make(NRAttribute::IS, (void *) h_val,
					   sizeof(H_value)));

      // Delete h_value structure
      delete h_val;
    }

    // H_VALUE END

    // Neighbor beacon msg is a DATA message
    beacon_msg = new Message(DIFFUSION_VERSION, DATA, 0, 0,
			     attrs->size(), pkt_count, rdm_id, neighbor_id,
			     LOCALHOST_ADDR);
    // Don't forget to increment pkt_count
    pkt_count++;

    beacon_msg->msg_attr_vec = CopyAttrs(attrs);

    // For now, send message to all neighbors
    beacon_msg->last_hop = LOCALHOST_ADDR;
    beacon_msg->next_hop = BROADCAST_ADDR;

    // Generate a beacon request, add some random jitter before
    // actually sending it
    diffPrint(DEBUG_IMPORTANT, "GEO: Broadcast neighbor info request...\n");

    timer = new TimerType(MESSAGE_SEND_TIMER);
    timer->param = (void *) CopyMessage(beacon_msg);
 
    ((DiffusionRouting *)dr)->addTimer(GEO_BEACON_REQUEST_DELAY +
				       (int) ((GEO_BEACON_REQUEST_JITTER *
					       (getRand() * 1.0 / RAND_MAX) -
					       (GEO_BEACON_REQUEST_JITTER / 2))),
				       (void *) timer, tcb);

    ClearAttrs(attrs);
    delete attrs;
    delete beacon_msg;
  }
}

//use the info. in the incoming beacon to update neighbor list.
void GeoRoutingFilter::UpdateNeighbor(int neighbor_id, double x, double y,
				      double energy)
{
  Neighbors_Hash::iterator itr;
  Neighbor_Entry *entry;

  if (neighbor_id < 0){
    diffPrint(DEBUG_ALWAYS, "GEO: Invalid Neighbor ID !\n");
    return;
  }

  itr = neighbors_table.find(neighbor_id);

  if (itr == neighbors_table.end()){
    // Insert a new neighbor into the hash table
    diffPrint(DEBUG_IMPORTANT, "GEO: Inserting a new neighbor %d !\n",
	      neighbor_id);

    entry = new Neighbor_Entry;
    entry->id = neighbor_id;
    if (x >= 0)
      entry->longitude = x;
    if (y >= 0)
      entry->latitude = y;
    if (energy > -1)
      entry->remaining_energy = energy;

    getTime(&(entry->tv));

    // Insert new element with key neighbor_id into the hash map
    neighbors_table[neighbor_id] = entry;
  }
  else{
    // Update an old neighbor entry
    (*itr).second->id = neighbor_id;
    if (x >= 0)
      (*itr).second->longitude = x;
    if (y >= 0)
      (*itr).second->latitude = y;
    if (energy > -1)
      (*itr).second->remaining_energy = energy;

    getTime(&(*itr).second->tv);
  }
}

bool GeoRoutingFilter::CheckNeighborsInfo()
{
  // Returns true if there's at least one neighbor whose info is not up to date	
  Neighbors_Hash::iterator itr;
  struct timeval tv; 
  int num_neighbors = 0;

  getTime(&tv);

  for (itr = neighbors_table.begin(); itr != neighbors_table.end(); ++itr){
    num_neighbors++;

    // Compare timestamp with GEO_NEIGHBOR_OUTDATE_TIMEOUT
    if ((tv.tv_sec - (*itr).second->tv.tv_sec) >= GEO_NEIGHBOR_OUTDATE_TIMEOUT)
      return true;
  }

  // Also, if neighbor's table is empty, that may mean we just started
  if (num_neighbors == 0)
    return true;

  return false;
}
 
Pkt_Header * GeoRoutingFilter::RetrievePacketHeader(Message *msg)
{
  Packets_List::iterator itr;
  Pkt_Header *pkt_header = NULL;

  for (itr = message_list.begin(); itr != message_list.end(); ++itr){
    pkt_header = *itr;
    if ((pkt_header->rdm_id == msg->rdm_id) && (pkt_header->pkt_num == msg->pkt_num)){
      itr = message_list.erase(itr);
      return pkt_header;
    }
  }

  // Entry not found
  return NULL;
}

Pkt_Header * GeoRoutingFilter::PreProcessInterest(Message *msg)
  {
  // This function takes out the extra field in interest messages
  float x1, x2, y1, y2;
  Pkt_Header *pkt_header = NULL;

  pkt_header = StripOutHeader(msg);

  if (!pkt_header){
    // This is an interest message coming from a local application
    // So, we must compose the geo_header from the raw msg by
    // trying to extract geographic information from this interest

    if (ExtractLocation(msg, &x1, &x2, &y1, &y2)){
      diffPrint(DEBUG_IMPORTANT, "GEO: NEW x--[%f, %f], y--[%f, %f]\n", x1, x2, y1, y2);

      // Create new Packet Header
      pkt_header = new Pkt_Header;

      // Fill in the new packet header
      pkt_header->pkt_num = msg->pkt_num;
      pkt_header->rdm_id = msg->rdm_id;
      pkt_header->prev_hop = msg->last_hop;
      pkt_header->path_len = 0;
      pkt_header->scope = DEFAULT_SCOPE;
      pkt_header->pkt_type = UNICAST_ORI;

      pkt_header->dst_region.center.x = (x1 + x2) / 2;
      pkt_header->dst_region.center.y = (y1 + y2) / 2;

      diffPrint(DEBUG_IMPORTANT, "GEO: ExtractLocation2 %f, %f, %f, %f, (%f, %f)\n", x1, x2, y1, y2,
		pkt_header->dst_region.center.x, pkt_header->dst_region.center.y);

      pkt_header->dst_region.radius = Distance(x1, y1, x2, y2) / 2;

      pkt_header->sub_region.center.x = pkt_header->dst_region.center.x;
      pkt_header->sub_region.center.y = pkt_header->dst_region.center.y;
      pkt_header->sub_region.radius = pkt_header->dst_region.radius;
      pkt_header->mode = GREEDY_MODE;
      pkt_header->greedy_failed_dist = 0;
    }
  }

  return pkt_header;
}

Pkt_Header * GeoRoutingFilter::StripOutHeader(Message *msg)
{
  // If the interest message has a geo_header, strip it out and put it the pkt_header
  NRSimpleAttribute<void *> *geo_header_p;
  Geo_Header *geo_h_val_p;
  Pkt_Header *pkt_header = NULL;

  geo_header_p = GeoHeaderAttr.find(msg->msg_attr_vec);

  if (geo_header_p){
    geo_h_val_p = (Geo_Header *)(geo_header_p->getVal());

    // Create Packet Header structure
    pkt_header = new Pkt_Header;

    pkt_header->pkt_num = msg->pkt_num;
    pkt_header->rdm_id = msg->rdm_id;
    pkt_header->prev_hop = msg->last_hop;

    // Copy the msg from Geo_header to Pkt_Header
    pkt_header->path_len = geo_h_val_p->path_len;
    pkt_header->scope = geo_h_val_p->scope;
    pkt_header->pkt_type = geo_h_val_p->pkt_type;
    pkt_header->mode = geo_h_val_p->mode;
    pkt_header->greedy_failed_dist = geo_h_val_p->greedy_failed_dist;
    pkt_header->dst_region.center.x = geo_h_val_p->dst_region.center.x;
    pkt_header->dst_region.center.y = geo_h_val_p->dst_region.center.y;
    pkt_header->dst_region.radius = geo_h_val_p->dst_region.radius;

    pkt_header->sub_region.center.x = geo_h_val_p->sub_region.center.x;
    pkt_header->sub_region.center.y = geo_h_val_p->sub_region.center.y;
    pkt_header->sub_region.radius = geo_h_val_p->sub_region.radius;

    diffPrint(DEBUG_IMPORTANT, "GEO: Extra interest field:%d\n", geo_h_val_p->mode);

    TakeOutAttr(msg->msg_attr_vec, GEO_HEADER_KEY);
    delete geo_header_p;
  }
  else{
    diffPrint(DEBUG_IMPORTANT, "GEO: No extra geo_interest field\n");
  }
  return pkt_header;
}

void GeoRoutingFilter::TakeOutAttr(NRAttrVec *attrs, int32_t key)
{
  NRAttrVec::iterator itr;

  for (itr = attrs->begin(); itr != attrs->end(); ++itr){
    if ((*itr)->getKey() == key){
      break;
    }
  }

  if (itr != attrs->end())
    attrs->erase(itr);
}

bool GeoRoutingFilter::ExtractLocation(Message *msg, float *x1, float *x2, float *y1, float *y2)
{
  // Extract geographic information from a message sent by an application
  NRSimpleAttribute<float> *latAttr;
  NRSimpleAttribute<float> *longAttr;
  NRAttrVec::iterator itr;
  NRAttrVec *attrs;
  bool has_x1 = false;
  bool has_x2 = false;
  bool has_y1 = false;
  bool has_y2 = false;

  attrs = msg->msg_attr_vec;

  // Extract y coordinates of the target region from the interest
  itr = attrs->begin();
  for (;;){
    longAttr = LongitudeAttr.find_from(attrs, itr, &itr);
    if (!longAttr)
      break;
    if (longAttr->getOp() == NRAttribute::GT){
      has_y1 = true;
      *y1 = longAttr->getVal();
    }
    if (longAttr->getOp() == NRAttribute::LE){
      has_y2 = true;
      *y2 = longAttr->getVal();
    }

    itr++; // Avoid infinite loops
  }

  // Extract x coordinates of the target region from the interest
  itr = attrs->begin();
  for (;;){
    latAttr = LatitudeAttr.find_from(attrs, itr, &itr);
    if (!latAttr)
      break;
    if (latAttr->getOp() == NRAttribute::GT){
      has_x1 = true;
      *x1 = latAttr->getVal();
    }
    if (latAttr->getOp() == NRAttribute::LE){
      has_x2 = true;
      *x2 = latAttr->getVal();
    }
    itr++; // Avoid infinite loops
  }

  if (has_x1 && has_x2 && has_y1 && has_y2){
    diffPrint(DEBUG_IMPORTANT, "GEO: In ExtractLocation %lf, %lf, %lf, %lf\n", *x1, *x2, *y1, *y2);
    return true;
  }
  return false;
}

void GeoRoutingFilter::CopyGeoHeader(Message *msg, Geo_Header *geo_header, Pkt_Header *pkt_header)
{
  // Fill in values from pkt_header
  msg->last_hop = pkt_header->prev_hop;

  geo_header->path_len = pkt_header->path_len;
  geo_header->scope = pkt_header->scope;
  geo_header->pkt_type = pkt_header->pkt_type;
  geo_header->mode = pkt_header->mode;
  geo_header->greedy_failed_dist = pkt_header->greedy_failed_dist;
  geo_header->dst_region.center.x = pkt_header->dst_region.center.x;
  geo_header->dst_region.center.y = pkt_header->dst_region.center.y;
  geo_header->dst_region.radius = pkt_header->dst_region.radius;

  geo_header->sub_region.center.x = pkt_header->sub_region.center.x;
  geo_header->sub_region.center.y = pkt_header->sub_region.center.y;
  geo_header->sub_region.radius = pkt_header->sub_region.radius;
}


int GeoRoutingFilter::NavigateHole(Geo_Header *pkt)
{
  Neighbors_Hash::iterator itr;
  GeoLocation cur_loc, dst, neighbor_loc, min_neighbor_loc;
  double cur_dist, distance, gap;
  double cur_l_cost, min_l_cost;
  int min_cost_id, neighbor_id;
  double min_dist;
  int num_neighbors;
  double new_h;

  cur_loc.x = geo_longitude;
  cur_loc.y = geo_latitude;

  if (pkt->pkt_type == UNICAST_ORI)
    dst = pkt->dst_region.center;
  else
    if (pkt->pkt_type == UNICAST_SUB)
      dst = pkt->sub_region.center;

  cur_dist = dist(cur_loc, dst);

  min_dist = MAX_INT;
  min_l_cost = MAX_INT;
  num_neighbors = 0;

  for (itr = neighbors_table.begin(); itr != neighbors_table.end(); ++itr){
    num_neighbors++;

    neighbor_loc.x = (*itr).second->longitude;
    neighbor_loc.y = (*itr).second->latitude;

    neighbor_id = (*itr).second->id;
    cur_l_cost = RetrieveLearnedCost(neighbor_id, dst);

    distance = dist(neighbor_loc, dst);

    diffPrint(DEBUG_IMPORTANT, "Neighbor: %d: l_cost = %f, min_l_cost = %f\n",
	      neighbor_id, cur_l_cost, min_l_cost);

    if (cur_l_cost < min_l_cost){
      min_l_cost = cur_l_cost;
      min_cost_id = (*itr).second->id;
      min_neighbor_loc.x = neighbor_loc.x;
      min_neighbor_loc.y = neighbor_loc.y;
    }
  }

  diffPrint(DEBUG_IMPORTANT, "GEO: # of neighbors: %d; cur: (%f, %f), dst: (%f, %f) \n", num_neighbors, cur_loc.x, cur_loc.y, dst.x, dst.y);

  if (min_l_cost < MAX_INT){
    // Update my h_value
    gap = dist(min_neighbor_loc, cur_loc);
    // The gap is one hop cost from me to the picked nexthop neighbor
    new_h = min_l_cost + gap;
    if (h_value_table.UpdateEntry(dst, new_h))
      BroadcastH_Value(dst, new_h, 0);
    return min_cost_id;
  }
  return FAIL;
}

int GeoRoutingFilter::GreedyNext(Geo_Header *pkt)
{
  Neighbors_Hash::iterator itr;
  GeoLocation cur_loc, dst, neighbor_loc, min_neighbor_loc;
  double distance, gap;
  double cur_l_cost, min_l_cost;
  int min_cost_id, neighbor_id;
  double cur_dist, min_dist;
  int num_neighbors;
  double new_h;

  cur_loc.x = geo_longitude;
  cur_loc.y = geo_latitude;

  if (pkt->pkt_type == UNICAST_ORI)
    dst = pkt->dst_region.center;
  else
    if (pkt->pkt_type == UNICAST_SUB)
      dst = pkt->sub_region.center;

  cur_dist = dist(cur_loc, dst);

  min_dist = MAX_INT;
  min_l_cost = MAX_INT;
  num_neighbors = 0;

  for (itr = neighbors_table.begin(); itr != neighbors_table.end(); ++itr){
    num_neighbors++;

    neighbor_loc.x = (*itr).second->longitude;
    neighbor_loc.y = (*itr).second->latitude;

    neighbor_id = (*itr).second->id;
    cur_l_cost = RetrieveLearnedCost(neighbor_id, dst);

    distance = dist(neighbor_loc, dst); // Calculate distance from this neighbor to dst

    if (distance > cur_dist)
      continue;

    diffPrint(DEBUG_IMPORTANT, "GEO: Neighbor: %d: l_cost = %f, min_l_cost = %f\n", neighbor_id, cur_l_cost, min_l_cost);

    if (cur_l_cost < min_l_cost){
      min_l_cost = cur_l_cost;
      min_cost_id = (*itr).second->id;
      min_neighbor_loc.x = neighbor_loc.x;
      min_neighbor_loc.y = neighbor_loc.y;
    }
  }

  diffPrint(DEBUG_IMPORTANT, "GEO: # of neighbors: %d; cur: (%f, %f), dst: (%f, %f) \n", num_neighbors, cur_loc.x, cur_loc.y, dst.x, dst.y);

  if (min_l_cost < MAX_INT){
    // Update my h_value
    gap = dist(min_neighbor_loc, cur_loc);
    // Gap is the geographical cost from me to the nexthop neighbor (min_cost_id) 
    new_h = min_l_cost + gap;
    if (h_value_table.UpdateEntry(dst, new_h))
      BroadcastH_Value(dst, new_h, 0);
    return min_cost_id;
  }
  return FAIL;
}

void GeoRoutingFilter::BroadcastH_Value(GeoLocation dst, double new_h, int to_id)
{
  NRAttrVec *attrs = new NRAttrVec;
  H_value *h_val = new H_value;
  Message *beacon_msg;
  TimerType *timer;

  attrs->push_back(GeoLongitudeAttr.make(NRAttribute::IS, geo_longitude));
  attrs->push_back(GeoLatitudeAttr.make(NRAttribute::IS, geo_latitude));
  attrs->push_back(GeoRemainingEnergyAttr.make(NRAttribute::IS, GeoRemainingEnergy()));
  attrs->push_back(GeoBeaconTypeAttr.make(NRAttribute::IS, GEO_H_VALUE_UPDATE));

  h_val->dst_x = dst.x;
  h_val->dst_y = dst.y;
  h_val->h_val = new_h;

  attrs->push_back(GeoH_valueAttr.make(NRAttribute::IS, (void *) h_val, sizeof(H_value)));

  beacon_msg = new Message(DIFFUSION_VERSION, DATA, 0, 0,
			   attrs->size(), pkt_count, rdm_id, to_id,
			   LOCALHOST_ADDR);

  // For now, send this message to broadcast
  beacon_msg->last_hop = LOCALHOST_ADDR;
  beacon_msg->next_hop = BROADCAST_ADDR;

  // Don't forget to increase pkt_count
  pkt_count++;

  beacon_msg->msg_attr_vec = CopyAttrs(attrs);

  // Now, we generate a beacon to broadcast triggered h-value update
  // but first we add some random jitter before actually sending it
  timer = new TimerType(MESSAGE_SEND_TIMER);

  timer->param = (void *) CopyMessage(beacon_msg);

  ((DiffusionRouting *)dr)->addTimer(GEO_BEACON_REQUEST_DELAY +
				     (int) ((GEO_BEACON_REQUEST_JITTER *
					     (getRand() * 1.0 / RAND_MAX) -
					     (GEO_BEACON_REQUEST_JITTER / 2))),
				     (void *) timer, tcb);

  // Delete everything we created here
  ClearAttrs(attrs);
  delete attrs;
  delete beacon_msg;
  delete h_val;
}

int32_t	GeoRoutingFilter::GeoFindNextHop(Message *msg)
{
  NRSimpleAttribute<void *> *geo_header_p;
  Geo_Header *geo_h_val_p;
  int32_t next_hop;
  int flood;

  // The interest message is ready to go out and has been filled with the geo_header field

  geo_header_p = GeoHeaderAttr.find(msg->msg_attr_vec);

  if (geo_header_p){
    geo_h_val_p = (Geo_Header *)(geo_header_p->getVal());
  }
  else{
    diffPrint(DEBUG_ALWAYS, "GEO: Error. Can't find geo_header !\n");
    return FAIL;
  }

  // Check if we have to broadcast this message. We do not need to check
  // if I have seen this broadcast packet before, since this is called
  // by PostProcessMessage, so it must be a new packet. We only need to
  // check if we are inside the target region

  flood = FloodInsideRegion(geo_h_val_p);

  if (flood == BROADCAST)
    return BROADCAST;
  else
    if (flood == BROADCAST_SUPPRESS)
      return  BROADCAST_SUPPRESS;

  // Greedy first
  next_hop = GreedyNext(geo_h_val_p);

  // If Greedy fails, use RTA* to navigate around holes
  if (next_hop == FAIL)
    next_hop = NavigateHole(geo_h_val_p);

  return next_hop;
}

int GeoRoutingFilter::FloodInsideRegion(Geo_Header *pkt)
{
  Neighbors_Hash::iterator itr;
  GeoLocation cur_loc, neighbor_loc;  
  GeoLocation dst;
  double radius, distance;

  if (pkt->pkt_type == UNICAST_ORI){
    dst = pkt->dst_region.center;
    radius = pkt->dst_region.radius;
  }
  else{
    if (pkt->pkt_type == UNICAST_SUB){
      dst = pkt->sub_region.center;
      radius = pkt->sub_region.radius;
    }
  }

  cur_loc.x = geo_longitude;
  cur_loc.y = geo_latitude;

  if (dist(cur_loc, dst) < radius){
    // We are inside the target region

    // Change its mode to BROADCAST
    pkt->pkt_type = BROADCAST_TYPE;

    // If all my neighbors are outside this region, suppress this broadcast message
    for (itr = neighbors_table.begin(); itr != neighbors_table.end(); ++itr){
      neighbor_loc.x = (*itr).second->longitude;
      neighbor_loc.y = (*itr).second->latitude;

      // Calculate distance between neighbor and dst
      distance = dist(neighbor_loc, dst);

      // As long as we have one neighbor inside the region, broadcast the message
      if  (distance < radius)
	return BROADCAST;
    }
    return BROADCAST_SUPPRESS;
  }

  if (pkt->pkt_type == BROADCAST_TYPE)
    // This is a BROADCAST pkt flooded outside the region
    return BROADCAST_SUPPRESS;

  // I am not inside the region, just a regular packet
  return FALSE;
}

double GeoRoutingFilter::dist(GeoLocation p1, GeoLocation p2)
{
  double dist, tmp;

  tmp = p1.x - p2.x;
  dist = tmp * tmp;
  tmp = p1.y - p2.y;
  dist += tmp * tmp;

  return sqrt(dist);
}

double GeoRoutingFilter::RetrieveLearnedCost(int neighbor_id, GeoLocation dst)
{
  int index;

  index = learned_cost_table.RetrieveEntry(neighbor_id, &dst);

  if (index != FAIL)
    return learned_cost_table.table[index].l_cost_value;
  else
    return EstimateCost(neighbor_id, dst);
}

double GeoRoutingFilter::EstimateCost(int neighbor_id, GeoLocation dst)
{
  double x_coor, y_coor;
  double distance;
  Neighbors_Hash::iterator itr;
  GeoLocation cur_loc;

  // To get this neighbor's location, we first find the entry with neighbor_id
  // Since right now it is pure geographical routing, the estimated cost is
  // just the distance between neighbor_id to dst.

  if (neighbor_id < 0){
    diffPrint(DEBUG_IMPORTANT, "GEO: Invalid neighbor ID !\n");
    return FAIL;
  }

  itr = neighbors_table.find(neighbor_id);

  if (itr == neighbors_table.end()){
    return FAIL;
  }

  x_coor = (*itr).second->longitude;
  y_coor = (*itr).second->latitude;

  cur_loc.x = x_coor;
  cur_loc.y = y_coor;

  distance = dist(cur_loc, dst);
  return distance;
}

//as long as there are one neighbor info outdated, return TRUE..
void GeoRoutingFilter::DeleteOldNeighbors()
{
  struct timeval tv;
  int num_neighbors = 0;
  Neighbors_Hash::iterator itr, itr2;
  bool deleted_neighbors = false;

  getTime(&tv);

  while (1){
    for (itr = neighbors_table.begin(); itr != neighbors_table.end(); ++itr){
      num_neighbors++;
      diffPrint(DEBUG_IMPORTANT, "GEO: Check: %d: %d --> %d\n",
		(*itr).second->id, (*itr).second->tv.tv_sec, tv.tv_sec);

      if ((tv.tv_sec - (*itr).second->tv.tv_sec) >= GEO_NEIGHBOR_TIMEOUT){
	// Delete timeout neighbors
	diffPrint(DEBUG_IMPORTANT, "GEO: Deleting neighbor %d\n",
		  (*itr).second->id);
	itr2 = itr;
	deleted_neighbors = true;
	break;
	// neighbors_table.erase(itr2);
      }
    }

    if (deleted_neighbors){
      // Erase the entry w/ key itr2
      neighbors_table.erase(itr2);
      deleted_neighbors = false;
    }
    else{
      break;
      // We did not find an entry to delete
    }
    // This is a stupid hack: since once an entry is deleted, it
    // messed up the pointer, so I use outer loop while (1) to restart
    // the search, to delete the other outdated entries.. in order to
    // solve this problem.. 
  }
}

double GeoRoutingFilter::RetrieveH_Value(GeoLocation dst)
{
  int index;

  index = h_value_table.RetrieveEntry(&dst);

  if (index != FAIL)
    return h_value_table.table[index].h_value;

  return INITIAL_H_VALUE;
}

#ifndef NS_DIFFUSION
int main(int argc, char **argv)
{
  GeoRoutingFilter *gr_app;

  gr_app = new GeoRoutingFilter(argc, argv);
  gr_app->run();

  return 0;
}
#endif // !NS_DIFFUSION

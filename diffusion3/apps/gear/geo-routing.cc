//
// geo-routing.cc : GEAR Filter
// authors        : Yan Yu and Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// Copyright (C) 2000-2002 by the University of California
// $Id: geo-routing.cc,v 1.15 2003/03/10 18:27:13 haldar Exp $
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//
//

#include "geo-routing.hh"

#ifdef NS_DIFFUSION
static class GeoRoutingFilterClass : public TclClass {
public:
  GeoRoutingFilterClass() : TclClass("Application/DiffApp/GeoRoutingFilter") {}
  TclObject * create(int argc, const char*const* argv) {
    return (new GeoRoutingFilter());
  }
} class_geo_routing_filter;

int GeoRoutingFilter::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "start") == 0) {
      run();
      return TCL_OK;
    }
  }
  return DiffApp::command(argc, argv);
}
#endif // NS_DIFFUSION

void GeoFilterReceive::recv(Message *msg, handle h)
{
  app_->recv(msg, h);
}

int GeoMessageSendTimer::expire()
{
  // Call timeout function
  agent_->messageTimeout(msg_);

  // Do not reschedule this timer
  return -1;
}

int GeoNeighborsTimer::expire()
{
  // Call timeout function
  agent_->neighborTimeout();

  // Reschedule this timer
  return 0;
}

int GeoBeaconRequestTimer::expire()
{
  // Call the timeout function
  agent_->beaconTimeout();

  // Reschedule this timer
  return 0;
}

void GeoRoutingFilter::beaconTimeout()
{
  NRAttrVec attrs;
  Message *beacon_msg;
  struct timeval tv;

  GetTime(&tv);

  DiffPrint(DEBUG_IMPORTANT, "Gear: %d.%06d - Beacon Timeout !\n",
	    tv.tv_sec, tv.tv_usec);

  // We broadcast the request from time to time, in case a new
  // neighbor joins in and never gets a chance to get its informaiton

  if ((last_neighbor_request_tv_.tv_sec == 0) ||
      ((tv.tv_sec - last_neighbor_request_tv_.tv_sec) >=
       GEO_NEIGHBOR_REQUEST_PERIOD)){

    // Update timestamp
    GetTime(&last_neighbor_request_tv_);

    // Sends a beacon to all neighbors
    attrs.push_back(GeoBeaconTypeAttr.make(NRAttribute::IS, GEO_REQUEST));
    attrs.push_back(GeoLatitudeAttr.make(NRAttribute::IS, geo_latitude_));
    attrs.push_back(GeoLongitudeAttr.make(NRAttribute::IS, geo_longitude_));
    attrs.push_back(GeoRemainingEnergyAttr.make(NRAttribute::IS,
						remainingEnergy()));

    beacon_msg = new Message(DIFFUSION_VERSION, DATA, 0, 0,
			     attrs.size(), pkt_count_, rdm_id_,
			     BROADCAST_ADDR, LOCALHOST_ADDR);
    // Don't forget to increment pkt_count
    pkt_count_++;

    beacon_msg->msg_attr_vec_ = CopyAttrs(&attrs);

    DiffPrint(DEBUG_IMPORTANT, "Gear: %d.%06d - Sending Beacon Request...\n",
	      tv.tv_sec, tv.tv_usec);
    ((DiffusionRouting *)dr_)->sendMessage(beacon_msg, post_filter_handle_);

    ClearAttrs(&attrs);
    delete beacon_msg;
  }
}

void GeoRoutingFilter::neighborTimeout()
{
  NeighborList::iterator neighbor_itr;
  NeighborEntry *neighbor_entry;
  struct timeval tv;

  GetTime(&tv);

  DiffPrint(DEBUG_IMPORTANT, "Gear: %d.%06d - Neighbors Timeout !\n",
	    tv.tv_sec, tv.tv_usec);

  neighbor_itr = neighbors_list_.begin();

  while (neighbor_itr != neighbors_list_.end()){
    neighbor_entry = *neighbor_itr;
    DiffPrint(DEBUG_IMPORTANT, "Gear: %d.%06d - Check: %d: %d --> %d\n",
	      tv.tv_sec, tv.tv_usec, neighbor_entry->id_,
	      neighbor_entry->tv_.tv_sec, tv.tv_sec);

    if ((tv.tv_sec - neighbor_entry->tv_.tv_sec) >= GEO_NEIGHBOR_EXPIRED){
      // Delete timed-out neighbor
      DiffPrint(DEBUG_IMPORTANT, "Gear: %d.%06d - Deleting neighbor %d\n",
		tv.tv_sec, tv.tv_usec, neighbor_entry->id_);
      neighbor_itr = neighbors_list_.erase(neighbor_itr);
      delete neighbor_entry;
    }
    else{
      DiffPrint(DEBUG_IMPORTANT, "Gear: %d.%06d - Neighbor %d, (%d,%d)\n",
		tv.tv_sec, tv.tv_usec, neighbor_entry->id_,
		neighbor_entry->tv_.tv_sec, neighbor_entry->tv_.tv_usec);
      neighbor_itr++;
    }
  }
}

void GeoRoutingFilter::messageTimeout(Message *msg)
{
  struct timeval tv;

  // Get current time
  GetTime(&tv);

  // Send message
  DiffPrint(DEBUG_IMPORTANT, "Gear: %d.%06d - Message Timeout !\n",
	    tv.tv_sec, tv.tv_usec);

  ((DiffusionRouting *)dr_)->sendMessage(msg, post_filter_handle_,
					 GEOROUTING_POST_FILTER_PRIORITY);
}

void GeoRoutingFilter::recv(Message *msg, handle h)
{
  struct timeval tv;

  GetTime(&tv);

  if (h == pre_filter_handle_){
    preProcessFilter(msg);
    return;
  }

  if (h == post_filter_handle_){
    postProcessFilter(msg);
    return;
  }

  DiffPrint(DEBUG_ALWAYS,
	    "Gear: %d.%06d - Received message for an unknown handle %d !\n",
	    tv.tv_sec, tv.tv_usec, h);
}

void GeoRoutingFilter::preProcessFilter(Message *msg)
{
  NRSimpleAttribute<void *> *heuristic_value_attribute = NULL;
  NRSimpleAttribute<int> *beacon_type_attribute = NULL;
  NRSimpleAttribute<double> *beacon_info_attribute = NULL;
  double neighbor_energy = 0.0;
  double neighbor_longitude = 0.0;
  double neighbor_latitude = 0.0;
  double new_heuristic_value = 0.0;
  int32_t neighbor_id;
  int beacon_type;
  struct timeval tv; 
  HeuristicValue *heuristic_value = NULL;
  GeoLocation dst_location;
  TimerCallback *beacon_timer = NULL;
  NRAttrVec attrs;
  Message *beacon_msg = NULL;
  PktHeader *pkt_header = NULL;

  // Get current time
  GetTime(&tv);

  switch (msg->msg_type_){

  case INTEREST:
  case EXPLORATORY_DATA:
  case PUSH_EXPLORATORY_DATA:

    if (msg->new_message_){
      // Don't worry about old messages

      // Pre-process the messages, extracting the geo-header attribute
      // if it's present (or trying to create a new geo-header
      // otherwise)
      pkt_header = preProcessMessage(msg);

      // If we have a packet header, add it to the message list
      if (pkt_header){
	sendNeighborRequest();
      	message_list_.push_back(pkt_header);
      }
    }

    ((DiffusionRouting *)dr_)->sendMessage(msg, pre_filter_handle_);

    break;

  case DATA:

    // Look for a beacon type attribute in the data message
    beacon_type_attribute = GeoBeaconTypeAttr.find(msg->msg_attr_vec_);

    // If there is no beacon_type attribute, this is not a beacon data
    // message. So, we just forward it to the next filter
    if (!beacon_type_attribute){
      ((DiffusionRouting *)dr_)->sendMessage(msg, pre_filter_handle_);

      break;
    }

    // Get neighbor id
    neighbor_id = msg->last_hop_;

    if ((neighbor_id == BROADCAST_ADDR) ||
	(neighbor_id == LOCALHOST_ADDR)){
      DiffPrint(DEBUG_ALWAYS, "Gear: %d.%06d - Neighbor ID Invalid !\n",
		tv.tv_sec, tv.tv_usec);
      return;
    }

    // Look for the rest of the information
    beacon_type = beacon_type_attribute->getVal();

    beacon_info_attribute = GeoLongitudeAttr.find(msg->msg_attr_vec_);
    if (beacon_info_attribute){
      neighbor_longitude = beacon_info_attribute->getVal();
    }

    beacon_info_attribute = GeoLatitudeAttr.find(msg->msg_attr_vec_);
    if (beacon_info_attribute){
      neighbor_latitude = beacon_info_attribute->getVal();
    }

    beacon_info_attribute = GeoRemainingEnergyAttr.find(msg->msg_attr_vec_);
    if (beacon_info_attribute){
      neighbor_energy = beacon_info_attribute->getVal();
    }

    // Update our neighbor list with the beacon's information
    updateNeighbor(neighbor_id, neighbor_longitude,
		   neighbor_latitude, neighbor_energy);

    // Update time
    GetTime(&tv);

    DiffPrint(DEBUG_IMPORTANT, "Gear: %d.%06d - Beacon Information id: %d - location: %f,%f - energy: %f\n",
	      tv.tv_sec, tv.tv_usec,
	      neighbor_id, neighbor_longitude,
	      neighbor_latitude, neighbor_energy);

    if (beacon_type == GEO_REQUEST){
      DiffPrint(DEBUG_IMPORTANT,
		"Gear: %d.%06d - Received a beacon request...\n",
		tv.tv_sec, tv.tv_usec);
      // If we received a neighbor request, we have to generate a
      // reply and send it after some random time. But this has to be
      // suppresed if we have recently done it already

      if (last_beacon_reply_tv_.tv_sec > 0){
	// Send at most one neighbor beacon every
	// GEO_BEACON_REPLY_PERIOD seconds
	if ((tv.tv_sec - last_beacon_reply_tv_.tv_sec)
	    < GEO_BEACON_REPLY_PERIOD){
	  DiffPrint(DEBUG_IMPORTANT,
		    "Gear: %d.%06d - Ignoring beacon request until %d.%06d!\n",
		    tv.tv_sec, tv.tv_usec,
		    (tv.tv_sec + GEO_BEACON_REPLY_PERIOD), tv.tv_usec);
	  break;
	}
      }

      // Create beacon reply message
      attrs.push_back(GeoLongitudeAttr.make(NRAttribute::IS, geo_longitude_));
      attrs.push_back(GeoLatitudeAttr.make(NRAttribute::IS, geo_latitude_));
      attrs.push_back(GeoRemainingEnergyAttr.make(NRAttribute::IS,
						  remainingEnergy()));
      attrs.push_back(GeoBeaconTypeAttr.make(NRAttribute::IS, GEO_REPLY));

      // Let's try to extract the beacon's heuristic_value
      heuristic_value_attribute = GeoHeuristicValueAttr.find(msg->msg_attr_vec_);

      if (heuristic_value_attribute){
	// Heuristic Value attribute found ! Include information about
	// this destination if we have anything in our table
	heuristic_value = (HeuristicValue *) heuristic_value_attribute->getVal();
	dst_location.longitude_ = heuristic_value->dst_longitude_;
	dst_location.latitude_ = heuristic_value->dst_latitude_;

	// Retrieve heuristic_value
	DiffPrint(DEBUG_IMPORTANT,
		  "Gear: %d.%06d - Requesting h-value in (%lf,%lf)\n",
		  tv.tv_sec, tv.tv_usec,
		  dst_location.longitude_, dst_location.latitude_); 

	new_heuristic_value = retrieveHeuristicValue(dst_location);

	if (new_heuristic_value != INITIAL_HEURISTIC_VALUE){
	  // Add the heuristic_value to the message if we have any
	  // information about this particular destination
 
	  heuristic_value = new HeuristicValue(dst_location.longitude_,
					       dst_location.latitude_,
					       new_heuristic_value);

	  DiffPrint(DEBUG_IMPORTANT,
		    "Gear: %d.%06d - Current h-value for (%lf,%lf):%lf\n",
		    tv.tv_sec, tv.tv_usec,
		    dst_location.longitude_, dst_location.latitude_,
		    new_heuristic_value);

	  attrs.push_back(GeoHeuristicValueAttr.make(NRAttribute::IS,
						     (void *) heuristic_value,
						     sizeof(HeuristicValue)));
	  delete heuristic_value;
	}
      }

      beacon_msg = new Message(DIFFUSION_VERSION, DATA, 0, 0, attrs.size(),
			       pkt_count_, rdm_id_, BROADCAST_ADDR,
			       LOCALHOST_ADDR);

      // Don't forget to update pkt_count
      pkt_count_++;
      beacon_msg->msg_attr_vec_ = CopyAttrs(&attrs);

      beacon_timer = new GeoMessageSendTimer(this, CopyMessage(beacon_msg));

      ((DiffusionRouting *)dr_)->addTimer(GEO_BEACON_REPLY_DELAY +
					  (int) ((GEO_BEACON_REPLY_JITTER *
						  (GetRand() * 1.0 / RAND_MAX) -
						  (GEO_BEACON_REPLY_JITTER / 2))),
					  beacon_timer);

      ClearAttrs(&attrs);
      delete beacon_msg;

      DiffPrint(DEBUG_IMPORTANT,
		"Gear: %d.%06d - Sending a beacon reply in response to request !\n",
		tv.tv_sec, tv.tv_usec);
      DiffPrint(DEBUG_IMPORTANT,
		"Gear: %d.%06d - Local info: location: %f,%f - energy: %f\n",
		tv.tv_sec, tv.tv_usec,
		geo_longitude_,	geo_latitude_, remainingEnergy());

      // Update beacon timestamp
      GetTime(&last_beacon_reply_tv_);
    }
    else{
      if (beacon_type == GEO_REPLY){
	DiffPrint(DEBUG_IMPORTANT, "Gear: %d.%06d - Received beacon reply\n",
		  tv.tv_sec, tv.tv_usec);
      }
      else{
	if (beacon_type == GEO_UPDATE){
	  DiffPrint(DEBUG_IMPORTANT,
		    "Gear: %d.%06d - Received beacon update\n",
		    tv.tv_sec, tv.tv_usec);
	}
	else{
	  DiffPrint(DEBUG_ALWAYS,
		    "Gear: %d.%06d - Received unknown message !\n",
		    tv.tv_sec, tv.tv_usec);
	  return;
	}
      }

      // Extract the heuristic_value from the message
      heuristic_value_attribute = GeoHeuristicValueAttr.find(msg->msg_attr_vec_);

      if (heuristic_value_attribute){
	heuristic_value = (HeuristicValue *) heuristic_value_attribute->getVal();

	// Update learned cost value table
	dst_location.longitude_ = heuristic_value->dst_longitude_;
	dst_location.latitude_ = heuristic_value->dst_latitude_;

	DiffPrint(DEBUG_IMPORTANT,
		  "Gear: %d.%06d - Received h-val update %d: (%lf,%lf):%f\n",
		  tv.tv_sec, tv.tv_usec,
		  neighbor_id, dst_location.longitude_,
		  dst_location.latitude_,
		  heuristic_value->heuristic_value_);

	learned_cost_table_.updateEntry(neighbor_id, dst_location,
					heuristic_value->heuristic_value_);
      }
    }

    break;

  default:

    // If we do not know about this message, we just pass it to the
    // next filter
    ((DiffusionRouting *)dr_)->sendMessage(msg, pre_filter_handle_);

    break;

  }
}

void GeoRoutingFilter::postProcessFilter(Message *msg)
{
  PktHeader *pkt_header = NULL;
  GeoHeader *geo_header = NULL;
  int action;
  int32_t next_hop;
  struct timeval tv;

  // Get current time
  GetTime(&tv);

  switch (msg->msg_type_){

  case INTEREST:
  case EXPLORATORY_DATA:
  case PUSH_EXPLORATORY_DATA:

    // Ignore messages with a local application as destination
    if (msg->next_hop_ != LOCALHOST_ADDR){

      // Retrieve packet header from previous stage
      pkt_header = retrievePacketHeader(msg);

      // If we have record of this packet, we restore the previous
      // geo_header information before forwarding it further
      if (pkt_header){
	geo_header = restoreGeoHeader(pkt_header, msg);

	// Increment path_len
	geo_header->path_len_++;

	// Check if we have to broadcast this message. We do not need
	// to check if I have seen this broadcast packet before, since
	// this is called by PostProcessFilter, so it must be a new
	// packet. We only need to check if we are inside the target
	// region
	action = floodInsideRegion(geo_header);

	// Add GeoHeader attribute to the message
	msg->msg_attr_vec_->push_back(GeoHeaderAttr.make(NRAttribute::IS,
							 (void *) geo_header,
							 sizeof(GeoHeader)));

	// Update time
	GetTime(&tv);

	switch (action){

	case BROADCAST:

	  // We are inside the region and have at least a neighbor
	  // which is inside the region too, so we just broadcast this
	  // message
	  DiffPrint(DEBUG_IMPORTANT,
		    "Gear: %d.%06d - Broadcasting message: last_hop: %d!\n",
		    tv.tv_sec, tv.tv_usec, msg->last_hop_);
	  
	  msg->next_hop_ = BROADCAST_ADDR;
	  ((DiffusionRouting *)dr_)->sendMessage(msg, post_filter_handle_);

	  break;

	case BROADCAST_SUPPRESS:

	  // We are either inside the region and have no neighbors
	  // also inside this region OR packet came from inside the
	  // region and we are outside the region. In either case, we
	  // do not forward the packet
	  DiffPrint(DEBUG_IMPORTANT,
		    "Gear: %d.%06d - Suppressing broadcast !\n",
		    tv.tv_sec, tv.tv_usec);

	  break;

	case OUTSIDE_REGION:

	  // The packet is still outside the region. In order to route
	  // it to the region, we first try using a 'greedy mode'. If
	  // that doesn't work, we probably need to navigate around a
	  // hole
	  DiffPrint(DEBUG_IMPORTANT,
		    "Gear: %d.%06d - Packet outside target region !\n",
		    tv.tv_sec, tv.tv_usec);

	  next_hop = findNextHop(geo_header, true);

	  // If there are no neighbors, let's try to go around a hole
	  if (next_hop == BROADCAST_ADDR){

	    // Check if we should still be looking for a path
	    if (geo_header->path_len_ < MAX_PATH_LEN)
	      next_hop = findNextHop(geo_header, false);
	  }

	  // Update time
	  GetTime(&tv);

	  // Still no neighbors, nothing we can do !
	  if (next_hop == BROADCAST_ADDR){
	    DiffPrint(DEBUG_IMPORTANT,
		      "Gear: %d.%06d - Cannot find next hop !\n",
		      tv.tv_sec, tv.tv_usec);
	  }
	  else{
	    // Forward message to next_hop
	    msg->next_hop_ = next_hop;
	    DiffPrint(DEBUG_IMPORTANT,
		      "Gear: %d.%06d - Forwarding message !\n",
		      tv.tv_sec, tv.tv_usec);
	    DiffPrint(DEBUG_IMPORTANT,
		      "Gear: %d.%06d - Next Hop: %d\n",
		      tv.tv_sec, tv.tv_usec, next_hop);
	    DiffPrint(DEBUG_IMPORTANT,
		      "Gear: %d.%06d - Last Hop: %d\n",
		      tv.tv_sec, tv.tv_usec, msg->last_hop_);
	    ((DiffusionRouting *)dr_)->sendMessage(msg, post_filter_handle_);
	  }
	
	  break;

	default:

	  break;
	}

	// Let's not forget to delete both geo_header and pkt_header
	delete pkt_header;
	delete geo_header;
      }
      else{
	// This message has no packet header information, so we just forward it
	DiffPrint(DEBUG_IMPORTANT,
		  "Gear: %d.%06d - Forwarding message without packet header info !\n",
		  tv.tv_sec, tv.tv_usec);
	((DiffusionRouting *)dr_)->sendMessage(msg, post_filter_handle_);
      }
    }
    else{
      // This is a message from gradient to a local app
      ((DiffusionRouting *)dr_)->sendMessage(msg, post_filter_handle_);
    }

    break;

  default:

    // All other messages are just forwarded without any changes
    ((DiffusionRouting *)dr_)->sendMessage(msg, post_filter_handle_);

    break;
  }
}

handle GeoRoutingFilter::setupPreFilter()
{
  NRAttrVec attrs;
  handle h;

  // Set up a filter that matches every packet coming in
  attrs.push_back(NRClassAttr.make(NRAttribute::IS,
				   NRAttribute::INTEREST_CLASS));

  h = ((DiffusionRouting *)dr_)->addFilter(&attrs,
					   GEOROUTING_PRE_FILTER_PRIORITY,
					   filter_callback_);
  ClearAttrs(&attrs);
  return h;
}

handle GeoRoutingFilter::setupPostFilter()
{
  NRAttrVec attrs;
  handle h;

  // This is a filter that matches all packets after processing by the
  // gradient filter
  attrs.push_back(NRClassAttr.make(NRAttribute::IS,
				   NRAttribute::INTEREST_CLASS));

  h = ((DiffusionRouting *)dr_)->addFilter(&attrs,
					   GEOROUTING_POST_FILTER_PRIORITY,
					   filter_callback_);

  ClearAttrs(&attrs);
  return h;
}

void GeoRoutingFilter::run()
{
#ifdef NS_DIFFUSION
  TimerCallback *neighbor_timer, *beacon_timer;
  struct timeval tv;

  // Set up node location
  getNodeLocation(&geo_longitude_, &geo_latitude_);

  // Get current time
  GetTime(&tv);

  DiffPrint(DEBUG_ALWAYS, "Gear: %d.%06d - Location %f,%f\n",
	    tv.tv_sec, tv.tv_usec, geo_longitude_, geo_latitude_);

  // Set up filters
  pre_filter_handle_ = setupPreFilter();
  post_filter_handle_ = setupPostFilter();

  // Add periodic neighbor checking timer
  neighbor_timer = new GeoNeighborsTimer(this);
  ((DiffusionRouting *)dr_)->addTimer(GEO_NEIGHBOR_DELAY, neighbor_timer);

  // Add periodic beacon request timer
  beacon_timer = new GeoBeaconRequestTimer(this);
  ((DiffusionRouting *)dr_)->addTimer(GEO_BEACON_REQUEST_CHECK_PERIOD,
				      beacon_timer);

  DiffPrint(DEBUG_ALWAYS, "Gear: %d.%06d - Received handle %d for pre Filter !\n",
	    tv.tv_sec, tv.tv_usec, pre_filter_handle_);
  DiffPrint(DEBUG_ALWAYS, "Gear: %d.%06d - Received handle %d for post Filter !\n",
	    tv.tv_sec, tv.tv_usec, post_filter_handle_);
  DiffPrint(DEBUG_ALWAYS, "Gear: %d.%06d - Initialized !\n",
	    tv.tv_sec, tv.tv_usec);
#endif // NS_DIFFUSION

  // Sends a beacon request upon start-up
  beaconTimeout();

#ifndef NS_DIFFUSION
  // We don't do anything
  while (1){
    sleep(1000);
  }
#endif // !NS_DIFFUSION
}

#ifdef NS_DIFFUSION
GeoRoutingFilter::GeoRoutingFilter()
{
#else
GeoRoutingFilter::GeoRoutingFilter(int argc, char **argv)
{
  TimerCallback *neighbor_timer, *beacon_timer;

  // Parse command line options
  parseCommandLine(argc, argv);
#endif // NS_DIFFUSION

  struct timeval tv;

  // Initialize a few parameters
  initial_energy_ = GEO_INITIAL_ENERGY;
  unit_energy_for_send_ = GEO_UNIT_ENERGY_FOR_SEND;
  unit_energy_for_recv_ = GEO_UNIT_ENERGY_FOR_RECV;
  num_pkt_sent_ = 0;
  num_pkt_recv_ = 0;

  // Initialize beacon timestamp
  last_beacon_reply_tv_.tv_sec = 0; 
  last_beacon_reply_tv_.tv_usec = 0; 
  last_neighbor_request_tv_.tv_sec = 0;
  last_neighbor_request_tv_.tv_usec = 0;

#ifndef NS_DIFFUSION
  getNodeLocation(&geo_longitude_, &geo_latitude_);
#endif
  // Get current time
  GetTime(&tv);

  DiffPrint(DEBUG_ALWAYS, "Gear: %d.%06d - Location %f,%f\n",
	    tv.tv_sec, tv.tv_usec, geo_longitude_, geo_latitude_);

  SetSeed(&tv);
  pkt_count_ = GetRand();
  rdm_id_ = GetRand();

#ifndef NS_DIFFUSION
  // Create Diffusion Routing class
  dr_ = NR::createNR(diffusion_port_);
#endif // !NS_DIFFUSION

  filter_callback_ = new GeoFilterReceive(this);

#ifndef NS_DIFFUSION
  // Set up filters
  pre_filter_handle_ = setupPreFilter();
  post_filter_handle_ = setupPostFilter();

  // Add periodic neighbor checking timer
  neighbor_timer = new GeoNeighborsTimer(this);
  ((DiffusionRouting *)dr_)->addTimer(GEO_NEIGHBOR_DELAY, neighbor_timer);

  // Add periodic beacon request timer
  beacon_timer = new GeoBeaconRequestTimer(this);
  ((DiffusionRouting *)dr_)->addTimer(GEO_BEACON_REQUEST_CHECK_PERIOD,
				      beacon_timer);

  DiffPrint(DEBUG_ALWAYS,
	    "Gear: %d.%06d - Received handle %d for pre Filter !\n",
	    tv.tv_sec, tv.tv_usec, pre_filter_handle_);
  DiffPrint(DEBUG_ALWAYS,
	    "Gear: %d.%06d - Received handle %d for post Filter !\n",
	    tv.tv_sec, tv.tv_usec, post_filter_handle_);
  DiffPrint(DEBUG_ALWAYS, "Gear: %d.%06d - Initialized !\n",
	    tv.tv_sec, tv.tv_usec);
#endif // !NS_DIFFUSION
}

void GeoRoutingFilter::getNodeLocation(double *longitude, double *latitude)
{
#ifdef NS_DIFFUSION
  double z;

  MobileNode *node = ((DiffusionRouting *)dr_)->getNode();
  node->getLoc(longitude, latitude, &z);
#else
  char *longitude_env, *latitude_env;
  struct timeval tv;

  // Get current time
  GetTime(&tv);

  longitude_env = getenv("gear_longitude");
  latitude_env = getenv("gear_latitude");

  if (longitude_env && latitude_env){
    *longitude = atof(longitude_env);
    *latitude = atof(latitude_env);
  }
  else{
    DiffPrint(DEBUG_ALWAYS,
	      "Gear: %d.%06d - Cannot get location (gear_longitude, gear_latitude) !\n",
	      tv.tv_sec, tv.tv_usec);
    exit(-1);
  }
#endif // NS_DIFFUSION
}

bool GeoRoutingFilter::checkNeighbors()
{
  NeighborList::iterator neighbor_itr;
  NeighborEntry *neighbor_entry;
  struct timeval tv;

  GetTime(&tv);

  for (neighbor_itr = neighbors_list_.begin();
       neighbor_itr != neighbors_list_.end(); ++neighbor_itr){
    neighbor_entry = *neighbor_itr;

    if ((tv.tv_sec - neighbor_entry->tv_.tv_sec) >= GEO_NEIGHBOR_UPDATE)
      return true;
  }
  
  return false;
}

void GeoRoutingFilter::sendNeighborRequest()
{
  NRAttrVec attrs;
  Message *beacon_msg;
  TimerCallback *beacon_timer;
  struct timeval tv;

  // Get current time
  GetTime(&tv);

  // Check if we need to send a beacon request to update our neighbor
  // information
  if (!checkNeighbors())
    return;

  // Yes ! Create the beacon message
  attrs.push_back(GeoBeaconTypeAttr.make(NRAttribute::IS, GEO_REQUEST));
  attrs.push_back(GeoLatitudeAttr.make(NRAttribute::IS, geo_latitude_));
  attrs.push_back(GeoLongitudeAttr.make(NRAttribute::IS, geo_longitude_));
  attrs.push_back(GeoRemainingEnergyAttr.make(NRAttribute::IS,
					      remainingEnergy()));

  // Neighbor beacon msg is a DATA message
  beacon_msg = new Message(DIFFUSION_VERSION, DATA, 0, 0,
			   attrs.size(), pkt_count_, rdm_id_,
			   BROADCAST_ADDR, LOCALHOST_ADDR);
  // Don't forget to increment pkt_count
  pkt_count_++;

  beacon_msg->msg_attr_vec_ = CopyAttrs(&attrs);

  // Generate a beacon request, add some random jitter before actually
  // sending it
  DiffPrint(DEBUG_IMPORTANT,
	    "Gear: %d.%06d - Broadcasting neighbor info request...\n",
	    tv.tv_sec, tv.tv_usec);

  beacon_timer = new GeoMessageSendTimer(this, CopyMessage(beacon_msg));

  ((DiffusionRouting *)dr_)->addTimer(GEO_BEACON_DELAY +
				      (int) ((GEO_BEACON_JITTER *
					      (GetRand() * 1.0 / RAND_MAX) -
					      (GEO_BEACON_JITTER / 2))),
				      beacon_timer);

  GetTime(&last_neighbor_request_tv_);

  ClearAttrs(&attrs);
  delete beacon_msg;
}

void GeoRoutingFilter::updateNeighbor(int32_t neighbor_id,
				      double neighbor_longitude,
				      double neighbor_latitude,
				      double neighbor_energy)
{
  NeighborEntry *neighbor_entry;
  struct timeval tv;

  // Get current time
  GetTime(&tv);

  // Look for this neighbor in our neighbor's list
  neighbor_entry = findNeighbor(neighbor_id);

  if (!neighbor_entry){
    // Insert a new neighbor into our list
    DiffPrint(DEBUG_IMPORTANT,
	      "Gear: %d.%06d - Inserting new neighbor %d !\n",
	      tv.tv_sec, tv.tv_usec, neighbor_id);

    neighbor_entry = new NeighborEntry(neighbor_id, neighbor_longitude,
				       neighbor_latitude, neighbor_energy);

    // Insert new element with key neighbor_id into the hash map
    neighbors_list_.push_back(neighbor_entry);
  }
  else{
    // Update an existing neighbor entry
    DiffPrint(DEBUG_IMPORTANT,
	      "Gear: %d.%06d - Updating neighbor %d !\n",
	      tv.tv_sec, tv.tv_usec, neighbor_id);

    neighbor_entry->longitude_ = neighbor_longitude;
    neighbor_entry->latitude_ = neighbor_latitude;
    neighbor_entry->remaining_energy_ = neighbor_energy;

    GetTime(&(neighbor_entry->tv_));
  }
}

PktHeader * GeoRoutingFilter::retrievePacketHeader(Message *msg)
{
  PacketList::iterator packet_itr;
  PktHeader *pkt_header = NULL;

  for (packet_itr = message_list_.begin();
       packet_itr != message_list_.end(); ++packet_itr){
    pkt_header = *packet_itr;
    if ((pkt_header->rdm_id_ == msg->rdm_id_) &&
	(pkt_header->pkt_num_ == msg->pkt_num_)){
      packet_itr = message_list_.erase(packet_itr);
      return pkt_header;
    }
  }

  // Entry not found in our list
  return NULL;
}

PktHeader * GeoRoutingFilter::preProcessMessage(Message *msg)
{
  float longitude_min, longitude_max;
  float latitude_min, latitude_max;
  PktHeader *pkt_header = NULL;
  struct timeval tv;

  // Get current time
  GetTime(&tv);

  pkt_header = stripOutHeader(msg);

  if (!pkt_header){
    // This is a message either coming from a local application (for
    // which a geo_header hasn't been extracted yet) or a message with
    // no geographic information (in which case there's nothing we can
    // do about). We first try to extract geographic information from
    // the message in order to create a new geo_header
    if (extractLocation(msg, &longitude_min, &longitude_max,
			&latitude_min, &latitude_max)){

      // Create new Packet Header
      pkt_header = new PktHeader;

      // Fill in the new packet header
      pkt_header->pkt_num_ = msg->pkt_num_;
      pkt_header->rdm_id_ = msg->rdm_id_;
      pkt_header->prev_hop_ = msg->last_hop_;
      pkt_header->pkt_type_ = UNICAST_ORIGINAL;
      pkt_header->path_len_ = 0;

      pkt_header->dst_region_.center_.longitude_ = (longitude_min +
						    longitude_max) / 2;
      pkt_header->dst_region_.center_.latitude_ = (latitude_min +
						   latitude_max) / 2;

      pkt_header->dst_region_.radius_ = Distance(longitude_min, latitude_min,
						 longitude_max,
						 latitude_max) / 2;

      DiffPrint(DEBUG_IMPORTANT,
		"Gear: %d.%06d - ExtractLocation %f,%f, %f,%f (%f,%f)\n",
		tv.tv_sec, tv.tv_usec,
		longitude_min, longitude_max, latitude_min, latitude_max,
		pkt_header->dst_region_.center_.longitude_,
		pkt_header->dst_region_.center_.latitude_);
    }
  }

  return pkt_header;
}

PktHeader * GeoRoutingFilter::stripOutHeader(Message *msg)
{
  NRSimpleAttribute<void *> *geo_header_attribute;
  GeoHeader *geo_header;
  PktHeader *pkt_header = NULL;
  struct timeval tv;

  // Get current time
  GetTime(&tv);

  geo_header_attribute = GeoHeaderAttr.find(msg->msg_attr_vec_);

  if (geo_header_attribute){
    geo_header = (GeoHeader *)(geo_header_attribute->getVal());

    // Create Packet Header structure
    pkt_header = new PktHeader;

    pkt_header->pkt_num_ = msg->pkt_num_;
    pkt_header->rdm_id_ = msg->rdm_id_;
    pkt_header->prev_hop_ = msg->last_hop_;

    // Copy the msg from Geo_header to PktHeader
    pkt_header->pkt_type_ = geo_header->pkt_type_;
    pkt_header->dst_region_.center_.longitude_ = geo_header->dst_region_.center_.longitude_;
    pkt_header->dst_region_.center_.latitude_ = geo_header->dst_region_.center_.latitude_;
    pkt_header->dst_region_.radius_ = geo_header->dst_region_.radius_;
    pkt_header->path_len_ = geo_header->path_len_;

    takeOutAttr(msg->msg_attr_vec_, GEO_HEADER_KEY);
    delete geo_header_attribute;

    DiffPrint(DEBUG_IMPORTANT,
	      "Gear: %d.%06d - Got GeoHeader last hop: %d, pkt(%d, %d) !\n",
	      tv.tv_sec, tv.tv_usec, msg->last_hop_,
	      msg->pkt_num_, msg->rdm_id_);
    DiffPrint(DEBUG_IMPORTANT,
	      "Gear: %d.%06d - Type: %d, Region: %f,%f,%f, Path: %d !\n",
	      tv.tv_sec, tv.tv_usec, pkt_header->pkt_type_,
	      pkt_header->dst_region_.center_.longitude_,
	      pkt_header->dst_region_.center_.latitude_,
	      pkt_header->dst_region_.radius_,
	      pkt_header->path_len_);
  }
  else{
    DiffPrint(DEBUG_IMPORTANT,
	      "Gear: %d.%06d - GeoHeader Attribute not present !\n",
	      tv.tv_sec, tv.tv_usec);
  }
  return pkt_header;
}

void GeoRoutingFilter::takeOutAttr(NRAttrVec *attrs, int32_t key)
{
  NRAttrVec::iterator attr_itr;

  for (attr_itr = attrs->begin(); attr_itr != attrs->end(); ++attr_itr){
    if ((*attr_itr)->getKey() == key){
      break;
    }
  }

  if (attr_itr != attrs->end())
    attrs->erase(attr_itr);
}

bool GeoRoutingFilter::extractLocation(Message *msg,
				       float *longitude_min,
				       float *longitude_max,
				       float *latitude_min,
				       float *latitude_max)
{
  NRSimpleAttribute<float> *lat_attr;
  NRSimpleAttribute<float> *long_attr;
  NRAttrVec::iterator itr;
  NRAttrVec *attrs;
  bool has_long_min = false;
  bool has_long_max = false;
  bool has_lat_min = false;
  bool has_lat_max = false;

  attrs = msg->msg_attr_vec_;

  // Extracts longitude coordinates of the target region from the
  // message
  itr = attrs->begin();

  for (;;){

    long_attr = LongitudeAttr.find_from(attrs, itr, &itr);

    if (!long_attr)
      break;

    if ((long_attr->getOp() == NRAttribute::GT) ||
	(long_attr->getOp() == NRAttribute::GE)){
      has_long_min = true;
      *longitude_min = long_attr->getVal();
    }

    if ((long_attr->getOp() == NRAttribute::LT) ||
	(long_attr->getOp() == NRAttribute::LE)){
      has_long_max = true;
      *longitude_max = long_attr->getVal();
    }

    // Increment itr to avoid an infinite loop
    itr++;
  }

  // Now we extract latitude coordinates of the target region from the
  // message
  itr = attrs->begin();

  for (;;){

    lat_attr = LatitudeAttr.find_from(attrs, itr, &itr);

    if (!lat_attr)
      break;

    if ((lat_attr->getOp() == NRAttribute::GT) ||
	(lat_attr->getOp() == NRAttribute::GE)){
      has_lat_min = true;
      *latitude_min = lat_attr->getVal();
    }

    if ((lat_attr->getOp() == NRAttribute::LT) ||
	(lat_attr->getOp() == NRAttribute::LE)){
      has_lat_max = true;
      *latitude_max = lat_attr->getVal();
    }

    // Increment itr to avoid an infinite loop
    itr++;
  }

  if (has_long_min && has_long_max && has_lat_min && has_lat_min)
    return true;

  return false;
}

GeoHeader * GeoRoutingFilter::restoreGeoHeader(PktHeader *pkt_header, Message *msg)
{
  GeoHeader *geo_header;

  geo_header = new GeoHeader;

  msg->last_hop_ = pkt_header->prev_hop_;

  geo_header->pkt_type_ = pkt_header->pkt_type_;
  geo_header->dst_region_.center_.longitude_ = pkt_header->dst_region_.center_.longitude_;
  geo_header->dst_region_.center_.latitude_ = pkt_header->dst_region_.center_.latitude_;
  geo_header->dst_region_.radius_ = pkt_header->dst_region_.radius_;
  geo_header->path_len_ = pkt_header->path_len_;

  return geo_header;
}

int32_t GeoRoutingFilter::findNextHop(GeoHeader *geo_header, bool greedy)
{
  NeighborList::iterator neighbor_itr;
  NeighborEntry *neighbor_entry;
  GeoLocation destination, min_neighbor_location;
  double current_learned_cost, min_learned_cost;
  double current_distance, min_distance;
  double distance, gap;
  int32_t min_cost_id, neighbor_id;
  int num_neighbors;
  double new_heuristic_value;
  struct timeval tv;

  // Get current time
  GetTime(&tv);

  // Load the destination coordinate from the packet
  destination = geo_header->dst_region_.center_;

  current_distance = Distance(geo_longitude_, geo_latitude_,
			      destination.longitude_, destination.latitude_);

  min_distance = MAX_INT;
  min_learned_cost = MAX_INT;
  num_neighbors = 0;

  // Now we go through out list of neighbor and compute the cost to
  // each one
  for (neighbor_itr = neighbors_list_.begin();
       neighbor_itr != neighbors_list_.end(); ++neighbor_itr){
    neighbor_entry = *neighbor_itr;
    num_neighbors++;

    neighbor_id = neighbor_entry->id_;
    current_learned_cost = retrieveLearnedCost(neighbor_id, destination);

    // Calculate distance from this neighbor to dst
    distance = Distance(neighbor_entry->longitude_, neighbor_entry->latitude_,
			destination.longitude_, destination.latitude_);

    // If we are in 'greedy mode', we do not want to move away, so we
    // skip neighbors that are farther away than us from the region
    if (greedy && (distance > current_distance))
      continue;

    DiffPrint(DEBUG_IMPORTANT,
	      "Gear: %d.%06d - Neighbor: %d: cost = %f, min_cost = %f\n",
	      tv.tv_sec, tv.tv_usec,
	      neighbor_id, current_learned_cost, min_learned_cost);

    // Found a neighbor with a lower cost
    if (current_learned_cost < min_learned_cost){
      min_learned_cost = current_learned_cost;
      min_cost_id = neighbor_entry->id_;
      min_neighbor_location.longitude_ = neighbor_entry->longitude_;
      min_neighbor_location.latitude_ = neighbor_entry->latitude_;
    }
  }

  DiffPrint(DEBUG_IMPORTANT,
	    "Gear: %d.%06d - # neighbors: %d; cur: %f,%f, dst: %f,%f\n",
	    tv.tv_sec, tv.tv_usec,
	    num_neighbors, geo_longitude_, geo_latitude_,
	    destination.longitude_, destination.latitude_);

  // Check if we have neighbors we can use to forward this message
  if (min_learned_cost < MAX_INT){

    // Calculate the cost from me to my next hop neighbor
    gap = Distance(min_neighbor_location.longitude_,
		   min_neighbor_location.latitude_,
		   geo_longitude_, geo_latitude_);

    // Update my heuristic_value
    new_heuristic_value = min_learned_cost + gap;

    // Broadcast the new heuristic value if it's changed significantly
    if (h_value_table_.updateEntry(destination, new_heuristic_value))
      broadcastHeuristicValue(destination, new_heuristic_value);

    // Return neighbor this message should go
    return min_cost_id;
  }

  // We have no neighbors to whom we can forward this message !
  return BROADCAST_ADDR;
}

void GeoRoutingFilter::broadcastHeuristicValue(GeoLocation dst,
					       double new_heuristic_value)
{
  NRAttrVec attrs;
  HeuristicValue *heuristic_value;
  Message *beacon_msg;
  TimerCallback *beacon_timer;

  attrs.push_back(GeoLongitudeAttr.make(NRAttribute::IS, geo_longitude_));
  attrs.push_back(GeoLatitudeAttr.make(NRAttribute::IS, geo_latitude_));
  attrs.push_back(GeoRemainingEnergyAttr.make(NRAttribute::IS,
					      remainingEnergy()));
  attrs.push_back(GeoBeaconTypeAttr.make(NRAttribute::IS, GEO_UPDATE));

  heuristic_value = new HeuristicValue(dst.longitude_,
				       dst.latitude_, new_heuristic_value);

  attrs.push_back(GeoHeuristicValueAttr.make(NRAttribute::IS,
					     (void *) heuristic_value,
					     sizeof(HeuristicValue)));

  beacon_msg = new Message(DIFFUSION_VERSION, DATA, 0, 0,
			   attrs.size(), pkt_count_, rdm_id_,
			   BROADCAST_ADDR, LOCALHOST_ADDR);

  // Don't forget to increase pkt_count
  pkt_count_++;

  beacon_msg->msg_attr_vec_ = CopyAttrs(&attrs);

  // Now, we generate a beacon to broadcast triggered h-value update
  // but first we add some random jitter before actually sending it
  beacon_timer = new GeoMessageSendTimer(this, CopyMessage(beacon_msg));

  ((DiffusionRouting *)dr_)->addTimer(GEO_BEACON_DELAY +
				      (int) ((GEO_BEACON_JITTER *
					      (GetRand() * 1.0 / RAND_MAX) -
					      (GEO_BEACON_JITTER / 2))),
				      beacon_timer);

  // Delete everything we created here
  ClearAttrs(&attrs);
  delete beacon_msg;
  delete heuristic_value;
}

int GeoRoutingFilter::floodInsideRegion(GeoHeader *geo_header)
{
  NeighborList::iterator neighbor_itr;
  NeighborEntry *neighbor_entry;
  GeoLocation destination;
  double radius, distance;
  struct timeval tv;

  // Get current time
  GetTime(&tv);

  // Load destination coordinates
  destination = geo_header->dst_region_.center_;
  radius = geo_header->dst_region_.radius_;

  if (Distance(geo_longitude_, geo_latitude_,
	       destination.longitude_, destination.latitude_) < radius){

    // We are inside the target region, change mode to BROADCAST
    geo_header->pkt_type_ = BROADCAST_TYPE;

    DiffPrint(DEBUG_IMPORTANT,
	      "Gear: %d.%06d - Packet inside target region !\n",
	      tv.tv_sec, tv.tv_usec);

    // If all my neighbors are outside this region, suppress this
    // broadcast message
    for (neighbor_itr = neighbors_list_.begin();
	 neighbor_itr != neighbors_list_.end(); ++neighbor_itr){
      neighbor_entry = *neighbor_itr;

      DiffPrint(DEBUG_IMPORTANT,
		"Gear: %d.%06d - Neighbor %d, %lf,%lf !\n",
		tv.tv_sec, tv.tv_usec, neighbor_entry->id_,
		neighbor_entry->longitude_, neighbor_entry->latitude_);

      // Calculate distance between neighbor and dst
      distance = Distance(neighbor_entry->longitude_,
			  neighbor_entry->latitude_,
			  destination.longitude_, destination.latitude_);

      // As long as we have one neighbor inside the region, broadcast
      // the message
      if (distance < radius)
	return BROADCAST;
    }
    return BROADCAST_SUPPRESS;
  }
  else{
    if (geo_header->pkt_type_ == BROADCAST_TYPE){
      // This is a BROADCAST packet flooded outside the region
      return BROADCAST_SUPPRESS;
    }
    else{
      // We are still outside the target region, continue forwading
      // the packet towards the region
      return OUTSIDE_REGION;
    }
  }
}

double GeoRoutingFilter::retrieveLearnedCost(int neighbor_id, GeoLocation dst)
{
  int index;

  index = learned_cost_table_.retrieveEntry(neighbor_id, &dst);

  if (index != FAIL)
    return learned_cost_table_.table_[index].learned_cost_value_;
  else
    return estimateCost(neighbor_id, dst);
}

double GeoRoutingFilter::estimateCost(int neighbor_id, GeoLocation dst)
{
  NeighborEntry *neighbor_entry;
  double distance;
  struct timeval tv;

  // Get current time
  GetTime(&tv);

  // To get this neighbor's location, we first find the entry with neighbor_id
  // Since right now it is pure geographical routing, the estimated cost is
  // just the distance between neighbor_id to dst.

  if (neighbor_id < 0){
    DiffPrint(DEBUG_IMPORTANT,
	      "Gear: %d.%06d - Invalid neighbor id: %d !\n",
	      tv.tv_sec, tv.tv_usec, neighbor_id);
    return FAIL;
  }

  neighbor_entry = findNeighbor(neighbor_id);

  if (!neighbor_entry)
    return FAIL;

  distance = Distance(neighbor_entry->longitude_, neighbor_entry->latitude_,
		      dst.longitude_, dst.latitude_);

  return distance;
}

NeighborEntry * GeoRoutingFilter::findNeighbor(int32_t neighbor_id)
{
  NeighborList::iterator neighbor_itr;
  NeighborEntry *neighbor_entry;

  for (neighbor_itr = neighbors_list_.begin();
       neighbor_itr != neighbors_list_.end(); ++neighbor_itr){
    neighbor_entry = *neighbor_itr;
    if (neighbor_entry->id_ == neighbor_id)
      return neighbor_entry;
  }
  return NULL;
}

double GeoRoutingFilter::retrieveHeuristicValue(GeoLocation dst)
{
  int index;

  index = h_value_table_.retrieveEntry(&dst);

  if (index != FAIL)
    return h_value_table_.table_[index].heuristic_value_;

  return INITIAL_HEURISTIC_VALUE;
}

#ifndef USE_SINGLE_ADDRESS_SPACE
int main(int argc, char **argv)
{
  GeoRoutingFilter *geo_filter;

  geo_filter = new GeoRoutingFilter(argc, argv);
  geo_filter->run();

  return 0;
}
#endif // !USE_SINGLE_ADDRESS_SPACE

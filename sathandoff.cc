/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1999 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the MASH Research
 *      Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/sathandoff.cc,v 1.2 1999/06/23 23:41:58 tomh Exp $";
#endif

#include "random.h"
#include "sathandoff.h"
#include "satlink.h"
#include "satroute.h"
#include "satposition.h"
#include "satnode.h"


static class LinkHandoffMgrClass : public TclClass {
public:
        LinkHandoffMgrClass() : TclClass("HandoffManager") {}
        TclObject* create(int, const char*const*) {
                return (new LinkHandoffMgr());
        }
} class_link_handoff_manager;

static class SatLinkHandoffMgrClass : public TclClass {
public:
        SatLinkHandoffMgrClass() : TclClass("HandoffManager/Sat") {}
        TclObject* create(int, const char*const*) {
                return (new SatLinkHandoffMgr());
        }
} class_sat_link_handoff_manager;

static class TermLinkHandoffMgrClass : public TclClass {
public:
        TermLinkHandoffMgrClass() : TclClass("HandoffManager/Term") {}
        TclObject* create(int, const char*const*) {
                return (new TermLinkHandoffMgr());
        }
} class_term_link_handoff_manager;

void SatHandoffTimer::expire(Event*)
{                           
        a_->handoff();  
}                               

void TermHandoffTimer::expire(Event*)
{                           
        a_->handoff();  
}                               

//////////////////////////////////////////////////////////////////////////////
// class LinkHandoffMgr
//////////////////////////////////////////////////////////////////////////////

RNG LinkHandoffMgr::handoff_rng_;
int LinkHandoffMgr::handoff_randomization_ = 0;

LinkHandoffMgr::LinkHandoffMgr()
{
	bind_bool("handoff_randomization_", &handoff_randomization_);
}

int LinkHandoffMgr::command(int argc, const char*const* argv)
{
	if (argc == 2) {
	} else if (argc == 3) {
		if(strcmp(argv[1], "setnode") == 0) {
			node_ = (Node*) TclObject::lookup(argv[2]);
			if (node_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		}
	}
	return (TclObject::command(argc, argv));
}

// Helper function-- returns the distance between points a and b
float LinkHandoffMgr::distance(coordinate a, coordinate b)
{
        float a_x, a_y, a_z, b_x, b_y, b_z;     // cartesian
        a_x = a.r * sin(a.theta) * cos (a.phi);
        a_y = a.r * sin(a.theta) * sin (a.phi);
        a_z = a.r * cos(a.theta);
        b_x = b.r * sin(b.theta) * cos (b.phi);
        b_y = b.r * sin(b.theta) * sin (b.phi);
        b_z = b.r * cos(b.theta);
        return (DISTANCE(a_x, a_y, a_z, b_x, b_y, b_z));

}

// Each crossseam satellite will have two net stacks-- at most one will
// be occupied.  This procedure finds an unoccupied stack on the node.
SatLinkHead* LinkHandoffMgr::get_peer_next_linkhead(SatNode* np)
{
	LinkHead* lhp;
	SatLinkHead* slhp;
	for (lhp = np->linklisthead_.lh_first; lhp; 
	    lhp = lhp->nextlinkhead() ) {
		slhp = (SatLinkHead*) lhp;
		if (slhp->type() == LINK_ISL_CROSSSEAM) {
			if (!slhp->phy_tx()->channel() && 
			    !slhp->phy_rx()->channel() ) 
				return slhp;
		}
	}
	printf("Error, couldn't find an empty crossseam stack for handoff\n");
	return 0;
}

// This helper function assumes that the channel to which the link interface
// is attached has one peer node (i.e., no other receive infs on channel)
SatLinkHead* LinkHandoffMgr::get_peer_linkhead(SatLinkHead* slhp)
{
	SatChannel* chan_;
	Phy* remote_phy_;
	Node* remote_node_;

	chan_ = (SatChannel*) slhp->phy_tx()->channel();
	if (chan_ == 0) {
		printf("Error:  get_peer_linkhead called for a non-");
		printf("connected link on node %d\n", slhp->node()->address());
		return 0; // Link is not currently connected
	}
	remote_phy_ = chan_->ifhead_.lh_first; 
	if (remote_phy_ == 0) {
		printf("Error:  node %d's tx phy ", slhp->node()->address());
		printf("connected to channel with no receivers\n");
		return 0;
	}
	remote_node_ = remote_phy_->head()->node();
	if (remote_phy_->nextchnl()) {
		printf("Error:  This ISL channel has more than one target\n");
		return 0;
	}
	return ( (SatLinkHead*) remote_phy_->head());
}

// This helper function assumes that the channel to which the link interface
// is attached has one peer node (i.e., no other receive infs on channel)
SatNode* LinkHandoffMgr::get_peer(SatLinkHead* slhp)
{
	SatChannel* chan_;
	Phy* remote_phy_;

	chan_ = (SatChannel*) slhp->phy_tx()->channel();
	if (chan_ == 0)
		return 0; // Link is not currently connected
	remote_phy_ = chan_->ifhead_.lh_first; 
	if (remote_phy_ == 0) {
		printf("Error:  node %d's tx phy ", slhp->node()->address());
		printf("connected to channel with no receivers\n");
		return 0;
	}
	if (remote_phy_->nextchnl()) {
		printf("Error:  This ISL channel has more than one target\n");
		return 0;
	}
	
	return ( (SatNode*) remote_phy_->head()->node());
}

//////////////////////////////////////////////////////////////////////////
// class TermLinkHandoffMgr
//////////////////////////////////////////////////////////////////////////

double TermLinkHandoffMgr::elevation_mask_ = 0;
int TermLinkHandoffMgr::term_handoff_int_ = 10;

TermLinkHandoffMgr::TermLinkHandoffMgr() : timer_(this)
{
	bind("elevation_mask_", &elevation_mask_);
	bind("term_handoff_int_", &term_handoff_int_);
}

int TermLinkHandoffMgr::check_elevation(coordinate satellite,
    coordinate terminal)
{

	double elev_mask_ = DEG_TO_RAD(TermLinkHandoffMgr::elevation_mask_);
	double S = satellite.r;  // satellite radius
	double S_2 = satellite.r * satellite.r;  // satellite radius^2
	double E = EARTH_RADIUS;
	double E_2 = E * E;
	double d, theta, alpha;

	d = distance(satellite, terminal);
	if (d < sqrt(S_2 - E_2)) {
		// elevation angle > 0
		theta = acos((E_2+S_2-(d*d))/(2*E*S));
		alpha = acos(sin(theta) * S/d);
		return (alpha > elev_mask_);
	} else
		return 0;
}

// 
// This is called each time the node checks to see if its link to a
// polar satellite needs to be handed off.  
// There are two cases:
//     i) current link is up; check to see if it stays up or is handed off
//     ii) current link is down; check to see if it can go up
// If there are any changes, call for rerouting.  Finally, restart the timer. 
//
int TermLinkHandoffMgr::handoff()
{
	coordinate sat_coord, earth_coord;
	LinkHead* lhp;
	SatLinkHead* slhp;
	SatNode* peer_; // Polar satellite at opposite end of the GSL
	Node* nodep_;  // Pointer used in searching the list of nodes
	PolarSatPosition* nextpos_;
	int link_changes_flag_ = FALSE; // Flag indicating change took place 
	int restart_timer_flag_ = FALSE; // Restart timer only if polar links
	int found_flag_ = FALSE;  // Flag indicates whether handoff can occur 

	earth_coord = ((SatNode *)node_)->position()->getCoordinate();
	// Traverse the linked list of link interfaces
	for (lhp = node_->linklisthead_.lh_first; lhp; 
	    lhp = lhp->nextlinkhead() ) {
		slhp = (SatLinkHead*) lhp;
		if (slhp->type() == LINK_GSL_GEO || 
		    slhp->type() == LINK_GENERIC)
			continue;
		if (slhp->type() != LINK_GSL_POLAR) {
			printf("Error: Terminal link type ");
			printf("not valid %d NOW %f\n", slhp->type(), NOW);
			exit(1);
		}
		// The link is a GSL_POLAR link-- should be one receive 
		// interface on it
		restart_timer_flag_ = TRUE;
		peer_ = get_peer(slhp);
		if (peer_) {
			sat_coord = peer_->position()->getCoordinate();
			if (!check_elevation(sat_coord, earth_coord) &&
			    slhp->linkup_) {
				slhp->linkup_ = FALSE;
				link_changes_flag_ = TRUE;
				// Detach receive link interface from channel
				slhp->phy_rx()->removechnl();
				// Set channel pointers to NULL
				slhp->phy_tx()->setchnl(0);
				slhp->phy_rx()->setchnl(0);
			}
		}
		if (!slhp->linkup_) {
			// If link is down, see if we can use another satellite
			// 
			// As an optimization, first check the next satellite 
			// coming over the horizon.  Next, consider satellites
			// in a plane neighboring our current one.  Finally,
			// consider all remaining satellites.
			// 
			if (peer_) {
				// Next satellite
				nextpos_ = ((PolarSatPosition*) 
				    peer_->position())->next();
				if (nextpos_) {
					sat_coord = nextpos_->getCoordinate();
					found_flag_ = 
					check_elevation(sat_coord, earth_coord);
					peer_ = (SatNode*) nextpos_->node();
				}
			}
			// Next, check all remaining satellites if not found
			if (!found_flag_) {
				for (nodep_=Node::nodehead_.lh_first; nodep_;
				    nodep_ = nodep_->nextnode()) {
					peer_ = (SatNode*) nodep_;
					if (peer_->position() && 
					    (peer_->position()->type() !=
					    POSITION_SAT_POLAR))
						    continue;
					sat_coord = 
					    peer_->position()->getCoordinate();
					found_flag_ = 
					    check_elevation(sat_coord, 
					    earth_coord);
					if (found_flag_)
						break;
				}
			}
			if (found_flag_) {
				slhp->linkup_ = TRUE;
				link_changes_flag_ = TRUE;
				// Point slhp->phy_tx to peer_'s inlink
				slhp->phy_tx()->setchnl(peer_->uplink());
				// Point slhp->phy_rx to peer_'s outlink and
				// add phy_rx to the channels list of phy's
				slhp->phy_rx()->setchnl(peer_->downlink());
				slhp->phy_rx()->insertchnl(&(peer_->downlink()->ifhead_));
			}
		}
	}
	if (link_changes_flag_) { 
		SatRouteObject::instance().recompute();
	}
	if (restart_timer_flag_) {
		// If we don't have polar GSLs, don't reset the timer
		if (handoff_randomization_) {
			timer_.resched(term_handoff_int_ + 
			    handoff_rng_.uniform(-1 * term_handoff_int_/2, 
			    term_handoff_int_/2));
		} else
			timer_.resched(term_handoff_int_);
	}
        return link_changes_flag_;
}

//////////////////////////////////////////////////////////////////////////
// class SatLinkHandoffMgr
//////////////////////////////////////////////////////////////////////////

double SatLinkHandoffMgr::latitude_threshold_ = 0;
double SatLinkHandoffMgr::longitude_threshold_ = 0;
int SatLinkHandoffMgr::sat_handoff_int_ = 10;

SatLinkHandoffMgr::SatLinkHandoffMgr() : timer_(this)
{
	bind("sat_handoff_int_", &sat_handoff_int_);
	bind("latitude_threshold_", &latitude_threshold_);
	bind("longitude_threshold_", &longitude_threshold_);
}

//
// This function is responsible for activating, deactivating, and handing off
// satellite ISLs.  If the ISL is an intraplane link, 
// do nothing.  If the ISL is an interplane link, it will be taken down
// when _either_ of the connected satellites are above lat_threshold_ 
// degrees, and brought back up when _both_ satellites move below 
// lat_threshold_ again.  If an ISL is a cross-seam link, it must also be 
// handed off periodically while the satellite is below lat_threshold_.  
// 
// Finally, optimizations that avoid going through the linked lists unless 
// the satellite is ``close'' to lat_threshold_ are employed.
//
int SatLinkHandoffMgr::handoff()
{
	LinkHead* lhp;
	SatLinkHead *slhp, *peer_slhp, *peer_next_slhp;
	SatNode *local_, *peer_, *peer_next_; 
	PolarSatPosition *pos_, *peer_pos_, *peer_next_pos_;
	double dist_to_peer, dist_to_next;
	Channel *tx_channel_, *rx_channel_;
	double sat_latitude_, sat_longitude_, peer_latitude_, peer_longitude_;	
	int low_lat_flag_, peer_low_lat_flag_;
	double lat_threshold_ = DEG_TO_RAD(latitude_threshold_);
	double cross_long_threshold_ = DEG_TO_RAD(longitude_threshold_);	
	int link_changes_flag_ = FALSE; // Flag indicating change took place 
	int longitude_shutdown_;

	local_ = (SatNode*) node_;
	sat_latitude_ = local_->position()->get_latitude();
	sat_longitude_ = local_->position()->get_longitude();
	if (fabs(sat_latitude_) < lat_threshold_)
		low_lat_flag_ = TRUE; 
	else
		low_lat_flag_ = FALSE;

	// First go through crossseam ISLs to search for handoffs
	for (lhp = local_->linklisthead_.lh_first; lhp; 
	    lhp = lhp->nextlinkhead() ) {
		slhp = (SatLinkHead*) lhp;
		if (slhp->type() != LINK_ISL_CROSSSEAM)  
			continue;
		pos_ = (PolarSatPosition*)slhp->node()->position(); 
		// If this is a crossseam link, first see if the link must 
		// be physically handed off to the next satellite.
		// Handoff is needed if the satellite at the other end of
		// the link is further away than the ``next'' satellite
		// in the peer's orbital plane.
		peer_ = get_peer(slhp);
		if (peer_ == 0)
			continue; // this link interface is not attached
		peer_slhp = get_peer_linkhead(slhp);
		peer_pos_ = (PolarSatPosition*) peer_->position();
		if (peer_pos_->plane() < pos_->plane()) {
			// Crossseam handoff is controlled by satellites
			// in the plane with a lower index
			break;  
		}
		peer_next_pos_ = peer_pos_->next();
		if (!peer_next_pos_) {
			printf("Error:  crossseam handoffs require ");
			printf("setting the ``next'' field\n");
			exit(1);
		}
		peer_next_ = (SatNode*) peer_next_pos_->node();
		dist_to_peer = SatPosition::distance((SatPosition*) peer_pos_,
		    local_->position()); 
		dist_to_next = 
		    SatPosition::distance((SatPosition*) peer_next_pos_,
		    local_->position()); 
		if (dist_to_next < dist_to_peer) {
			// Handoff -- the "next" satellite should have a 
			// currently unused network stack.  Find this 
			// unused stack and handoff the channels to it.
			// 
	 		// Remove peer's tx/rx interface from channel
			peer_slhp->phy_rx()->removechnl();
			peer_slhp->phy_tx()->setchnl(0);
			peer_slhp->phy_rx()->setchnl(0);
			// Add peer_next's tx/rx interfaces to our channels
			peer_next_slhp = get_peer_next_linkhead(peer_next_);
			tx_channel_ = slhp->phy_tx()->channel();
			rx_channel_ = slhp->phy_rx()->channel();
                        peer_next_slhp->phy_tx()->setchnl(rx_channel_);
			peer_next_slhp->phy_rx()->setchnl(tx_channel_);
			peer_next_slhp->phy_rx()->insertchnl(&(tx_channel_->ifhead_));
			link_changes_flag_ = TRUE; 
			// Now reset the peer_ variables to point to next
			peer_ = peer_next_;
			peer_slhp = peer_next_slhp;
		}
		// Next, see if the link needs to be taken down.
		peer_latitude_ = peer_->position()->get_latitude();
		peer_longitude_ = peer_->position()->get_longitude();
		if (fabs(peer_latitude_) < lat_threshold_)
			peer_low_lat_flag_ = TRUE; 
		else
			peer_low_lat_flag_ = FALSE;
		// If the two satellites are too close to each other in
		// longitude, the link should be down
		if ((fabs(peer_longitude_ - sat_longitude_) <
		    cross_long_threshold_) ||
		    fabs(peer_longitude_ - sat_longitude_) >
		    (2 * PI - cross_long_threshold_))
			longitude_shutdown_ = 1;
		else
			longitude_shutdown_ = 0;

		// Evaluate whether a change in link status is needed
		if ((slhp->linkup_ || peer_slhp->linkup_) && 
		    (!low_lat_flag_ || !peer_low_lat_flag_ || 
		    longitude_shutdown_)) {
			slhp->linkup_ = FALSE;
			peer_slhp->linkup_ = FALSE;
			link_changes_flag_ = TRUE;
		} else if ((!slhp->linkup_  || !peer_slhp->linkup_) && 
		    (low_lat_flag_ && peer_low_lat_flag_ &&
		    !longitude_shutdown_)) {
			slhp->linkup_ = TRUE;
			peer_slhp->linkup_ = TRUE;
			link_changes_flag_ = TRUE;
		}
	}

	// Now, work on interplane ISLs (intraplane ISLs are not handed off)
	
	// Now search for interplane ISLs
	for (lhp = local_->linklisthead_.lh_first; lhp; 
	    lhp = lhp->nextlinkhead() ) {
		slhp = (SatLinkHead*) lhp;
		if (slhp->type() != LINK_ISL_INTERPLANE)  
			continue;
		peer_ = get_peer(slhp);
		peer_slhp = get_peer_linkhead(slhp);
		peer_latitude_ = peer_->position()->get_latitude();
		if (fabs(peer_latitude_) < lat_threshold_)
			peer_low_lat_flag_ = TRUE; 
		else
			peer_low_lat_flag_ = FALSE;
		if (slhp->linkup_ && (!low_lat_flag_ || !peer_low_lat_flag_)) {
			// Take links down if either satellite at high latitude
			slhp->linkup_ = FALSE;
			peer_slhp->linkup_ = FALSE;
			link_changes_flag_ = TRUE;
		} else if (!slhp->linkup_ && 
		    (low_lat_flag_ && peer_low_lat_flag_)) {
			slhp->linkup_ = TRUE;
			peer_slhp->linkup_ = TRUE;
			link_changes_flag_ = TRUE;
		}
	}
	if (link_changes_flag_)  {
		SatRouteObject::instance().recompute();
	}
	if (handoff_randomization_) {
		timer_.resched(sat_handoff_int_ + 
		    handoff_rng_.uniform(-1 * sat_handoff_int_/2, 
		    sat_handoff_int_/2));
	} else
		timer_.resched(sat_handoff_int_);
	return link_changes_flag_;
}

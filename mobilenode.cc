/*-*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
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
 */
/* Ported from CMU/Monarch's code, nov'98 -Padma.

 * CMU-Monarch project's Mobility extensions ported by Padma Haldar, 
 * 11/98.
 */


#include <math.h>
#include <stdlib.h>

#include <connector.h>
#include <delay.h>
#include <packet.h>
#include <random.h>

//#include <debug.h>
#include <arp.h>
#include <topography.h>
#include <trace.h>
#include <address.h>
#include <ll.h>
#include <mac.h>
#include <propagation.h>
#include <mobilenode.h>

// XXX Must supply the first parameter in the macro otherwise msvc
// is unhappy. 
static LIST_HEAD(_dummy_MobileNodeList, MobileNode)      nodehead = { 0 };

//static int	MobileNodeIndex = 0;

static class MobileNodeClass : public TclClass {
public:
        MobileNodeClass() : TclClass("Node/MobileNode") {}
        TclObject* create(int, const char*const*) {
                return (new MobileNode);
        }
} class_mobilenode;


/*
 *  PositionHandler()
 *
 *  Updates the position of a mobile node every N seconds, where N is
 *  based upon the speed of the mobile and the resolution of the topography.
 *
 */

void
PositionHandler::handle(Event*)
{
	Scheduler& s = Scheduler::instance();

#if 0
	fprintf(stderr, "*** POSITION HANDLER for node %d (time: %f) ***\n",
		node->address(), s.clock());
#endif
	/*
	 * Update current location
	 */
	node->update_position();

	/*
	 * Choose a new random speed and direction
	 */
#ifdef DEBUG
        fprintf(stderr, "%d - %s: calling random_destination()\n",
                node->address_, __PRETTY_FUNCTION__);
#endif
	node->random_destination();

	s.schedule(&node->pos_handle, &node->pos_intr,
		   node->position_update_interval);
}


/* ======================================================================
   Mobile Node
   ====================================================================== */

MobileNode::MobileNode(void) : Node(), pos_handle(this)
#ifdef NAM_TRACE
        , namChan_(0)
#endif

{
	X = 0.0; Y = 0.0; Z = 0.0; speed = 0.0;
	dX=0.0; dY=0.0; dZ=0.0;
	destX=0.0; destY=0.0;

        // address_ = MobileNodeIndex++;
	random_motion_ = 0;
	base_stn_ = -1;
	T = 0;

	position_update_interval = POSITION_UPDATE_INTERVAL;
	position_update_time = 0.0;
	
	LIST_INSERT_HEAD(&nodehead, this, link);	// node list
	LIST_INIT(&ifhead_);				// interface list
	bind("X_", &X);
	bind("Y_", &Y);
	bind("Z_", &Z);
	bind("speed_", &speed);
	// bind("position_update_interval_", &position_update_interval);
}


int
MobileNode::command(int argc, const char*const* argv)
{
	if(argc == 2) {
		//Tcl& tcl = Tcl::instance();

		// if(strcmp(argv[1], "id") == 0) {
// 			tcl.resultf("%d", address_);
// 			return TCL_OK;
// 		}
		if(strcmp(argv[1], "start") == 0) {
		        start();
			return TCL_OK;
		}		
		if(strcmp(argv[1], "log-movement") == 0) {
#ifdef DEBUG
                        fprintf(stderr,
                                "%d - %s: calling update_position()\n",
                                address_, __PRETTY_FUNCTION__);
#endif

		        update_position();
		        log_movement();
			return TCL_OK;
		}		
		if(strcmp(argv[1], "log-energy") == 0) {
			log_energy(1);
			return TCL_OK;
		}		
	}
	else if(argc == 3) {
                if(strcmp(argv[1], "radius") == 0) {
                        radius_ = strtod(argv[2],NULL);
                        return TCL_OK;
                }
                if(strcmp(argv[1], "namattach") == 0) {
                        Tcl& tcl = Tcl::instance();
                        int mode;
                        const char* id = argv[2];
                        namChan_ = Tcl_GetChannel(tcl.interp(), (char*)id,
                                                  &mode);
                        if (namChan_ == 0) {
                                tcl.resultf("trace: can't attach %s for writing", id);
                                return (TCL_ERROR);
                        }
                        return (TCL_OK);
                }
		if(strcmp(argv[1], "random-motion") == 0) {
			random_motion_ = atoi(argv[2]);
			return TCL_OK;
		}
		else if(strcmp(argv[1], "addif") == 0) {
			WirelessPhy *n = (WirelessPhy*) TclObject::lookup(argv[2]);
			if(n == 0)
				return TCL_ERROR;
			n->insertnode(&ifhead_);
			n->setnode(this);
			return TCL_OK;
		}
		else if(strcmp(argv[1], "topography") == 0) {
			T = (Topography*) TclObject::lookup(argv[2]);
			if(T == 0)
				return TCL_ERROR;
			return TCL_OK;
		}
		else if(strcmp(argv[1], "log-target") == 0) {
			log_target = (Trace*) TclObject::lookup(argv[2]);
			if(log_target == 0)
				return TCL_ERROR;
			return TCL_OK;
		}
		else if (strcmp(argv[1],"base-station") == 0) {
			//base_stn_ = (MobileNode*) TclObject::lookup(argv[2]);
			base_stn_ = atoi(argv[2]);
			if(base_stn_ == -1)
				return TCL_ERROR;
			return TCL_OK;
		} 
	} else if (argc == 5) {
	  if (strcmp(argv[1], "setdest") == 0)
	    { /* <mobilenode> setdest <X> <Y> <speed> */
#ifdef DEBUG
                    fprintf(stderr, "%d - %s: calling set_destination()\n",
                            address_, __FUNCTION__);
#endif
  
	      if(set_destination(atof(argv[2]), atof(argv[3]), atof(argv[4])) < 0)
		return TCL_ERROR;
	      return TCL_OK;
	    }
	}

	return Node::command(argc, argv);
}


/* ======================================================================
   Other class functions
   ====================================================================== */
void
MobileNode::dump(void)
{
	Phy *n;

	fprintf(stdout, "Index: %d\n", address_);
	fprintf(stdout, "Network Interface List\n");
 	for(n = ifhead_.lh_first; n; n = n->nextnode() )
		n->dump();	
	fprintf(stdout, "--------------------------------------------------\n");
}


/* ======================================================================
   Position Functions
   ====================================================================== */
void 
MobileNode::start()
{
	Scheduler& s = Scheduler::instance();

	if(random_motion_ == 0) {
		log_movement();
		return;
	}

	assert(initialized());

	random_position();
#ifdef DEBUG
        fprintf(stderr, "%d - %s: calling random_destination()\n",
                address_, __PRETTY_FUNCTION__);
#endif
	random_destination();

	s.schedule(&pos_handle, &pos_intr, position_update_interval);
}


void 
MobileNode::log_movement()
{
        if (!log_target) return;

	Scheduler& s = Scheduler::instance();

	sprintf(log_target->buffer(),
		"M %.5f %d (%.2f, %.2f, %.2f), (%.2f, %.2f), %.2f",
		s.clock(), address_, X, Y, Z, destX, destY, speed);
	log_target->dump();
}


void
MobileNode::log_energy(int flag)
{
	if(!log_target) return;
	Scheduler &s = Scheduler::instance();
	if (flag) {
	   sprintf(log_target->buffer(),"N -t %f -n %d -e %f", s.clock(), address_,energy()); } else {
	   sprintf(log_target->buffer(),"N -t %f -n %d -e 0 ", s.clock(), address_); 
		}
	log_target->dump();
}

void
MobileNode::idle_energy_patch(float total, float P_idle)
{
	fprintf(stderr,"MobileNode::idle_energy_patch is obsolete!\n");

	/*
       float real_idle = total-(total_sndtime_+total_rcvtime_+total_sleeptime_);
       //printf("total=%f send=%f rcv=%f, slp=%f\n",total, total_sndtime_,total_rcvtime_,total_sleeptime_);
       
       //printf("real_idle=%f\n",real_idle);
       energy_model_-> DecrIdleEnergy(real_idle, P_idle);
       
       //set node energy into zero

       if ((this->energy_model())->energy() < 0) {  
	  // saying node died
	      this->energy_model()->setenergy(0);
	      this->log_energy(0);
       }

	*/
}


void
MobileNode::bound_position()
{
	double minX;
	double maxX;
	double minY;
	double maxY;
	int recheck = 1;

	assert(T != 0);

	minX = T->lowerX();
	maxX = T->upperX();
	minY = T->lowerY();
	maxY = T->upperY();

	while(recheck) {

		recheck = 0;

		if(X < minX) {
			X = minX + (minX - X);
			recheck = 1;
		}
		if(X > maxX) {
			X = maxX - (X - maxX);
			recheck = 1;
		}
		if(Y < minY) {
			Y = minY + (minY - Y);
			recheck = 1;
		}
		if(Y > maxY) {
			Y = maxY- (Y - maxY);
			recheck = 1;
		}
		if(recheck) {
			fprintf(stderr, "Adjust position of node %d\n",address_);
		}
	}
}

int
MobileNode::set_destination(double x, double y, double s)
{
  assert(initialized());

  if(x >= T->upperX() || x <= T->lowerX())
	return -1;
  if(y >= T->upperY() || y <= T->lowerY())
	return -1;

  update_position();		// figure out where we are now

  destX = x;
  destY = y;
  speed = s;

  dX = destX - X;
  dY = destY - Y;
  dZ = 0.0;			// this isn't used, since flying isn't allowed

  if(destX != X || destY != Y) {
	// normalize dx, dy to unit len
  	double len = sqrt( (dX * dX) + (dY * dY) );
	dX /= len;
	dY /= len;
  }
  
  position_update_time = Scheduler::instance().clock();

#ifdef DEBUG
  fprintf(stderr, "%d - %s: calling log_movement()\n", address_, __FUNCTION__);
#endif
  log_movement();

  /* update gridkeeper */
  if (GridKeeper::instance()){
         GridKeeper* gp =  GridKeeper::instance();
         gp-> new_moves(this);
  }                     
               
   
#ifdef NAM_TRACE                
  if (namChan_ != 0) {
                                
     sprintf(nwrk_,     
             "n -t %f -s %d -x %f -y %f -u %f -v %f -T %f",
              Scheduler::instance().clock(),
              nodeid_,
              X,Y,
              speed*dX, speed*dY,
              ((speed*dX) != 0 ) ? (destX-X)/(speed*dX) : speed*dX
             );   
     namdump();         
  }
#endif                  


  return 0;
}

#ifdef NAM_TRACE
void MobileNode::namdump()
{
        int n = 0;
    
        /* Otherwise nwrk_ isn't initialized */
        if (namChan_ != 0)
                n = strlen(nwrk_);
        if ((n > 0) && (namChan_ != 0)) {
                /*
                 * tack on a newline (temporarily) instead
                 * of doing two writes
                 */
                nwrk_[n] = '\n';
                nwrk_[n + 1] = 0;
                (void)Tcl_Write(namChan_, nwrk_, n + 1);
                nwrk_[n] = 0;
        }
}
#endif


void
MobileNode::update_position()
{
	double now = Scheduler::instance().clock();
	double interval = now - position_update_time;

	if(interval == 0.0)
		return;

	X += dX * (speed * interval);
	Y += dY * (speed * interval);

	if ((dX > 0 && X > destX) || (dX < 0 && X < destX))
	  X = destX;		// correct overshoot (slow? XXX)
	if ((dY > 0 && Y > destY) || (dY < 0 && Y < destY))
	  Y = destY;		// correct overshoot (slow? XXX)

	bound_position();

	Z = T->height(X, Y);
#if 0
	fprintf(stderr, "Node: %d, X: %6.2f, Y: %6.2f, Z: %6.2f, time: %f\n",
		address_, X, Y, Z, now);
#endif

	position_update_time = now;
}


void
MobileNode::random_position()
{
	if(T == 0) {
		fprintf(stderr, "No TOPOLOGY assigned\n");
		exit(1);
	}

	X = Random::uniform() * T->upperX();
	Y = Random::uniform() * T->upperY();
	Z = T->height(X, Y);

	position_update_time = 0.0;
}

void
MobileNode::random_destination()
{
	if(T == 0) {
		fprintf(stderr, "No TOPOLOGY assigned\n");
		exit(1);
	}

	random_speed();
#ifdef DEBUG
        fprintf(stderr, "%d - %s: calling set_destination()\n",
                address_, __FUNCTION__);
#endif
	(void) set_destination(Random::uniform() * T->upperX(),
                               Random::uniform() * T->upperY(),
                               speed);
}

void
MobileNode::random_direction()
{
  /* this code isn't used anymore -dam 1/22/98 */
	double len;

	dX = (double) Random::random();
	dY = (double) Random::random();

	len = sqrt( (dX * dX) + (dY * dY) );

	dX /= len;
	dY /= len;
	dZ = 0.0;				// we're not flying...

	/*
	 * Determine the sign of each component of the
	 * direction vector.
	 */
	if(X > (T->upperX() - 2*T->resol())) {
		if(dX > 0) dX = -dX;
	}
	else if(X < (T->lowerX() + 2*T->resol())) {
		if(dX < 0) dX = -dX;
	}
	else if(Random::uniform() <= 0.5) {
		dX = -dX;
	}

	if(Y > (T->upperY() - 2*T->resol())) {
		if(dY > 0) dY = -dY;
	}
	else if(Y < (T->lowerY() + 2*T->resol())) {
		if(dY < 0) dY = -dY;
	}
	else if(Random::uniform() <= 0.5) {
		dY = -dY;
	}
#if 0
	fprintf(stderr, "Location: (%f, %f), Direction: (%f, %f)\n",
		X, Y, dX, dY);
#endif
}

void
MobileNode::random_speed()
{
	speed = Random::uniform() * MAX_SPEED;
}


double
MobileNode::distance(MobileNode *m)
{
	update_position();		// update my position
	m->update_position();		// update m's position

        double Xpos = (X - m->X) * (X - m->X);
        double Ypos = (Y - m->Y) * (Y - m->Y);
	double Zpos = (Z - m->Z) * (Z - m->Z);

        return sqrt(Xpos + Ypos + Zpos);
}


double
MobileNode::propdelay(MobileNode *m)
{
	return distance(m) / SPEED_OF_LIGHT;
}


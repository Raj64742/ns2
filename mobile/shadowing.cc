/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 2000 University of Southern California.
 * All rights reserved.                                            
 *                                                                
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 **
 * Shadowing propation model, including the path-loss model.
 * This statistcal model is applicable for both outdoor and indoor.
 * Wei Ye, weiye@isi.edu, 2000
 */


#include <math.h>

#include <delay.h>
#include <packet.h>

#include <packet-stamp.h>
#include <antenna.h>
#include <mobilenode.h>
#include <propagation.h>
#include <wireless-phy.h>
#include <shadowing.h>


static class ShadowingClass: public TclClass {
public:
	ShadowingClass() : TclClass("Propagation/Shadowing") {}
	TclObject* create(int, const char*const*) {
		return (new Shadowing);
	}
} class_shadowing;


Shadowing::Shadowing()
{
	bind("pathlossExp_", &pathlossExp_);
	bind("std_db_", &std_db_);
	bind("dist0_", &dist0_);
	bind("seed_", &seed_);
	
	ranVar = new RNG;
	ranVar->set_seed(RNG::PREDEF_SEED_SOURCE, seed_);
}


Shadowing::~Shadowing()
{
	delete ranVar;
}


double Shadowing::Pr(PacketStamp *t, PacketStamp *r, WirelessPhy *ifp)
{
	double L = ifp->getL();		// system loss
	double lambda = ifp->getLambda();   // wavelength

	double Xt, Yt, Zt;		// loc of transmitter
	double Xr, Yr, Zr;		// loc of receiver

	t->getNode()->getLoc(&Xt, &Yt, &Zt);
	r->getNode()->getLoc(&Xr, &Yr, &Zr);

	// Is antenna position relative to node position?
	Xr += r->getAntenna()->getX();
	Yr += r->getAntenna()->getY();
	Zr += r->getAntenna()->getZ();
	Xt += t->getAntenna()->getX();
	Yt += t->getAntenna()->getY();
	Zt += t->getAntenna()->getZ();

	double dX = Xr - Xt;
	double dY = Yr - Yt;
	double dZ = Zr - Zt;
	double dist = sqrt(dX * dX + dY * dY + dZ * dZ);

	// get antenna gain
	double Gt = t->getAntenna()->getTxGain(dX, dY, dZ, lambda);
	double Gr = r->getAntenna()->getRxGain(dX, dY, dZ, lambda);

	// calculate receiving power at reference distance
	double Pr0 = Friis(t->getTxPr(), Gt, Gr, lambda, L, dist0_);

	// calculate average power loss predicted by path loss model
	double avg_db = -10.0 * pathlossExp_ * log10(dist/dist0_);
   
	// get power loss by adding a log-normal random variable (shadowing)
	// the power loss is relative to that at reference distance dist0_
	double powerLoss_db = avg_db + ranVar->normal(0.0, std_db_);

	// calculate the receiving power at dist
	double Pr = Pr0 * pow(10.0, powerLoss_db/10.0);
	
	return Pr;
}


int Shadowing::command(int argc, const char* const* argv)
{
	if (argc == 4) {
		if (strcmp(argv[1], "seed") == 0) {
			int s = atoi(argv[3]);
			if (strcmp(argv[2], "raw") == 0) {
				ranVar->set_seed(RNG::RAW_SEED_SOURCE, s);
			} else if (strcmp(argv[2], "predef") == 0) {
				ranVar->set_seed(RNG::PREDEF_SEED_SOURCE, s);
				// s is the index in predefined seed array
				// 0 <= s < 64
			} else if (strcmp(argv[2], "heuristic") == 0) {
				ranVar->set_seed(RNG::HEURISTIC_SEED_SOURCE, 0);
			}
			return(TCL_OK);
		}
	}
	
	return Propagation::command(argc, argv);
}


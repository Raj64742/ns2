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


#ifndef shadowing_h
#define shadowing_h

#include <packet-stamp.h>
#include <wireless-phy.h>
#include <propagation.h>
#include <rng.h>

class Shadowing : public Propagation {
public:
	Shadowing();
	~Shadowing();
	virtual double Pr(PacketStamp *tx, PacketStamp *rx, WirelessPhy *ifp);
	virtual int command(int argc, const char*const* argv);

protected:
	RNG *ranVar;	// random number generator for normal distribution
	
	double pathlossExp_;	// path-loss exponent
	double std_db_;		// shadowing deviation (dB),
	double dist0_;	// close-in reference distance
	int seed_;	// seed for random number generator
};

#endif


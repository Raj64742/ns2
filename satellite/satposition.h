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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/satellite/satposition.h,v 1.4 1999/07/18 20:02:09 tomh Exp $
 *
 * Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
 */

#ifndef __satposition_h__
#define __satposition_h__

#include <trace.h>
#include <list.h>
#include <phy.h>
#include <node.h>
#include <stdlib.h>
#include "object.h"
#include "sat.h"

class SatPosition : public TclObject {
 public:
	SatPosition();
	int type() { return type_; }
	Node* node() { return node_; }
	virtual coordinate getCoordinate() = 0; 
	virtual double get_latitude() = 0;
	virtual double get_longitude() = 0;
	static double distance(SatPosition*, SatPosition*);
	double propdelay(SatPosition*);
	static void spherical_to_cartesian(double, double, double,
	    double &, double &, double &);

	// configuration parameters
	static double time_advance_;
 protected:
        int command(int argc, const char*const* argv);
	coordinate initial_;
	int type_;
	Node* node_;
};

class PolarSatPosition : public SatPosition {
 public:
	PolarSatPosition(double = 1000, double = 90, double = 0, double = 0, 
            double = 0);
	virtual coordinate getCoordinate();
	void set(double Altitude, double Lon, double Alpha, double inclination=90); 
	virtual double get_latitude(); 
	virtual double get_longitude(); 
	double get_altitude() { return initial_.r; }
	PolarSatPosition* next() { return next_; }
	int plane() { return plane_; }
	static int num_planes() { return num_planes_; }

 protected:
        int command(int argc, const char*const* argv);
        PolarSatPosition* next_;    // Next intraplane satellite
	int plane_;  // Orbital plane that this satellite resides in
	static int num_planes_; // Number of planes
	double inclination_; // radians

	
};

class GeoSatPosition : public SatPosition {
 public:
	GeoSatPosition(double longitude = 0);
	virtual coordinate getCoordinate();
	void set(double longitude); 
	virtual double get_latitude() { return 0; } 
	virtual double get_longitude(); 
 protected:
};

class TermSatPosition : public SatPosition {
 public:
	TermSatPosition(double = 0, double = 0);
	virtual coordinate getCoordinate();
	void set(double latitude, double longitude);
	virtual double get_latitude(); 
	virtual double get_longitude(); 
 protected:
};

#endif __satposition_h__

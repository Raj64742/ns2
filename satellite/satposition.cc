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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/satellite/satposition.cc,v 1.3 1999/07/02 21:02:08 haoboy Exp $";
#endif

#include "satposition.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static class TermSatPositionClass : public TclClass {
public:
	TermSatPositionClass() : TclClass("Position/Sat/Term") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc == 5) {
			float a, b;
			sscanf(argv[4], "%f %f", &a, &b);
			return (new TermSatPosition(a, b));
		} else
			return (new TermSatPosition);
	}
} class_term_sat_position;

static class PolarSatPositionClass : public TclClass {
public:
	PolarSatPositionClass() : TclClass("Position/Sat/Polar") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc == 5) {
			float a = 0, b = 0, c = 0, d = 0, e = 0;
			sscanf(argv[4], "%f %f %f %f %f", &a, &b, &c, &d, &e);
			return (new PolarSatPosition(a, b, c, d, e));
		}
		else {
			return (new PolarSatPosition);
		}
	}
} class_polarsatposition;

static class GeoSatPositionClass : public TclClass {
public:
	GeoSatPositionClass() : TclClass("Position/Sat/Geo") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc == 5) 
			return (new GeoSatPosition(float(atof(argv[4]))));
		else 
			return (new GeoSatPosition);
	}
} class_geosatposition;

static class SatPositionClass : public TclClass {
public:
	SatPositionClass() : TclClass("Position/Sat") {}
	TclObject* create(int argc, const char*const* argv) {
			printf("Error:  do not instantiate Position/Sat\n");
			return (0);
	}
} class_satposition;

int PolarSatPosition::num_planes_ = 0;
double SatPosition::time_advance_ = 0;

SatPosition::SatPosition() : node_(0)  
{
        bind("time_advance_", &time_advance_);
}

// Returns distance in km between two SatNodes
float SatPosition::distance(SatPosition* first, SatPosition* second) 
{
	coordinate a = first->getCoordinate();
	coordinate b = second->getCoordinate();
	float s_x, s_y, s_z, e_x, e_y, e_z;     // cartesian
	spherical_to_cartesian(a.r, a.theta, a.phi, s_x, s_y, s_z);
	spherical_to_cartesian(b.r, b.theta, b.phi, e_x, e_y, e_z);
	return (DISTANCE(s_x, s_y, s_z, e_x, e_y, e_z));

}

// Returns propagation delay in s between two SatNodes
float SatPosition::propdelay(SatPosition* remote)
{
	return (distance(this, remote)/LIGHT);
}

void SatPosition::spherical_to_cartesian(float R, float Theta,
    float Phi, float &X, float &Y, float &Z)
{       
        X = R * sin(Theta) * cos (Phi);
        Y = R * sin(Theta) * sin (Phi);
        Z = R * cos(Theta);
}
 
int SatPosition::command(int argc, const char*const* argv) {     
	//Tcl& tcl = Tcl::instance();
	if (argc == 2) {
	}
	if (argc == 3) {
		if(strcmp(argv[1], "setnode") == 0) {
			node_ = (Node*) TclObject::lookup(argv[2]);
			if (node_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		}
	}
	return (TclObject::command(argc, argv));
}

/////////////////////////////////////////////////////////////////////
// class TermSatPosition
/////////////////////////////////////////////////////////////////////

// Specify initial coordinates.  Default coordinates place the terminal
// on the Earth's surface at 0 deg lat, 0 deg long.
TermSatPosition::TermSatPosition(float Theta, float Phi)  {
	initial_.r = EARTH_RADIUS;  	
	set(Theta, Phi);
	type_ = POSITION_SAT_TERM;
}

//
// Convert user specified latitude and longitude to our spherical coordinates
// Latitude is in the range (-90, 90) with neg. values -> south
// Initial_.theta is stored from 0 to PI (spherical)
// Longitude is in the range (-180, 180) with neg. values -> west
// Initial_.phi is stored from 0 to 2*PI (spherical)
//
void TermSatPosition::set(float latitude, float longitude)
{
	if (latitude < -90 || latitude > 90)
		fprintf(stderr, "TermSatPosition:  latitude out of bounds %f\n",
		   latitude);
	if (longitude < -180 || longitude > 180)
		fprintf(stderr, "TermSatPosition: longitude out of bounds %f\n",
		    longitude);
	initial_.theta = DEG_TO_RAD(90 - latitude);
	if (longitude < 0)
		initial_.phi = DEG_TO_RAD(360 + longitude);
	else
		initial_.phi = DEG_TO_RAD(longitude);
}

// Returns longitude in radians,  ranging from -PI to PI
// This returns earth-centric longitude, for trace purposes (coordinates
// stored in constellation-centric coordinate system)
float TermSatPosition::get_longitude()
{
	float earth_longitude = initial_.phi;
	if (earth_longitude > PI)
		return (-(2*PI - earth_longitude));
	else
		return (earth_longitude);
}

// Returns latitude in radians, in the range from -PI/2 to PI/2
// This returns earth-centric longitude, for trace purposes (coordinates
// stored in constellation-centric coordinate system)
float TermSatPosition::get_latitude()
{
	coordinate temp = getCoordinate();
	return (PI/2 - temp.theta);
}

coordinate TermSatPosition::getCoordinate()
{
	coordinate current;
	float period = EARTH_PERIOD; // seconds

	current.r = initial_.r;
	current.theta = initial_.theta;
	current.phi = fmod((initial_.phi + 
	    (fmod(NOW + time_advance_,period)/period) * 2*PI), 2*PI);

#ifdef POINT_TEST
	current = initial_; // debug option to stop earth's rotation
#endif
	return current;
}

/////////////////////////////////////////////////////////////////////
// class PolarSatPosition
/////////////////////////////////////////////////////////////////////

PolarSatPosition::PolarSatPosition(float altitude, float Inc, float Lon, 
    float Alpha, float Plane) : next_(0), plane_(0) {
	set(altitude, Lon, Alpha, Inc);
        if (Plane) {
		plane_ = int(Plane);
		if (plane_ > num_planes_)
			num_planes_ = plane_;
	}
	type_ = POSITION_SAT_POLAR;
}

//
// Since it is easier to compute instantaneous orbit position based on a
// coordinate system centered on the orbit itself, we keep initial coordinates
// specified in terms of the satellite orbit, and convert to true spherical 
// coordinates in getCoordinate().
// Initial satellite position is specified as follows:
// initial_.theta specifies initial angle with respect to "ascending node"
// i.e., zero is at equator (ascending)-- this is the $alpha parameter in Otcl
// initial_.phi specifies East longitude of "ascending node"  
// -- this is the $lon parameter in OTcl
// Note that with this system, only theta changes with time
//
void PolarSatPosition::set(float Altitude, float Lon, float Alpha, float Incl)
{
	if (Altitude < 0) {
		fprintf(stderr, "PolarSatPosition:  altitude out of \
		   bounds: %f\n", Altitude);
		exit(1);
	}
	initial_.r = Altitude + 6378; // Altitude in km above the earth
	if (Alpha < 0 || Alpha >= 360) {
		fprintf(stderr, "PolarSatPosition:  alpha out of bounds: %f\n", 
		    Alpha);
		exit(1);
	}
	initial_.theta = DEG_TO_RAD(Alpha);
	if (Lon < -180 || Lon > 180) {
		fprintf(stderr, "PolarSatPosition:  lon out of bounds: %f\n", 
		    Lon);
		exit(1);
	}
	if (Lon < 0)
		initial_.phi = DEG_TO_RAD(360 + Lon);
	else
		initial_.phi = DEG_TO_RAD(Lon);
	if (Incl < 0 || Incl > 180) {
		// retrograde orbits = (90 < Inclination < 180)
		fprintf(stderr, "PolarSatPosition:  inclination out of \
		    bounds: %f\n", Incl); 
		exit(1);
	}
	inclination_ = DEG_TO_RAD(Incl);
}

//
// The initial coordinate has the following properties:
// theta: 0 < theta < 2 * PI (essentially, this specifies initial position)  
// phi:  0 < phi < 2 * PI (longitude of ascending node)
// Return a coordinate with the following properties (i.e. convert to a true
// spherical coordinate):
// theta:  0 < theta < PI
// phi:  0 < phi < 2 * PI
//
coordinate PolarSatPosition::getCoordinate()
{
	coordinate current;
	double partial;  // fraction of orbit period completed
	// XXX: can't use "num = pow(initial_.r,3)" here because of linux lib
	double num = initial_.r * initial_.r * initial_.r;
	double period = 2 * PI * sqrt(num/398601.2); // seconds
	partial = 
	    (fmod(NOW + time_advance_, period)/period) * 2*PI; //rad
	double theta_cur, phi_cur, theta_new, phi_new;

	// Compute current orbit-centric coordinates:
	// theta_cur adds effects of time (orbital rotation) to initial_.theta
	theta_cur = fmod(initial_.theta + partial, 2*PI);
	phi_cur = initial_.phi;
	// Reminder:  theta_cur and phi_cur are temporal translations of 
	// initial parameters and are NOT true spherical coordinates.
	//
	// Now generate actual spherical coordinates,
	// with 0 < theta_new < PI and 0 < phi_new < 360

	if (inclination_ < PI) {
		// asin returns value between -PI/2 and PI/2, so 
		// theta_new guaranteed to be between 0 and PI
		theta_new = PI/2 - asin(sin(inclination_) * sin(theta_cur));
		// if theta_new is between PI/2 and 3*PI/2, must correct
		// for return value of atan()
		if (theta_cur > PI/2 && theta_cur < 3*PI/2)
			phi_new = atan(cos(inclination_) * tan(theta_cur)) + 
			    phi_cur + PI;
		else
			phi_new = atan(cos(inclination_) * tan(theta_cur)) + 
			    phi_cur;
		phi_new = fmod(phi_new + 2*PI, 2*PI);
	} else 
		printf("ERROR:  inclination_ > PI\n");
	
	current.r = initial_.r;
	current.theta = (float)theta_new;
	current.phi = (float)phi_new;
	return current;
}

// Returns (earth-centric) longitude of satellite nadir point in radians,  
// ranging from -PI to PI
float PolarSatPosition::get_longitude()
{
	float period = EARTH_PERIOD; // period of earth in seconds
	// use getCoordinate() to get constellation-centric latitude and
	// longitude
	coordinate temp = getCoordinate();
	// adjust longitude so that it is earth-centric (i.e., account
	// for earth rotating beneath)
	float earth_longitude = fmod((temp.phi - 
	    (fmod(NOW + time_advance_,period)/period) * 2*PI), 2*PI);
	if (earth_longitude > PI)
		return (-(2*PI - earth_longitude));
	else
		return (earth_longitude);
}

// Returns (earth-centric) latitude of satellite nadir point in radians,  
// in the range from -PI/2 to PI/2
float PolarSatPosition::get_latitude()
{
	coordinate temp = getCoordinate();
	return (PI/2 - temp.theta);
}

int PolarSatPosition::command(int argc, const char*const* argv) {     
	Tcl& tcl = Tcl::instance();
        if (argc == 2) {
	}
	if (argc == 3) {
		if (strcmp(argv[1], "set_plane") == 0) {
			plane_ = atoi(argv[2]);
			if (plane_ > PolarSatPosition::num_planes_)
				PolarSatPosition::num_planes_ = plane_;
			return (TCL_OK);
		}
		else if (strcmp(argv[1], "set_next") == 0) {
			next_ = (PolarSatPosition *) TclObject::lookup(argv[2]);
			if (next_ == 0) {
				tcl.resultf("no such object %s", argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
	}
	return (SatPosition::command(argc, argv));
}

/////////////////////////////////////////////////////////////////////
// class GeoSatPosition
/////////////////////////////////////////////////////////////////////

GeoSatPosition::GeoSatPosition(float longitude) 
{
	initial_.r = EARTH_RADIUS + GEO_ALTITUDE;
	initial_.theta = PI/2;
	set(longitude);
	type_ = POSITION_SAT_GEO;
}

coordinate GeoSatPosition::getCoordinate()
{
	coordinate current;
	current.r = initial_.r;
	current.theta = initial_.theta;
	double fractional = 
	    (fmod(NOW + time_advance_, 86164)/86164) * 2*PI; // radians
	current.phi = fmod(initial_.phi + fractional, 2*PI);
	return current;
}

// Returns (earth-centric) longitude in radians,  ranging from -PI to PI
float GeoSatPosition::get_longitude()
{
	float earth_longitude = initial_.phi;
	if (earth_longitude > PI)
		return (-(2*PI - earth_longitude));
	else
		return (earth_longitude);
}

//
// Longitude is in the range (0, 180) with negative values -> west
//
void GeoSatPosition::set(float longitude)
{
	if (longitude < -180 || longitude > 180)
		fprintf(stderr, "GeoSatPosition:  longitude out of bounds %f\n",
		    longitude);
	if (longitude < 0)
		initial_.phi = DEG_TO_RAD(360 + longitude);
	else
		initial_.phi = DEG_TO_RAD(longitude);
}

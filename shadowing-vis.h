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
 * Visibiliy-based Shadowing propation model. A bitmap is used for the
 * simulation environment. It has 2 shadowing model instances that
 * represent good (has line-of-sight path) and bad (obstructed)
 * propagation conditions. 
 * Wei Ye, weiye@isi.edu, 2000
 */

#ifndef visibiliy_shadowing_h
#define visibiliy_shadowing_h

#include <shadowing.h>
#include <packet-stamp.h>
#include <wireless-phy.h>
#include <propagation.h>

class ShadowingVis : public Propagation {
public:
	ShadowingVis();
	virtual double Pr(PacketStamp *tx, PacketStamp *rx, WirelessPhy *ifp);
	virtual int command(int argc, const char*const* argv);
	
	// bitmap handling
	void loadImg(const char* fname);
	inline char get_pixel(int x, int y) {
		if (x<0 || x>=width || y<0 || y>=height) return 0;
		return pixel[x + y * width];
	}

/*	
	inline void set_pixel(int x, int y, char c) {
		if (x<0 || x>=width || y<0 || y>=height) return;
		pixel[(x + y * width)] = c;
	}

	
	inline unsigned int RGB( int r, int g, int b ) {
		return (((r << 8) | g) << 8) | b;
	}
*/

	// get prop condition: good or bad
	int getPropCond(float xr, float yr, float xt, float yt);

protected:
	Shadowing *goodProp, *badProp;	// two shadowing models
	unsigned char *pixel;  // bitmap pixels
    int	width, height;
	int ppm;	// number of pixels per meter

};

#endif


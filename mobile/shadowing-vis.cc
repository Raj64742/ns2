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
 * Visibility-Shadowing propation model. It has 2 shadowing model instances
 * that represent good (has line-of-sight path) and bad (obstructed)
 * propagation conditions.
 * Wei Ye, weiye@isi.edu, 2000
 */

#include "config.h"

#include <math.h>
#include <fstream>


#include <delay.h>
#include <packet.h>
#include <packet-stamp.h>
#include <antenna.h>
#include <mobilenode.h>
#include <propagation.h>
#include <wireless-phy.h>
#include <shadowing-vis.h>

#define BLACK 0
//#define WHITE 0xff

//#define DEBUG

static class ShadowingVisClass: public TclClass {
public:
	ShadowingVisClass() : TclClass("Propagation/ShadowingVis") {}
	TclObject* create(int, const char*const*) {
		return (new ShadowingVis);
	}
} class_visibility_shadowing;


ShadowingVis::ShadowingVis()
{
	goodProp = NULL;
	badProp = NULL;
}


double ShadowingVis::Pr(PacketStamp *t, PacketStamp *r, WirelessPhy *ifp)
{

	double Xt, Yt, Zt;		// loc of transmitter
	double Xr, Yr, Zr;		// loc of receiver
	double Pr;			// received signal power

	t->getNode()->getLoc(&Xt, &Yt, &Zt);
	r->getNode()->getLoc(&Xr, &Yr, &Zr);

	// Is antenna position relative to node position?
	Xt += t->getAntenna()->getX();
	Yt += t->getAntenna()->getY();
	Xr += r->getAntenna()->getX();
	Yr += r->getAntenna()->getY();
		
	// Since we use a bitmap, it's a 2-d environment.
	// So Zt and Zr are omitted.
	
	//get propagation conidition between T/R
	int propCond = getPropCond(Xt, Yt, Xr, Yr);
	if (propCond == 1) {
		Pr = goodProp->Pr(t, r, ifp);
#ifdef DEBUG
		printf("Good propagation condition!\n");
#endif
	} else {
		Pr = badProp->Pr(t, r, ifp);
#ifdef DEBUG
		printf("Bad propagation condition!\n");
#endif
	}
		
	return Pr;
}


int ShadowingVis::command(int argc, const char*const* argv)
{
	TclObject *obj;
	
	if (argc == 3) {
		if (strcmp(argv[1], "get-bitmap") == 0) {
			loadImg(argv[2]);
			return(TCL_OK);
		}
		if (strcmp(argv[1], "set-ppm") == 0) {
			ppm = atoi(argv[2]);
			return(TCL_OK);
		}
	}
	if (argc == 4) {
		if (strcmp(argv[1], "add-models") == 0) {
			if( (obj = TclObject::lookup(argv[2])) == 0) {
				fprintf(stderr, "ShadowingVis: %s lookup of %s failed\n", \
					argv[1], argv[2]);
				return TCL_ERROR;
			} else {
				goodProp = (Shadowing*) obj;
			}
			if( (obj = TclObject::lookup(argv[3])) == 0) {
				fprintf(stderr, "ShadowingVis: %s lookup of %s failed\n", \
					argv[1], argv[3]);
				return TCL_ERROR;
			} else {
				badProp = (Shadowing*) obj;
			}
			return(TCL_OK);
		}
	}
	
	return Propagation::command(argc, argv);
}


void ShadowingVis::loadImg(const char* fname)
{
#ifdef DEBUG
	printf("loading %s\n", fname);
#endif

	ifstream image( fname );

	char magicNumber[10];
	char comment[256];
	int whiteNum;
	char terminator;

	if( image.fail() ) printf("image bad!\n");

	if( strstr( fname, ".pnm" ) ) {
		image >> magicNumber;
		image.get( terminator );
		image.get( comment, 255, '\n');
		image.get( terminator );
		image >> width >> height >> whiteNum;
	} else if( strstr( fname, ".pgm" ) ) {
		image >> magicNumber;
		image.get( terminator );
		image >> width >> height >> whiteNum;
	}

#ifdef DEBUG
    printf("magic: %s\ncomment: %s\nwidth: %d height: %d white: %d\n", magicNumber, comment, width, height, whiteNum);
#endif

	pixel = new unsigned char[width * height];
	//unsigned char a;   #Sun OS compilation Fix - fcartegn@univ-valenciennes.fr - Apr 26 02
	char a;
	image.get(a);	// this byte is not the image

	// the upper-left corner is the origin (0, 0)
	for(int n = 0; n < height; n++) {
		for(int m = 0; m < width; m++) {
			image.get( a );
			pixel[m + n * width] = a;
		}
	}

#ifdef DEBUG
	FILE *testFile;
	char *filename = "imgtest.hex";
	testFile = fopen(filename, "w");
	for (int n = 0; n < height; n++) {
	  for (int m = 0; m < width; m++)
		if (pixel[m + n * height] == BLACK)
			fprintf(testFile, "%x", pixel[m + n * height]);
		else
			fprintf(testFile, "%x", 0xf);
		fprintf(testFile, "\n");
	}
	fclose(testFile);
#endif

	image.close();
}


int ShadowingVis::getPropCond(float xt, float yt, float xr, float yr)
{
#ifdef DEBUG
	printf("xt = %f yt = %f xr = %f yr = %f\n", xt, yt, xr, yr);
#endif
	float dx = xr - xt;
	float dy = yr - yt;
	float maxDistSq = dx * dx + dy * dy;
	float maxDist = sqrt(maxDistSq);
	dx /= maxDist;
	dy /= maxDist;
	maxDistSq = (float)(ppm * ppm) * maxDistSq;

	float xtPxl = (float)ppm * xt;
	float ytPxl = (float)ppm * yt;

	// Now each time there is only one pixel is test along the line from
	// the transmitter to the receiver. If ppm is large, more pixels 
	// can be tested: if the line of sight path is just obstructed by
	// a very small object, the propagation is still good
	int goodProp = 1;
/*
	float distSq = 0; 
	while (distSq < maxDistSq) {
		int xIdx = (int)(xx * ppm);
		int yIdx = (int)(yy * ppm);
		if ( pixel[xIdx + yIdx * width] != BLACK ) {
			goodProp = 0;
#ifdef DEBUG
			cout << "xx = " << xx << " yy = " << yy << endl;
			printf("pixel = %x\n", pixel[xIdx + yIdx * width]);
#endif
			break;
		}
		xx += dx;
		yy += dy;
		distSq = (xx - xt) * (xx - xt) + (yy - yt) * (yy - yt);
	}
*/
/*
	int maxRange = (int)(maxDist * ppm); // distance times pixels-per-meter
	for (int rng = 0; rng < maxRange; rng++) {
		if (pixel[(int)xx + (int)yy * width] != BLACK) {
#ifdef DEBUG
			cout << "xx = " << xx << " yy = " << yy << endl;
			printf("pixel = %x\n", pixel[(int)xx + (int)yy * width]);
#endif
			goodProp = 0;
			break;
		}
		xx += dx;
		yy += dy;
	}
*/
	
	float distX = 0;
	float distY = 0;
	float distSq = 0;
	while (distSq < maxDistSq) {
		int xIdx = (int)(xtPxl + distX);
		int yIdx = (int)(ytPxl + distY);
		if (pixel[xIdx + yIdx * width] != BLACK) {
			goodProp = 0;
#ifdef DEBUG
			printf("xx = %f  yy = %f\n", xx, yy);
			printf("pixel = %x\n", pixel[xIdx + yIdx * width]);
#endif
			break;
		}
		distX += dx;
		distY += dy;
		distSq = distX * distX + distY * distY;
	}
	
	return goodProp;
}


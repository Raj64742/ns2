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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/sat.h,v 1.2 1999/07/18 20:02:11 tomh Exp $
 *
 * Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
 */

#ifndef __ns_sat_h__
#define __ns_sat_h__

// Various constants
#define EARTH_RADIUS 6378  // km
#define EARTH_PERIOD 86164 // seconds
#define GEO_ALTITUDE 35786 // km
#define PI 3.1415926535897
#define LIGHT 299793 // km/s
#define DEG_TO_RAD(x) ((x) * PI/180)
#define RAD_TO_DEG(x) ((x) * 180/PI)
#define DISTANCE(s_x, s_y, s_z, e_x, e_y, e_z) (sqrt((s_x - e_x) * (s_x - e_x) \
                + (s_y - e_y) * (s_y - e_y) + (s_z - e_z) * (s_z - e_z)))

// Link types
#define LINK_GENERIC 1
#define LINK_GSL_GEO 2 
#define LINK_GSL_POLAR 3 
#define LINK_GSL_REPEATER 4 
#define LINK_GSL 5 
#define LINK_ISL_INTRAPLANE 6 
#define LINK_ISL_INTERPLANE 7 
#define LINK_ISL_CROSSSEAM 8 

// Position types
#define POSITION_SAT 1
#define POSITION_SAT_POLAR 2
#define POSITION_SAT_GEO 3 
#define POSITION_SAT_TERM 4 

// Handoff manager types
#define LINKHANDOFFMGR_SAT 1
#define LINKHANDOFFMGR_TERM 2


struct coordinate {
        double r;        // km
        double theta;    // radians
        double phi;      // radians
        // Convert to cartesian as follows:
        // x = rsin(theta)cos(phi)
        // y = rsin(theta)sin(phi)
        // z = rcos(theta)
};

#endif __ns_sat_h__

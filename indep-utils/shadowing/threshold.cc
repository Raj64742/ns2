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
 * Calculating the receiving threshold (RXThresh_ for Phy/Wireless) when
 * the shadowing propagation model is used.
 * Input: distance, percentage of correct reception
 *	  (Other parameters can be changed directly in the code.)
 * Output: receiving threshold (RXThresh_)
 * 
 * Wei Ye, weiye@isi.edu, 2000
 */


#include <math.h>
#include <stdlib.h>
#include <iostream.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double Friss(double Pt, double Gt, double Gr, double lambda, double L, double d)
{
        /*
         * Friss free space propagation equation:
         *
         *       Pt * Gt * Gr * (lambda^2)
         *   P = --------------------------
         *       (4 *pi * d)^2 * L
         */
  double M = lambda / (4 * M_PI * d);
  return (Pt * Gt * Gr * (M * M)) / L;
}


// inverse of complementary error function
// y = erfc(x) --> x = inv_erfc(y)

double inv_erfc(double y)
{
    double s, t, u, w, x, z;

    z = y;
    if (y > 1) {
        z = 2 - y;
    }
    w = 0.916461398268964 - log(z);
    u = sqrt(w);
    s = (log(u) + 0.488826640273108) / w;
    t = 1 / (u + 0.231729200323405);
    x = u * (1 - s * (s * 0.124610454613712 + 0.5)) -
        ((((-0.0728846765585675 * t + 0.269999308670029) * t +
        0.150689047360223) * t + 0.116065025341614) * t +
        0.499999303439796) * t;
    t = 3.97886080735226 / (x + 3.97886080735226);
    u = t - 0.5;
    s = (((((((((0.00112648096188977922 * u +
        1.05739299623423047e-4) * u - 0.00351287146129100025) * u -
        7.71708358954120939e-4) * u + 0.00685649426074558612) * u +
        0.00339721910367775861) * u - 0.011274916933250487) * u -
        0.0118598117047771104) * u + 0.0142961988697898018) * u +
        0.0346494207789099922) * u + 0.00220995927012179067;
    s = ((((((((((((s * u - 0.0743424357241784861) * u -
        0.105872177941595488) * u + 0.0147297938331485121) * u +
        0.316847638520135944) * u + 0.713657635868730364) * u +
        1.05375024970847138) * u + 1.21448730779995237) * u +
        1.16374581931560831) * u + 0.956464974744799006) * u +
        0.686265948274097816) * u + 0.434397492331430115) * u +
        0.244044510593190935) * t -
        z * exp(x * x - 0.120782237635245222);
    x += s * (x * s + 1);
    if (y > 1) {
        x = -x;
    }
    return x;
}


// Inverse of Q-function
// y = Q(x) --> x = inv_Q(y)

double inv_Q(double y)
{
	double x;
	x = sqrt(2.0) * inv_erfc(2.0 * y);
	return x;
}


void main(int argc, char** argv)
{
	double dist0_, sysLoss, Gt, Gr, freq, lambda, Pr0;
	double Pt, dist, prob, rxThresh_;
	double pathlossExp_, std_db_;
	dist = atof(argv[1]);
	prob = atof(argv[2]);
	dist0_ = 1.0;  // reference distance
	sysLoss = 1.0;
	Gt = 1.0;
	Gr = 1.0;
	Pt = 0.2818;
	freq = 914.0e6;  // frequency
	lambda = 3.0e8/freq;
	pathlossExp_ = 2.0;
	std_db_ = 4.0;

	// calculate receiving power at reference distance
	Pr0 = Friss(Pt, Gt, Gr, lambda, sysLoss, dist0_);
	cout << "Pr0 = " << Pr0 << endl;

	// calculate average power loss predicted by path loss model
	double avg_db = -10.0 * pathlossExp_ * log10(dist/dist0_);
	cout << "avg_db = " << avg_db << endl;

	// calculate the the threshold
	double invq = inv_Q(prob);
	cout << "invq = " << invq << endl;
	double threshdb = invq * std_db_ + avg_db;
	cout << "threshdb = " << threshdb << endl;
	rxThresh_ = Pr0 * pow(10.0, threshdb/10.0);

	cout << "Receiving threshold is: " << rxThresh_ << endl;
}
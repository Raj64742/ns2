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
 * Calculating the receiving threshold (RXThresh_ for Phy/Wireless)
 * Wei Ye, weiye@isi.edu, 2000
 */


#include <math.h>
#include <stdlib.h>
#include <iostream.h>

#ifndef M_PI
#define M_PI 3.14159265359
#endif

double Friis(double Pt, double Gt, double Gr, double lambda, double L, double d)
{
        /*
         * Friis free space propagation equation:
         *
         *       Pt * Gt * Gr * (lambda^2)
         *   P = --------------------------
         *       (4 *pi * d)^2 * L
         */
  double M = lambda / (4 * M_PI * d);
  return (Pt * Gt * Gr * (M * M)) / L;
}

double TwoRay(double Pt, double Gt, double Gr, double ht, double hr, double L, double d, double lambda)
{
        /*
         *  if d < crossover_dist, use Friis free space model
         *  if d >= crossover_dist, use two ray model
         *
         *  Two-ray ground reflection model.
         *
         *	     Pt * Gt * Gr * (ht^2 * hr^2)
         *  Pr = ----------------------------
         *           d^4 * L
         *
         * The original equation in Rappaport's book assumes L = 1.
         * To be consistant with the free space equation, L is added here.
         */

	double Pr;  // received power
	double crossover_dist = (4 * M_PI * ht * hr) / lambda;

	if (d < crossover_dist)
		Pr = Friis(Pt, Gt, Gr, lambda, L, d);
	else
		Pr = Pt * Gt * Gr * (hr * hr * ht * ht) / (d * d * d * d * L);
		
	return Pr;
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


int main(int argc, char** argv)
{
	
	// specify default values
	char** propModel = NULL;       // propagation model
	double Pt = 0.28183815;            // transmit power
	double Gt = 1.0;               // transmit antenna gain
	double Gr = 1.0;               // receive antenna
	double freq = 914.0e6;         // frequency
	double sysLoss = 1.0;          // system loss
	
	// for two-ray model
	double ht = 1.5;               // transmit antenna height
	double hr = 1.5;               // receive antenna height
	
	// for shadowing model
	double pathlossExp_ = 2.0;     // path loss exponent
	double std_db_ = 4.0;          // shadowing deviation
	double dist0_ = 1.0;           // reference distance
	double prob = 0.95;            // correct reception rate
	
	double rxThresh_;              // receiving threshold

	// check arguments	
	if (argc < 4) {
		cout << "USAGE: find receiving threshold for certain communication range (distance)" << endl;
		cout << endl;
		cout << "SYNOPSIS: threshold -m <propagation-model> [other-options] distance" << endl;
		cout << endl;
		cout << "<propagation-model>: FreeSpace, TwoRayGround or Shadowing" << endl;
		cout << "[other-options]: set parameters other than default values:" << endl;
		cout << endl << "Common parameters:" << endl;
		cout << "-Pt <transmit-power>" << endl;
		cout << "-fr <frequency>" << endl;
		cout << "-Gt <transmit-antenna-gain>" << endl;
		cout << "-Gr <receive-antenna-gain>" << endl;
		cout << "-L <system-loss>" << endl;
		cout << endl << "For two-ray ground model:" << endl;
		cout << "-ht <transmit-antenna-height>" << endl;
		cout << "-hr <receive-antenna-height>" << endl;
		cout << endl << "For shadowing model:" << endl;
		cout << "-pl <path-loss-exponent>" << endl;
		cout << "-std <shadowing-deviation>" << endl;
		cout << "-d0 <reference-distance>" << endl;
		cout << "-r <receiving-rate>" << endl;
		return 0;
	}

	// parse arguments	
	double dist = atof(argv[argc-1]);
	cout << "distance = " << dist << endl;

	int argCount = (argc - 2) / 2;   // number of parameters
	argv++;
	for (int i = 0; i < argCount; i++) {
        if(!strcmp(*argv,"-m")) {          // propagation model
	    	propModel = argv + 1;
	    	cout << "propagation model: " << *propModel << endl;
	    }
        if(!strcmp(*argv,"-Pt")) {        // transmit power
            Pt = atof(*(argv + 1));
        }
        if(!strcmp(*argv,"-fr")) {        // frequency
            freq = atof(*(argv + 1));
        }
        if(!strcmp(*argv,"-Gt")) {        // transmit antenna gain
            Gt = atof(*(argv + 1));
        }
        if(!strcmp(*argv,"-Gr")) {        // receive antenna gain
            Gr = atof(*(argv + 1));
        }
        if(!strcmp(*argv,"-L")) {        // system loss
            sysLoss = atof(*(argv + 1));
	    }
        if(!strcmp(*argv,"-ht")) {        // transmit antenna height (Two ray model)
            ht = atof(*(argv + 1));
	    }
        if(!strcmp(*argv,"-hr")) {        // receive antenna height (Two ray model)
            hr = atof(*(argv + 1));
	    }
        if(!strcmp(*argv,"-pl")) {        // path loss exponent (Shadowing model)
            pathlossExp_ = atof(*(argv + 1));
        }
        if(!strcmp(*argv,"-std")) {        // shadowing deviation (Shadowing model)
            std_db_ = atof(*(argv + 1));
        }
        if(!strcmp(*argv,"-d0")) {        // close-in reference distance (Shadowing model)
            dist0_ = atof(*(argv + 1));
	    }
        if(!strcmp(*argv,"-r")) {        // rate of correct reception (Shadowing model)
            prob = atof(*(argv + 1));
        }
	    argv += 2;
	}
	
	if (propModel == NULL) {
		cout << "Must specify propagation model: -m <propagation model>" << endl;
		return 0;
	}
	
	double lambda = 3.0e8/freq;
	
	// compute threshold	
	if (!strcmp(*propModel, "FreeSpace")) {
		rxThresh_ = Friis(Pt, Gt, Gr, lambda, sysLoss, dist);
		cout << endl << "Selected parameters:" << endl;
        cout << "transmit power: " << Pt << endl;
        cout << "frequency: " << freq << endl;
        cout << "transmit antenna gain: " << Gt << endl;
        cout << "receive antenna gain: " << Gr << endl;
        cout << "system loss: " << sysLoss << endl;
	} else if (!strcmp(*propModel, "TwoRayGround")) {
		rxThresh_ = TwoRay(Pt, Gt, Gr, ht, hr, sysLoss, dist, lambda);
		cout << endl << "Selected parameters:" << endl;
        cout << "transmit power: " << Pt << endl;
        cout << "frequency: " << freq << endl;
        cout << "transmit antenna gain: " << Gt << endl;
        cout << "receive antenna gain: " << Gr << endl;
        cout << "system loss: " << sysLoss << endl;
        cout << "transmit antenna height: " << ht << endl;
        cout << "receive antenna height: " << hr << endl;
	} else if (!strcmp(*propModel, "Shadowing")) {
		// calculate receiving power at reference distance
		double Pr0 = Friis(Pt, Gt, Gr, lambda, sysLoss, dist0_);

		// calculate average power loss predicted by path loss model
		double avg_db = -10.0 * pathlossExp_ * log10(dist/dist0_);

		// calculate the the threshold
		double invq = inv_Q(prob);
		double threshdb = invq * std_db_ + avg_db;
		rxThresh_ = Pr0 * pow(10.0, threshdb/10.0);
	
#ifdef DEBUG
		cout << "Pr0 = " << Pr0 << endl;
		cout << "avg_db = " << avg_db << endl;
		cout << "invq = " << invq << endl;
		cout << "threshdb = " << threshdb << endl;
#endif
		
		cout << endl << "Selected parameters:" << endl;
        cout << "transmit power: " << Pt << endl;
        cout << "frequency: " << freq << endl;
        cout << "transmit antenna gain: " << Gt << endl;
        cout << "receive antenna gain: " << Gr << endl;
        cout << "system loss: " << sysLoss << endl;
        cout << "path loss exp.: " << pathlossExp_ << endl;
        cout << "shadowing deviation: " << std_db_ << endl;
        cout << "close-in reference distance: " << dist0_ << endl;
        cout << "receiving rate: " << prob << endl;
    } else {
    	cout << "Error: unknown propagation model." << endl;
    	cout << "Available model: FreeSpace, TwoRayGround, Shadowing" << endl;
    	return 0;
    }

	cout << endl << "Receiving threshold RXThresh_ is: " << rxThresh_ << endl;

}
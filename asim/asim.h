// Copyright (c) 2000 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.


#ifndef _RED_ROUTER_H_
#define _RED_ROUTER_H_

#include <assert.h>
#include <stdio.h>
#include <math.h>


class RedRouter {
  double MinTh, MaxTh, MaxP;
  double Lambda_L;
  double Lambda_H;

  void Populate();
 public:
  RedRouter(int mTh, int MTh, double MP) {
    MinTh = mTh;
    MaxTh = MTh;
    MaxP = MP; 
    Populate();
  }
  double ComputeProbability(double Lambda, double &Delay);
  short Identical(int mTh, int MTh, double MP) {
    return (mTh == MinTh &&
	    MTh == MaxTh &&
	    MP  == MaxP);
  }
};

void RedRouter::Populate() {
  // rho = Lambda_L: p = 0 => rho/(1-rho) = MinTh
  Lambda_L = ((double)MinTh)/((double)(1+MinTh));

  // rho = Lambda_H: p = Max_p => rho(1-Max_p)/(1-rho(1-Max_p)) = MaxTh;
  if (MaxP < 1)
    Lambda_H = ((double)MaxTh)/((double)(1+MaxTh))/(1-MaxP);
}


double RedRouter::ComputeProbability(double Lambda, double &delay) {
  double p;
  
  if (Lambda <= Lambda_L) {
    delay = Lambda/(1-Lambda);
    return 0;
  }

  if (MaxP < 1 && Lambda > Lambda_H) {
    delay = MaxTh;
    p = (Lambda - Lambda_H*(1 - MaxP))/Lambda;
    return p;
  }

  // Solve the quadratic a.p^2 + b.p + c = 0
  double a, b, c;
  a = Lambda * (MaxTh - MinTh)/(MaxP);
  b = (MaxTh - MinTh)*(1-Lambda)/MaxP + MinTh * Lambda + Lambda;
  c = MinTh*(1-Lambda)-Lambda;

  p = (-b + sqrt(b*b - 4 * a * c))/(2 * a);
  delay = Lambda*(1-p)/(1-(Lambda*(1-p)));
  return p;
}


#endif







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








// Contributed by Satish Kumar (kkumar@isi.edu)

#ifndef energy_model_h_
#define energy_model_h_

#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <iostream.h>
#include <iomanip.h>
#include <assert.h>
#include "config.h"
#include "trace.h"
#include "rng.h"


class EnergyModel : public TclObject {
public:
  EnergyModel(double energy,double l1, double l2) {
  energy_ = energy; initialenergy_ = energy; level1_ = l1 ; level2_ = l2 ;}
  inline double energy() { return energy_; }
  inline double initialenergy() { return initialenergy_; }
  inline double level1() { return level1_; }
  inline double level2() { return level2_; }
  inline void setenergy(double e) {energy_ = e;}
  virtual void DecrTxEnergy(double txtime, double P_tx) {
    energy_ -= (P_tx * txtime);
  }
  virtual void DecrRcvEnergy(double rcvtime, double P_rcv) {
    energy_ -= (P_rcv * rcvtime);
  }
//  virtual void DecrIdleEnergy(double idletime, double P_idle) { }
protected:
  double energy_;
  double level1_;
  double level2_;
  double initialenergy_;
};


#endif




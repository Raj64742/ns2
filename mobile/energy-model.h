
// Contributed by Satish Kumar (kkumar@isi.edu)

#ifndef energy_model_h_
#define energy_model_h_

#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <iostream.h>
#include <iomanip.h>
#include <assert.h>
#include <tclcl.h>
#include <trace.h>
#include <rng.h>


class EnergyModel : public TclObject {
public:
  EnergyModel(double energy) { energy_ = energy; }
  inline double energy() { return energy_; }
  virtual void DecrTxEnergy(double txtime, double P_tx) {
    energy_ -= (P_tx * txtime);
  }
  virtual void DecrRcvEnergy(double rcvtime, double P_rcv) {
    energy_ -= (P_rcv * rcvtime);
  }
//  virtual void DecrIdleEnergy(double idletime, double P_idle) { }
protected:
  double energy_;
};


#endif




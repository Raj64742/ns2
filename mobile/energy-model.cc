// Contributed by Satish Kumar <kkumar@isi.edu>


extern "C" {
#include <stdarg.h>
#include <float.h>
};

#include "energy-model.h"
#include "god.h"

static class EnergyModelClass:public TclClass
{
  public:
  EnergyModelClass ():TclClass ("EnergyModel") { }
  TclObject *create (int, const char *const *argv)
  {
    return (new EnergyModel (atof(argv[4]),atof(argv[5]),atof(argv[6])));
  }
} class_energy_model;



// Modified by Chalermek

void EnergyModel::DecrTxEnergy(double txtime, double P_tx) 
{
    double dEng = P_tx * txtime;
    if (energy_ <= dEng)
      energy_ = 0.0;
    else
      energy_ = energy_ - dEng;
    
    if (energy_ <= 0.0) {
      God::instance()->ComputeRoute();
    }
}


void EnergyModel::DecrRcvEnergy(double rcvtime, double P_rcv) 
{
     double dEng = P_rcv * rcvtime;
     if (energy_ <= dEng)
       energy_ = 0.0;
     else
       energy_ = energy_ - dEng;

     if (energy_ <= 0.0) {
       God::instance()->ComputeRoute();
     }

}


void EnergyModel::DecrIdleEnergy(double idletime, double P_idle) 
{
     double dEng = P_idle * idletime;
     if (energy_ <= dEng)
       energy_ = 0.0;
     else
       energy_ = energy_ - dEng;

     if (energy_ <= 0.0) {
       God::instance()->ComputeRoute();
     }
}





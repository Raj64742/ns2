/* 
   antenna.cc
   $Id: antenna.cc,v 1.1 1998/12/08 00:17:16 haldar Exp $
   */

#include <antenna.h>

static class AntennaClass : public TclClass {
public:
  AntennaClass() : TclClass("Antenna") {}
  TclObject* create(int, const char*const*) {
    return (new Antenna);
  }
} class_Antenna;

Antenna::Antenna()
{
  X_ = 0; Y_= 0; Z_= 0;
  bind("X_", &X_);
  bind("Y_", &Y_);
  bind("Z_", &Z_);
}


double 
Antenna::getTxGain(double /*dX*/, double /*dY*/, double /*dZ*/,
		   double /*lambda*/)
{
  return 1.0;
}

double 
Antenna::getRxGain(double /*dX*/, double /*dY*/, double /*dZ*/,
		   double /*lambda*/)
{
  return 1.0;
}

Antenna *
Antenna::copy()
{
  return this;
}

void
Antenna::release()
{
  ;
}

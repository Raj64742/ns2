// Contributed by Satish Kumar <kkumar@isi.edu>


extern "C" {
#include <stdarg.h>
#include <float.h>
};

#include "energy-model.h"


static class EnergyModelClass:public TclClass
{
  public:
  EnergyModelClass ():TclClass ("EnergyModel") { }
  TclObject *create (int, const char *const *argv)
  {
    return (new EnergyModel (atof(argv[4]),atof(argv[5]),atof(argv[6])));
  }
} class_energy_model;


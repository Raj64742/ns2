/* 
   omni-antenna.cc
   $Id: omni-antenna.cc,v 1.1 1998/12/08 00:17:17 haldar Exp $
   */

#include <omni-antenna.h>

static class OmniAntennaClass : public TclClass {
public:
  OmniAntennaClass() : TclClass("Antenna/OmniAntenna") {}
  TclObject* create(int, const char*const*) {
    return (new OmniAntenna);
  }
} class_OmniAntenna;

OmniAntenna::OmniAntenna() {
  Gt_ = 1.0;
  Gr_ = 1.0;
  bind("Gt_", &Gt_);
  bind("Gr_", &Gr_);
}



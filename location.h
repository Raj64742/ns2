#ifndef location_h_
#define location_h_

#include <assert.h>
#include "config.h"

class Location : public TclObject {

public:
  Location(double x, double y, double z) {
    X = x;
    Y = y;
    Z = z;
  }
  virtual void setLocation(double x, double y, double z) {

    X = x;
    Y = y;
    Z = z;
  }

  virtual void getLocation(double *x, double *y, double *z) {
    update_location();
    *x = X;
    *y = Y;
    *z = Z;
  }

  virtual void update_location(){}

 protected:
  double X,Y,Z;        // 3-D coordinates

};

#endif


/* Ported from CMU/Monarch's code, nov'98 -Padma.
   antenna.h
   super class for all antenna types

*/

#ifndef ns_antenna_h
#define ns_antenna_h

#include <object.h>
#include <list.h>

class Antenna;
LIST_HEAD(an_head, Antenna);

class Antenna : public TclObject {

public:
  Antenna();
  
  virtual double getTxGain(double /*dX*/, double /*dY*/, double /*dZ*/,
			   double /*lambda*/);
  // return the gain for a signal to a node at vector dX, dY, dZ
  // from the transmitter at wavelength lambda

  virtual double getRxGain(double /*dX*/, double /*dY*/, double /*dZ*/,
			   double /*lambda*/);
  // return the gain for a signal from a node at vector dX, dY, dZ
  // from the receiver at wavelength lambda
  
  virtual Antenna * copy();
  // return a pointer to a copy of this antenna that will return the 
  // same thing for get{Rx,Tx}Gain that this one would at this point
  // in time.  This is needed b/c if a pkt is sent with a directable
  // antenna, this antenna may be have been redirected by the time we
  // call getTxGain on the copy to determine if the pkt is received.

  virtual void release();
  // release a copy created by copy() above

  inline void insert(struct an_head* head) {
    LIST_INSERT_HEAD(head, this, link);
  }

  virtual inline double getX() {return X_;}
  virtual inline double getY() {return Y_;}
  virtual inline double getZ() {return Z_;}

private:
  LIST_ENTRY(Antenna) link;
  
protected:  
  double X_;			// position w.r.t. the node
  double Y_;
  double Z_;
};


#endif ns_antenna_h

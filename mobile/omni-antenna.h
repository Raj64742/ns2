/* Ported from CMU/Monarch's code, nov'98 -Padma.
   omni-antenna.h

   omni-directional antenna

*/

#ifndef ns_omniantenna_h
#define ns_omniantenna_h

#include <antenna.h>

class OmniAntenna : public Antenna {

public:
  OmniAntenna();
  
  virtual double getTxGain(double, double, double, double)
  // return the gain for a signal to a node at vector dX, dY, dZ
  // from the transmitter at wavelength lambda
    {
      return Gt_;
    }
  virtual double getRxGain(double, double, double, double)
  // return the gain for a signal from a node at vector dX, dY, dZ
  // from the receiver at wavelength lambda
    {
      return Gr_;
    }
  
  virtual Antenna * copy()
  // return a pointer to a copy of this antenna that will return the 
  // same thing for get{Rx,Tx}Gain that this one would at this point
  // in time.  This is needed b/c if a pkt is sent with a directable
  // antenna, this antenna may be have been redirected by the time we
  // call getTxGain on the copy to determine if the pkt is received.
    {
      // since the Gt and Gr are constant w.r.t time, we can just return
      // this object itself
      return this;
    }

  virtual void release()
  // release a copy created by copy() above
    {
      // don't do anything
    }
 
protected:
  double Gt_;			// gain of transmitter (db)
  double Gr_;			// gain of receiver (db)
};


#endif ns_omniantenna_h



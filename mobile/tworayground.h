/* -*- c++ -*- 
   tworayground.h


*/
#ifndef __tworayground_h__
#define __tworayground_h__

#include <packet-stamp.h>
#include <wireless-phy.h>
#include <propagation.h>

class TwoRayGround : public Propagation {
public:
  TwoRayGround();
  virtual double Pr(PacketStamp *tx, PacketStamp *rx, WirelessPhy *ifp)
    {return Pr(tx, rx, ifp->getL(), ifp->getLambda());}

protected:
  virtual double Pr(PacketStamp *tx, PacketStamp *rx, double L, double lambda);

  double last_hr, last_ht;
  double crossover_dist;
};


#endif /* __tworayground_h__ */

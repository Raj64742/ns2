/* Ported from CMU/Monarch's code, nov'98 -Padma.
   propagation.h

   superclass of all propagation models
*/

#ifndef ns_propagation_h
#define ns_propagation_h

#define SPEED_OF_LIGHT	300000000		// 3 * 10^8 m/s
#define PI		3.1415926535897


#include <topography.h>
#include <phy.h>
#include <wireless-phy.h>
#include <packet-stamp.h>

class PacketStamp;
class WirelessPhy;
/*======================================================================
   Progpagation Models

	Using postion and wireless transmission interface properties,
	propagation models compute the power with which a given
	packet will be received.

	Not all propagation models will be implemented for all interface
	types, since a propagation model may only be appropriate for
	certain types of interfaces.

   ====================================================================== */

class Propagation : public TclObject {

public:
  Propagation() : name(NULL), topo(NULL) {}

  // calculate the Pr by which the receiver will get a packet sent by
  // the node that applied the tx PacketStamp for a given inteface 
  // type
  virtual double Pr(PacketStamp *tx, PacketStamp *rx, Phy *);
  virtual double Pr(PacketStamp *tx, PacketStamp *rx, WirelessPhy *);

  virtual int command(int argc, const char*const* argv);
  
protected:
  char *name;
  Topography *topo;
};

#endif /* __propagation_h__ */


/* Ported from CMU/Monarch's code, nov'98 -Padma.

   wireless-phy.h
   -- a SharedMedia network interface

*/

#ifndef ns_WirelessPhy_h
#define ns_WirelessPhy_h


#include <propagation.h>
#include <modulation.h>
#include <omni-antenna.h>
#include <phy.h>
#include <mobilenode.h>
class Phy;
class Propagation;

class WirelessPhy : public Phy {
 public:
	WirelessPhy();
	
	void sendDown(Packet *p);
	int sendUp(Packet *p);
	
	inline double getL() {return L_;}
	
	inline double getLambda() {return lambda_;}
  
	virtual int command(int argc, const char*const* argv);
	virtual void dump(void) const;
	MobileNode* node(void) const { return node_; }
	
	void setnode (MobileNode *node) { node_ = node; }
	
 protected:
  double Pt_;			// transmission power (W)
  double freq_;                  // frequency
  double lambda_;		// wavelength (m)
  double L_;			// system loss factor
  
  double RXThresh_;		// receive power threshold (W)
  double CSThresh_;		// carrier sense threshold (W)
  double CPThresh_;		// capture threshold (db)
  
  Antenna *ant_;
  Propagation *propagation_;	// Propagation Model
  Modulation *modulation_;	// Modulation Scheme
  MobileNode* node_;
  
 private:
  
  inline int	initialized() {
	  return (node_ && uptarget_ && downtarget_ && propagation_);
  }
  

};

#endif ns_WirelessPhy_h

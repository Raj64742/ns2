/*Ported from CMU/Monarch's code, nov'98 -Padma.
  channel.h

  */


#ifndef ns_new_channel_h__
#define ns_new_channel_h__

class newChannel;
class WirelessChannel;
class Phy;

#include <packet.h>
#include <phy.h>

/* ======================================================================
   Channel

	This class is used to represent the physical media to which
	network interfaces are attached.  As such, the sendUp() function
	simply schedules packet reception at the interfaces.  The recv()
	function should never be called.
   ====================================================================== */

class newChannel : public TclObject {
 public:
	newChannel(void);
	virtual int command(int argc, const char*const* argv);

	void recv(Packet* p, Handler *h);
	struct if_head	ifhead_;
	TclObject* gridkeeper_;
 private:
	virtual void sendUp(Packet* p, Phy *txif);
	void dump(void);
	
	int index_;
	double delay_;           // propagation delay for lan
	//struct if_head ifhead_;	
	//TclObject* gridkeeper_;
};



/*====================================================================
  WirelessChannel

  This class is used to represent the physical media to which
  
====================================================================*/

class WirelessChannel : public newChannel{

 public:
	WirelessChannel(void);

 private:
	void sendUp(Packet *p, Phy *txif);

};


#endif __channel_h__





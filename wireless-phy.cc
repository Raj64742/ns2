/* Ported from CMU/Monarch's code, nov'98 -Padma.
   wireless-phy.cc
   */

#include <math.h>

#include <packet.h>

#include <mobilenode.h>
#include <phy.h>
#include <propagation.h>
#include <modulation.h>
#include <omni-antenna.h>
#include <wireless-phy.h>

/* ======================================================================
   WirelessPhy Interface
   ====================================================================== */
static class WirelessPhyClass: public TclClass {
public:
        WirelessPhyClass() : TclClass("Phy/WirelessPhy") {}
        TclObject* create(int, const char*const*) {
                return (new WirelessPhy);
        }
} class_WirelessPhy;


WirelessPhy::WirelessPhy(void) : Phy()
{
	propagation_ = 0;
	node_ = 0;
	modulation_ = 0;
	bandwidth_ = 2*1e6;                  // 2 Mb
	Pt_ = pow(10, 2.45) * 1e-3;   // 24.5 dbm, ~ 281.8mw
	lambda_ = SPEED_OF_LIGHT / (914 * 1e6);  // 914 mHz
	L_ = 1.0;
	freq_ = -1.0;

  /*
   *  It sounds like 10db should be the capture threshold.
   *
   *  If a node is presently receiving a packet a a power level
   *  Pa, and a packet at power level Pb arrives, the following
   *  comparion must be made to determine whether or not capture
   *  occurs:
   *
   *    10 * log(Pa) - 10 * log(Pb) > 10db
   *
   *  OR equivalently
   *
   *    Pa/Pb > 10.
   *
   */
	
	CPThresh_ = 10.0;
	CSThresh_ = 1.559e-11;
	RXThresh_ = 3.652e-10;

	//bind("CPThresh_", &CPThresh_);
	//bind("CSThresh_", &CSThresh_);
	//bind("RXThresh_", &RXThresh_);
	//bind("bandwidth_", &bandwidth_);
	//bind("Pt_", &Pt_);
	//bind("freq_", &freq_);
	//bind("L_", &L_);

  if (freq_ != -1.0) 
    { // freq was set by tcl code
      lambda_ = SPEED_OF_LIGHT / freq_;
    }
}

int
WirelessPhy::command(int argc, const char*const* argv)
{
  TclObject *obj;    
  if (argc == 2) {
	  Tcl& tcl = Tcl::instance();
	  if(strcmp(argv[1], "node") == 0) {
		  tcl.resultf("%s", node_->name());
		  return TCL_OK;
	  }
  }
  
  else if(argc == 3) {
          if( (obj = TclObject::lookup(argv[2])) == 0) {
		  fprintf(stderr, "WirelessPhy: %s lookup of %s failed\n", 
			  argv[1], argv[2]);
		  return TCL_ERROR;
	  }
	  if (strcmp(argv[1], "propagation") == 0) {
		  assert(propagation_ == 0);
		  propagation_ = (Propagation*) obj;
		  return TCL_OK;
	  }
	  else if (strcmp(argv[1], "node") == 0) {
		  assert(node_ == 0);
		  node_ = (MobileNode*) obj;
		  // LIST_INSERT_HEAD() is done by Node
		  return TCL_OK;
	  }
	  else if (strcasecmp(argv[1], "antenna") == 0) {
		  ant_ = (Antenna*) obj;
		  return TCL_OK;
	  }
  }
  return Phy::command(argc,argv);
}
 
void 
WirelessPhy::sendDown(Packet *p)
{
  /*
   * Sanity Check
   */
	assert(initialized());
	
  /*
   *  Stamp the packet with the interface arguments
   */
  p->txinfo.stamp(node_, ant_->copy(), Pt_, lambda_);
  
  // Send the packet
  channel_->recv(p, this);
}

int 
WirelessPhy::sendUp(Packet *p)
{

  /*
   * Sanity Check
   */
	assert(initialized());

  PacketStamp s;
  double Pr;
  int pkt_recvd = 0;

  if(propagation_) {

    s.stamp(node_, ant_, 0, lambda_);
    
    Pr = propagation_->Pr(&p->txinfo, &s, this);
    
    if (Pr < CSThresh_) {
      pkt_recvd = 0;
      goto DONE;
    }

    if (Pr < RXThresh_) {
      /*
       * We can detect, but not successfully receive
       * this packet.
       */
      hdr_cmn *hdr = HDR_CMN(p);
      hdr->error() = 1;
#if DEBUG > 3
      printf("SM %f.9 _%d_ drop pkt from %d low POWER %e/%e\n",
	     Scheduler::instance().clock(), node_->index(),
	     p->txinfo.getNode()->index(),
	     Pr,RXThresh);
#endif
    }
  }
  

  if(modulation_) {
    hdr_cmn *hdr = HDR_CMN(p);
    hdr->error() = modulation_->BitError(Pr);
  }
  
  /*
   * The MAC layer must be notified of the packet reception
   * now - ie; when the first bit has been detected - so that
   * it can properly do Collision Avoidance / Detection.
   */
  pkt_recvd = 1;

DONE:
  p->txinfo.getAntenna()->release();
  //*RxPr = Pr;

  /* WILD HACK: The following two variables are a wild hack.
     They will go away in the next release...
     They're used by the mac-802_11 object to determine
     capture.  This will be moved into the net-if family of 
     objects in the future. */
  p->txinfo.RxPr = Pr;
  p->txinfo.CPThresh = CPThresh_;

  return pkt_recvd;
}

void
WirelessPhy::dump(void) const
{
	Phy::dump();
	fprintf(stdout,
		"\tPt: %f, Gt: %f, Gr: %f, lambda: %f, L: %f\n",
		Pt_, ant_->getTxGain(0,0,0,lambda_), ant_->getRxGain(0,0,0,lambda_), lambda_, L_);
	fprintf(stdout, "\tbandwidth: %f\n", bandwidth_);
	fprintf(stdout, "--------------------------------------------------\n");
}




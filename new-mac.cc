/* Ported from CMU/Monarch's code, nov'98 -Padma.

 */


#include <delay.h>
#include <connector.h>
#include <packet.h>
#include <random.h>

#include <god.h>
//#include <debug.h>
#include <arp.h>
#include <new-ll.h> 
#include <new-mac.h>




/* ======================================================================
   Mac Packet Header
   ====================================================================== */
int newhdr_mac::offset_;

static class newMacHeaderClass : public PacketHeaderClass {
public:
	newMacHeaderClass() :
		PacketHeaderClass("PacketHeader/newMac", MAC_HDR_LEN) {
		bind_offset(&newhdr_mac::offset_);
	}
} class_machdr;

/*static class newMacClass : public TclClass {
  public:
  newMacClass() : TclClass("newMac") {}
  TclObject* create(int, const char*const*) {
  return (new newMac);
  }
  } class_mac;*/

void
newMacHandlerResume::handle(Event*)
{
	mac_->resume();
}


void
newMacHandlerSend::handle(Event* e)
{
	mac_->sendUp((Packet*)e);
}

/* ======================================================================
   Mac Class Functions
   ====================================================================== */
static int MacIndex = 1;

newMac::newMac() : BiConnector(), hRes_(this), hSend_(this)
{
        index_ = MacIndex++;
	//bitRate_ = 2.0 * 1e6;         // XXX 2 Mbps
	bind_bw("bandwidth_", &bandwidth_);
	bind_time("delay_", &delay_);
	bind("off_newmac_", &off_newmac_);
	
        netif_ = 0;
	ll_ = 0;
        upcall_ = 0;

        state_ = newMAC_IDLE;
        pktTx_ = 0;
        pktRx_ = 0;
	tap_ = 0;
	
}

int
newMac::command(int argc, const char*const* argv)
{
	if(argc == 2) {
		Tcl& tcl = Tcl::instance();

		if(strcasecmp(argv[1], "id") == 0) {
			tcl.resultf("%d", addr());
			return TCL_OK;
		}
	}
	if (argc == 3) {
		TclObject *obj;
		
		if( (obj = TclObject::lookup(argv[2])) == 0) {
			fprintf(stderr, "%s lookup failed\n", argv[1]);
			return TCL_ERROR;
		}
		
		if (strcmp(argv[1], "netif") == 0) {
			netif_ = (Phy*) obj;
			return TCL_OK;
		}
		else if (strcmp(argv[1], "up-target") == 0) {
			uptarget_ = (NsObject*) obj;
			return TCL_OK;
		}
		else if (strcmp(argv[1], "down-target") == 0) {
			downtarget_ = (NsObject*) obj;
			return TCL_OK;
		}
		
	}
	return BiConnector::command(argc, argv);
}

void newMac::recv(Packet* p, Handler* h)
{
	// if h is NULL, packet comes from the lower layer, ie. MAC classifier
	//if (h == 0) {
	hdr_cmn *ch = HDR_CMN(p);
	int d = ch->direction();
	
	if (d == 1) {
		sendUp(p);
		return;
	} else if (d == -1) {
		
		upcall_ = h;
		//newhdr_mac* mh = newhdr_mac::access(p);
		newhdr_mac* mh = (newhdr_mac*)p->access(off_newmac_);
		mh->set(MF_DATA, index_);
		state(newMAC_SEND);
		sendDown(p);
	} else {
		
		printf("Direction of pkt-flow not specifed; Sending pkt up on default.\n\n");
		sendUp(p);
	}
}


void newMac::sendUp(Packet* p) 
{
	state(newMAC_IDLE);
	Scheduler::instance().schedule(uptarget_, p, delay_);
}

void newMac::sendDown(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	double txt = netif_->txtime(p);
	downtarget_->recv(p, this);
	s.schedule(&hRes_, &intr_, txt);
}


void newMac::resume(Packet* p)
{
	if (p != 0)
		drop(p);
	state(newMAC_IDLE);
	upcall_->handle(&intr_);
}


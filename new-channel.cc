/*Ported from CMU/Monarch's code, nov'98 -Padma.
  channel.cc

  */

#include <float.h>

#include <delay.h>
#include <object.h>
#include <packet.h>
#include <trace.h>

//#include <debug.h>
#include <list.h>
#include <phy.h>
#include <wireless-phy.h>
#include <mobilenode.h>
#include <new-channel.h>

static class newChannelClass : public TclClass {
public:
        newChannelClass() : TclClass("newChannel") {}
        TclObject* create(int, const char*const*) {
                return (new newChannel);
        }
} class_channel;


static class WirelessChannelClass : public TclClass {
public:
        WirelessChannelClass() : TclClass("newChannel/WirelessChannel") {}
        TclObject* create(int, const char*const*) {
                return (new WirelessChannel);
        }
} class_Wireless_channel;

/* ======================================================================
   NS Initialization Functions
   ====================================================================== */
int
newChannel::command(int argc, const char*const* argv)
{

	if(argc == 2) {
		Tcl& tcl = Tcl::instance();

		if(strncasecmp(argv[1], "id", 2) == 0) {
			tcl.resultf("%d", index_);
			return TCL_OK;
		}
	}
	else if(argc == 3) {
		TclObject *obj;

		if( (obj = TclObject::lookup(argv[2])) == 0) {
			fprintf(stderr, "%s lookup failed\n", argv[1]);
			return TCL_ERROR;
		}

		if(strncasecmp(argv[1], "addif", 5) == 0) {
			((Phy*) obj)->insertchnl(&ifhead_);
			((Phy*) obj)->setchnl(this);
			return TCL_OK;
		}
		// add interface for grid_keeper_
		/*else if(strncasecmp(argv[1], "grid_keeper", 5) == 0) {
			grid_keeper_ = (GridKeeper*)obj;
			return TCL_OK;
			}*/
	}
	return TclObject::command(argc, argv);
}


/* ======================================================================
   Other class functions
   ====================================================================== */
static int newChannelIndex = 0;

newChannel::newChannel()
{
	index_ = newChannelIndex++;
	LIST_INIT(&ifhead_);
	bind_time("delay_", &delay_);
}


void
newChannel::recv(Packet *p, Handler *h)
{
	sendUp(p, (Phy*)h);
}


void
newChannel::sendUp(Packet* p, Phy *tifp)
{
	Scheduler	&s = Scheduler::instance();
	Phy		*rifp = ifhead_.lh_first;
	Packet		*newp;

	// Reverse direction in pkt
	struct hdr_cmn *hdr = HDR_CMN(p);
	hdr->direction() = 1;
	for( ; rifp; rifp = rifp->nextchnl()) {
		// every interface connected to the channel receives a pkt. what if multiple interfaces connected to the same channel?
		if(rifp == tifp)
			continue;
		/*
		 * Each node needs to get their own copy of this packet.
		 * Since collisions occur at the receiver, we can have
		 * two nodes canceling and freeing the *same* simulation
		 * event.
		 *
		 */
		newp = p->copy();
		
		/*
		 * Each node on the channel receives a copy of the
		 * packet.  The propagation delay determines exactly
		 * when the receiver's interface detects the first
		 * bit of this packet.
		 */
		
		s.schedule(rifp, newp, delay_);
	}
}

void
newChannel::dump(void)
{
	Phy *n;
	
	fprintf(stdout, "Network Interface List\n");
 	for(n = ifhead_.lh_first; n; n = n->nextchnl() )
		n->dump();
	fprintf(stdout, "--------------------------------------------------\n");
}

// Wireless extensions
class MobileNode;

WirelessChannel::WirelessChannel(void) : newChannel() {}
	

void
WirelessChannel::sendUp(Packet* p, Phy *tifp)
{
	Scheduler	&s = Scheduler::instance();
	MobileNode	*tnode = (MobileNode*)((WirelessPhy*)tifp)->node();
	Phy		*rifp = ifhead_.lh_first;
	MobileNode	*rnode = 0;
	double		propdelay;
	Packet		*newp;
	
	
	// reverse direction in pkt
	struct hdr_cmn *hdr = HDR_CMN(p);
	hdr->direction() = 1;
	if(gridkeeper_)
	{
		// get a subset of neighbouring interfaces and hand
		// a pkt to each
	}
	
	for( ; rifp; rifp = rifp->nextchnl()) {
		rnode = (MobileNode*)((WirelessPhy*)rifp)->node();

		if(rnode == tnode)
			continue;
		/*
		 * Each node needs to get their own copy of this packet.
		 * Since collisions occur at the receiver, we can have
		 * two nodes canceling and freeing the *same* simulation
		 * event.
		 *
		 */
		newp = p->copy();
		/*
		 * Each node on the channel receives a copy of the
		 * packet.  The propagation delay determines exactly
		 * when the receiver's interface detects the first
		 * bit of this packet.
		 */
		propdelay = tnode->propdelay(rnode);

		assert(propdelay >= 0.0);
		if (propdelay == 0.0) {
		  /* if the propdelay is 0 b/c two nodes are on top of 
		     each other, move them slightly apart -dam 7/28/98 */
		  propdelay = 2 * DBL_EPSILON;
		  printf ("propdelay 0: %d->%d at %f\n",
			  tnode->index(), rnode->index(), s.clock());
		}
		s.schedule(rifp, newp, propdelay);
	}
}




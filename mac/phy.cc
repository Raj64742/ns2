/* Ported from CMU/Monarch's code, nov'98 -Padma.
 * phy.cc
 */

#include <math.h>
#include <string.h>
#include <packet.h>
#include <phy.h>

class Mac;

static int InterfaceIndex = 0;


Phy::Phy() {
	index_ = InterfaceIndex++;
	bandwidth_ = 0.0;
	channel_ = 0;
}

int
Phy::command(int argc, const char*const* argv) {
	if (argc == 2) {
		Tcl& tcl = Tcl::instance();

		if(strcmp(argv[1], "id") == 0) {
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
		if (strcmp(argv[1], "channel") == 0) {
                        assert(channel_ == 0);
			channel_ = (newChannel*) obj;
			// LIST_INSERT_HEAD() is done by Channel
			return TCL_OK;
		}
		/*else if (strncasecmp(argv[1], "up-target", 9) == 0) {
			assert(uptarget_ == 0);
			uptarget_ = (NsObject*) obj;
			return TCL_OK;
			}*/
	} 
	return BiConnector::command(argc, argv);
}



void
Phy::recv(Packet* p, Handler* h)
{
	struct hdr_cmn *hdr = HDR_CMN(p);	
	/*
	 * Handle outgoing packets
	 */
	int d = hdr->direction();
	if (d == -1) {
		/*
		 * The MAC schedules its own EOT event so we just
		 * ignore the handler here.  It's only purpose
		 * it distinguishing between incoming and outgoing
		 * packets.
		 */
		sendDown(p);
		return;
	} else if (d == 1) {
		if (sendUp(p) == 0) {
			/*
			 * XXX - This packet, even though not detected,
			 * contributes to the Noise floor and hence
			 * may affect the reception of other packets.
			 */
			Packet::free(p);
			return;
		} else {
			uptarget_->recv(p, (Handler*) 0);
		}
	} else {
		printf("Direction for pkt-flow not specified; Sending pkt up the stack on default.\n\n");
		if (sendUp(p) == 0) {
			/*
			 * XXX - This packet, even though not detected,
			 * contributes to the Noise floor and hence
			 * may affect the reception of other packets.
			 */
			Packet::free(p);
			return;
		} else {
			uptarget_->recv(p, (Handler*) 0);
		}
	}
	
}

/* NOTE: this might not be the best way to structure the relation
between the actual interfaces subclassed from net-if(phy) and 
net-if(phy). 
It's fine for now, but if we were to decide to have the interfaces
themselves properly handle multiple incoming packets (they currently
require assistance from the mac layer to do this), then it's not as
generic as I'd like.  The way it is now, each interface will have to
have it's own logic to keep track of the packets that are arriving.
Seems like this is general service that net-if could provide.

Ok.  A fair amount of restructuring is going to have to happen here
when/if net-if keep track of the noise floor at their location.  I'm
gonna punt on it for now.

Actually, this may be all wrong.  Perhaps we should keep a separate 
noise floor per antenna, which would mean the particular interface types
would have to track noise floor themselves, since only they know what
kind of antenna diversity they have.  -dam 8/7/98 */


// double
// Phy::txtime(Packet *p) const
// {
// 	hdr_cmn *hdr = HDR_CMN(p);
// 	return hdr->size() * 8.0 / Rb_;
// }


void
Phy::dump(void) const
{
	fprintf(stdout, "\tINDEX: %d\n",
		index_);
	fprintf(stdout, "\tuptarget: %x, channel: %x",
		(u_int32_t) uptarget_, (u_int32_t) channel_);

}



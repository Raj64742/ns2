/* 
   mac-802_3.cc
   $Id: mac-802_3.cc,v 1.2 1999/03/13 03:52:52 haoboy Exp $
   */
#include <delay.h>
#include <connector.h>
#include <packet.h>
#include <random.h>

// #define DEBUG
//#include <debug.h>
#include <arp.h>
#include <ll.h> 
#include <mac.h>
#include <mac-802_3.h>


/* ======================================================================
   TCL hooks for the 802.3 class.
   ====================================================================== */
static class Mac802_3Class : public TclClass {
public:
	Mac802_3Class() : TclClass("Mac/802_3") {}
	TclObject* create(int, const char*const*) {
		return (new Mac802_3);
	}
} class_mac802_3;


/* ======================================================================
   Defer Handler Functions
	- used to resume the upper layer
   ====================================================================== */
void
MacHandlerDefer::handle(Event *)
{
	busy_ = 0;
	callback->handle((Event*) 0);
}


void
MacHandlerDefer::schedule(Handler *h, double t)
{
	Scheduler& s = Scheduler::instance();
	assert(h && busy_ == 0);
	// intr.uid_ = 0;
	s.schedule(this, &intr, t);
	busy_ = 1;
	callback = h;
}


/* ======================================================================
   Send Handler Functions
	- called when a transmission completes without any collisions
   ====================================================================== */
void
Mac8023HandlerSend::handle(Event*)
{
	busy_ = 0;
	/*
	 *  Transmission completed successfully
	 */
	assert(mac->pktTx_);
	//Packet::free(mac->pktTx_);
	mac->pktTx_ = 0;
	mac->pktTxcnt = 0;
	mac->resume();
}

void
Mac8023HandlerSend::schedule(double t)
{
	Scheduler& s = Scheduler::instance();
	assert(busy_ == 0);
	//intr.uid_ = 0;
	s.schedule(this, &intr, t);
	busy_ = 1;
}


/* ======================================================================
   Backoff Handler Functions
   ====================================================================== */
void
MacHandlerBack::handle(Event* e)
{
	busy_ = 0;
	mac->send((Packet*) e);
}

void
MacHandlerBack::schedule(Packet *p, double t) {
	Scheduler& s = Scheduler::instance();
	assert(p && busy_ == 0);
	s.schedule(this, p, t);
	busy_ = 1;
}


/* ======================================================================
   Receive Handler Functions
   ====================================================================== */
void
MacHandlerRecv::handle(Event* e)
{
	busy_ = 0;

	/*
	 * Reception Successful
	 */
	mac->pktRx_ = 0;
	mac->recv_complete((Packet*) e);
}

void
MacHandlerRecv::schedule(Packet *p, double t) {
	Scheduler& s = Scheduler::instance();
	assert(p && busy_ == 0);
	s.schedule(this, p, t);
	busy_ = 1;
	p_ = p;
}


/* ======================================================================
   Packet Headers Routines
inline int
Mac802_3::hdr_dst(char* hdr, u_int32_t dst)
{
	struct hdr_mac802_3 *mh = (struct hdr_mac802_3*) hdr;

	if(dst)
		//ETHER_ADDR(mh->mh_da) = dst;
		STORE4BYTE(&dst, (mh->mh_da));

	return ETHER_ADDR(mh->mh_da);
}

inline int 
Mac802_3::hdr_src(char* hdr, u_int32_t src)
{
	struct hdr_mac802_3 *mh = (struct hdr_mac802_3*) hdr;

	if(src)
		//ETHER_ADDR(mh->mh_sa) = src;
		STORE4BYTE(&src, (mh->mh_da));

	return ETHER_ADDR(mh->mh_sa);
}

inline int 
Mac802_3::hdr_type(char* hdr, u_int16_t type)
{
	struct hdr_mac802_3 *mh = (struct hdr_mac802_3*) hdr;

	if(type)
		mh->mh_type = type;

	return  mh->mh_type;
}
   ====================================================================== */

/* ======================================================================
   Mac Class Functions
   ====================================================================== */

Mac802_3::Mac802_3() : Mac(), mhBack(this),mhDefer(this), mhRecv(this), mhSend(this)
{
        pktTxcnt = 0;
}




int
Mac802_3::command(int argc, const char*const* argv)
{
	return Mac::command(argc, argv);
}




void Mac802_3::discard(Packet *p, const char*)
{
	hdr_mac802_3* mh = (hdr_mac802_3*)p->access(off_mac_);

	if((u_int32_t)ETHER_ADDR(mh->mh_da) == (u_int32_t)index_)
		drop(p);
	else
		Packet::free(p);
}


void
Mac802_3::collision(Packet *p)
{

	switch(state_) {

	case MAC_SEND:
		assert(pktTx_);

		/*
		 *  Collisions occur at the receiver, so receiving a
		 *  packet, while transmitting may not be a problem.
		 *
		 *  If the packet is for us, then drop() the packet -
		 *  ie; log an event recording the collision.
		 *  Otherwise, just free the packet and forget about
		 *  it.
		 */
		discard(p);
		break;

	case MAC_RECV:
		assert(pktRx_);

		/*
		 * Drop the packet that is presently being received.
		 * Again, if the packet was not for us, then its not
		 * really worth logging.
		 */
		mhRecv.cancel();
		discard(pktRx_);
		pktRx_ = 0;

		/*
		 *  If the packet is for us, then drop() the packet -
		 *  ie; log an event recording the collision.
		 *  Otherwise, just free the packet and forget about
		 *  it.
		 */
		discard(p);
		resume();		// restart the MAC

		break;
	default:
	  assert("SHOULD NEVER HAPPEN" == 0);
	}
}


void
Mac802_3::send(Packet *p, Handler *h)
{
	hdr_cmn *hdr = HDR_CMN(p);
	hdr->size() += ETHER_HDR_LEN;

	callback_ = h;
	send(p);
}

/*
 * This function gets called when packets are passed down from the
 * link layer.
 */
void
Mac802_3::recv(Packet *p, Handler *h)
{

	/*
	 * Sanity Check
	 */
	assert(initialized());

	/*
	 *  Handle outgoing packets.
	 */
	if(h) {
		send(p, h);
		return;
	}

	/*
	 *  Handle incoming packets.
	 *
	 *  We just received the 1st bit of a packet on the network
	 *  interface.
	 *
	 */
	if(state_ == MAC_IDLE) {
		state_ = MAC_RECV;
		pktRx_ = p;

		/*
		 * Schedule the reception of this packet, in
		 * txtime seconds.
		 */
		mhRecv.schedule(p, netif_->txtime(p));

#if 0
		/*
		 *  If we are in the first 2/3 of an IFS, then go
		 *  ahead and cancel the timer.
		 */
		if(mhDefer.busy()) {
			double now = Scheduler::instance().clock();
			if(mhDefer.expire() - now > (IEEE_8023_IFS/3) ) {
				mhDefer.cancel();
			}
			mhDefer.cancel();
		}
#endif
	}
	else {
		collision(p);
	}
}


void
Mac802_3::send(Packet *p)
{
	double txtime = netif_->txtime(p);


	/*
	 *  Sanity Check
	 */
	if(pktTx_) {
		drop(pktTx_);
		drop(p);
		fprintf(stderr, "index: %d\n", index_);
		fprintf(stderr, "Backoff Timer: %d\n", mhBack.busy());
		fprintf(stderr, "Defer Timer: %d\n", mhDefer.busy());
		fprintf(stderr, "Recv Timer: %d\n", mhRecv.busy());
		fprintf(stderr, "Send Timer: %d\n", mhSend.busy());
		exit(1);
	}
	assert(callback_ && pktTx_ == 0);

	pktTx_ = p;

	/*
	 *  Perform carrier sense.  If the medium is not IDLE, then
	 *  backoff again.  Otherwise, go for it!
	 */
	if(state_ != MAC_IDLE) {
		backoff();
		return;
	}

	state_ = MAC_SEND;

	/*
	 * pass the packet on the "interface" which will in turn
	 * place the packet on the channel.
	 *
	 * NOTE: a handler is passed along so that the Network Interface
	 *	 can distinguish between incoming and outgoing packets.
	 */
	downtarget_->recv(p, this);

	/*
	 * This MAC will remain in the MAC_SEND state until packet
	 * transmission completes.  Schedule an event that will
	 * reset the MAC to the IDLE state.
	 */
	mhSend.schedule(txtime);
}


void
Mac802_3::recv_complete(Packet *p)
{
	hdr_cmn *hdr = HDR_CMN(p);

	/*
	 * Address Filtering
	 */

//	hdr_mac802_3* mh = (hdr_mac802_3*)p->access(off_mac_);
//	if((u_int32_t)ETHER_ADDR(mh->mh_da) != (u_int32_t) index_ &&
//	   (u_int32_t)ETHER_ADDR(mh->mh_da) != MAC_BROADCAST) {
	char* mha = (char*)p->access(hdr_mac::offset_);
	int dst = this->hdr_dst(mha);

	if (((u_int32_t)dst != MAC_BROADCAST) && (dst != index_)) {
		/*
		 *  We don't want to log this event, so we just free
		 *  the packet instead of calling the drop routine.
		 */
		Packet::free(p);
		goto done;
	}

	/*
	 * Now, check to see if this packet was received with enough
	 * bit errors that the current level of FEC still could not
	 * fix all of the problems - ie; after FEC, the checksum still
	 * failed.
	 */
	if( hdr->error() ) {
		discard(p);
		goto done;
	}

	/*
	 * Adjust the MAC packet size - ie; strip off the mac header
	 */
	hdr->size() -= ETHER_HDR_LEN;

	/*
	 *  Pass the packet up to the link-layer.
	 *  XXX - we could schedule an event to account
	 *  for this processing delay.
	 */
	uptarget_->recv(p, (Handler*) 0);

	done:
	/*
	 * reset the MAC to the IDLE state
	 */
	resume();
}


void
Mac802_3::backoff() {
	int k, r;

	pktTxcnt++;
	k = min(pktTxcnt, IEEE_8023_BLIMIT);
	r = Random::integer(1 << k);

	if(pktTxcnt < IEEE_8023_ALIMIT) {
		mhBack.schedule(pktTx_, r * IEEE_8023_SLOT);
		pktTx_ = 0;
		return;
	}
	else {
		drop(pktTx_);
		pktTx_ = 0;
		pktTxcnt = 0;
	}
}


void
Mac802_3::resume()
{
	state_ = MAC_IDLE;

	/*
	 *  Sanity Check
	 */
	assert(pktRx_ == 0);
	assert(pktTx_ == 0);
	// assert(mhDefer.busy() == 0);
	assert(mhRecv.busy() == 0);
	assert(mhSend.busy() == 0);

	/*
	 *  If we're not backing off right now, go ahead and unblock
	 *  the upper layer so that we can get more packets to send.
	 */
	if(callback_ && mhBack.busy() == 0) {

		mhDefer.schedule(callback_, IEEE_8023_IFS);
		callback_ = 0;
	}
}



/* 
   mac-802_3.cc
   $Id: mac-802_3.cc,v 1.6 1999/10/28 19:45:35 yuriy Exp $
   */
#include <packet.h>
#include <random.h>
#include <arp.h>
#include <ll.h> 
#include <mac-802_3.h>

static class Mac802_3Class : public TclClass {
public:
	Mac802_3Class() : TclClass("Mac/802_3") {}
	TclObject* create(int, const char*const*) {
		return (new Mac802_3);
	}
} class_mac802_3;

void Mac8023HandlerSend::handle(Event*) {
	assert(mac->pktTx_);
	/* Transmission completed successfully */
	busy_ = 0;
	mac->pktTx_ = 0;
	mac->pktTxcnt = 0;
	Packet::free(mac->mhRetx_.packet()); // free buffered copy
	mac->mhRetx_.reset();
	mac->mhIFS_.schedule(IEEE_8023_IFS);
}

void Mac8023HandlerSend::schedule(double t) {
	Scheduler& s = Scheduler::instance();
	assert(busy_ == 0);
	s.schedule(this, &intr, t);
	busy_ = 1;
}

void MacHandlerRecv::handle(Event* e) {
	/* Reception Successful */
	busy_ = 0;
	mac->pktRx_ = 0;
	mac->recv_complete((Packet*) e);
}

void MacHandlerRecv::schedule(Packet *p, double t) {
	Scheduler& s = Scheduler::instance();
	assert(p && busy_ == 0);
	s.schedule(this, p, t);
	busy_ = 1;
	p_ = p;
}

bool MacHandlerRetx::schedule() {
	Scheduler& s = Scheduler::instance();
	assert(p_ && !busy_);
	int k, r;
	if(try_ < IEEE_8023_ALIMIT) {
		k = min(try_, IEEE_8023_BLIMIT);
		r = Random::integer(1 << k);
		s.schedule(this, &intr, r*IEEE_8023_SLOT);
		busy_ = 1;
		return true;
	}
	reset();
	return false;
}

void MacHandlerRetx::handle(Event *) {
	assert(p_ && busy_);
	busy_= 0;
	++try_;
	mac->send(p_);
}

inline void MacHandlerIFS::handle(Event*) { 
	busy_= 0; 
	mac->resume(); 
}


Mac802_3::Mac802_3() : Mac(), 
	mhRecv_(this), mhRetx_(this), mhIFS_(this), mhSend_(this) {
        pktTxcnt = 0;
}

void Mac802_3::recv(Packet *p, Handler *h) {
	assert(initialized());

	/* Handle outgoing packets */
	if (HDR_CMN(p)->direction() == hdr_cmn::DOWN) {
		assert(h);
		send(p, h);
		return;
	}
	/* Handle incoming packets : just received the 1st bit of a packet */
	if(state_ == MAC_IDLE) {
		state_ = MAC_RECV;
		pktRx_ = p;

		/* the last bit will arrive in txtime seconds */
		mhRecv_.schedule(p, netif_->txtime(p));
	}
	else {
		collision(p); //received packet while sending or receiving
	}
}

void Mac802_3::send(Packet *p, Handler *h) {
	HDR_CMN(p)->size() += ETHER_HDR_LEN; //XXX also preamble?
	callback_ = h;
	mhRetx_.packet()= p; //packet's buffered by mhRetx in case of retransmissions
	send(p);
}


void Mac802_3::send(Packet *p) {
	assert(callback_);
	if(pktTx_) {
		fprintf(stderr, "index: %d\n", index_);
		fprintf(stderr, "Retx Timer: %d\n", mhRetx_.busy());
		fprintf(stderr, "IFS  Timer: %d\n", mhIFS_.busy());
		fprintf(stderr, "Recv Timer: %d\n", mhRecv_.busy());
		fprintf(stderr, "Send Timer: %d\n", mhSend_.busy());
		exit(1);
	}

	/* Perform carrier sense  - if we were sending before, never mind state_ */
	if (mhIFS_.busy() || (state_ != MAC_IDLE)) {
		/* we'll try again when IDLE. It'll happen either when
                   reception completes, or if collision.  Either way,
                   we call resume() */
		return;
	}
	assert(state_!=MAC_SEND);
		
	pktTx_ = p;
	HDR_CMN(p)->direction()= hdr_cmn::DOWN; //down

	double txtime = netif_->txtime(p);
	/* Schedule transmission of the packet's last bit */
	mhSend_.schedule(txtime);

	// pass the packet to the PHY: need to send a copy, 
	// because there may be collision and it may be freed
	downtarget_->recv(p->copy()); 

	state_= MAC_SEND;
}

void Mac802_3::collision(Packet *p) {
	Packet::free(p);
	switch(state_) {
	case MAC_SEND:
		if (mhSend_.busy()) mhSend_.cancel();
		pktTx_= 0;

		if (!mhRetx_.busy()) {
			/* schedule retransmissions */
			assert(mhRetx_.packet());
			p= mhRetx_.packet();
			if (!mhRetx_.schedule()) {
				HDR_CMN(p)->size() -= ETHER_HDR_LEN;
				drop(p); // drop if backed off far enough
			}
		}
		break;
	case MAC_RECV:
		// more than 2 packets collisions possible
		if (mhRecv_.busy()) mhRecv_.cancel();
		if (pktRx_) {
			Packet::free(pktRx_);
			pktRx_ = 0;
		}
		break;
	default:
	  assert("SHOULD NEVER HAPPEN" == 0);
	}
	if (mhIFS_.busy()) mhIFS_.cancel();
	mhIFS_.schedule(netif_->txtime(IEEE_8023_JAMSIZE/8) + // jam time +
			IEEE_8023_IFS);                       // IFS
}

void Mac802_3::recv_complete(Packet *p) {
	assert(!mhRecv_.busy());
	assert(!mhSend_.busy());
	hdr_cmn *ch= HDR_CMN(p);
	/* Address Filtering */
	char* mha= (char*)p->access(hdr_mac::offset_);
	int dst= hdr_dst(mha);

	if (((u_int32_t)dst != MAC_BROADCAST) && (dst != index_)) {
		Packet::free(p);
		goto done;
	}
	/* Strip off the mac header */
	ch->size() -= ETHER_HDR_LEN;

	/* xxx FEC here */
	if( ch->error() ) {
		HDR_CMN(p)->size() -= ETHER_HDR_LEN;
		drop(p);
		goto done;
	}


	/* we could schedule an event to account */
	uptarget_->recv(p, (Handler*) 0);

	done:
	mhIFS_.schedule(IEEE_8023_IFS); 	// wait for one IFS, then resume
}

/* we call resume() in these cases:
   - successful transmission
   - whole packet's received
   - collision and backoffLimit's exceeded
   - collision while receiving */
void Mac802_3::resume() {
	assert(pktRx_ == 0);
	assert(pktTx_ == 0);
	assert(mhRecv_.busy() == 0);
	assert(mhSend_.busy() == 0);
	assert(mhIFS_.busy() == 0);

	state_= MAC_IDLE;

	if (mhRetx_.packet()) {
		if (!mhRetx_.busy()) {
			// we're not backing off and not sensing carrier right now: send
			send(mhRetx_.packet());
		}
	} else {
		if (callback_ && !mhRetx_.busy()) {
			assert(state_==MAC_IDLE && !mhSend_.busy());
			//calling callback_->handle may or may not change the value of callback_
			Handler* h= callback_; 
			callback_= 0;
			h->handle(0);
		}
	}
}


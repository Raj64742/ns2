/*
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory and the Daedalus
 *	research group at UC Berkeley.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "random.h"
#include "mac-multihop.h"

static class MultihopMacClass : public TclClass {
public:
	MultihopMacClass() : TclClass("Mac/Multihop") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new MultihopMac);
	}
} class_mac_multihop;

MultihopMac::MultihopMac() : Mac(), mode_(MAC_IDLE), peer_(0),
	pkt_(0), pendingPollEvent_(0),
	ph_(this), pah_(this), pnh_(this), pth_(this), mh_(this)
{
	/* Bind a bunch of variables to access from Tcl */
//	bind("mode_", &mode_);
	bind_time("tx_rx_", &tx_rx_);
	bind_time("rx_tx_", &rx_tx_);
	bind_time("rx_rx_", &rx_rx_);
}

int
MultihopMac::command(int argc, const char*const* argv)
{
	return Mac::command(argc, argv);
}

MultihopMac *
getPeerMac(Packet *p)
{
	BaseLL *ll = (BaseLL *)(p->target());
	Queue *ifq = ll->ifq();
	return (MultihopMac *)(ifq->target());	// peer mac
}

/*
 * Poll a peer node prior to a send.  There can be at most one POLL 
 * outstanding from a node at any point in time.  This is achieved implicitly
 * because there can be at most one packet down from LL (thru IFQ) to this MAC.
 */
void
MultihopMac::poll(Packet *p)
{
	Scheduler& s = Scheduler::instance();
double now = s.clock();
	MultihopMac *pm = getPeerMac(p);
	PollEvent *pe = new PollEvent(pm, this);
	
	cout << "polling\n";
	
	if (mode_ == MAC_SND || mode_ == MAC_RCV) { // schedule poll for later
		s.schedule(&bh_, pe, 0);
		return;
	}

	peer_ = pm;
	pkt_ = new Packet;
	memcpy(pkt_, p, sizeof(Packet));
	s.schedule(pm->ph(), pe, pollTxtime(MAC_POLLSIZE)); // poll handler
	double timeout = max(pm->rx_tx(), tx_rx_) + 2*pollTxtime(MAC_POLLSIZE);
	s.schedule(&bh_, pe, timeout); // backoff handler
}

/*
 * Handle a POLL request from a peer node's MAC.
 */
void
PollHandler::handle(Event *e)
{
	PollEvent *pe = (PollEvent *) e;
	Scheduler& s = Scheduler::instance();
double now = s.clock();
	MultihopMac* myMac = (MultihopMac *) (&mac_);
	MultihopMac* pm = myMac->peer(); // here, random unless in MAC_RCV mode

	cout << "handling poll\n";

	/*
	 * Send POLLACK if either IDLE or currently receiving 
	 * from same mac as the poller.
	 */
	if (myMac->mode() == MAC_IDLE || 
	    (myMac->mode() == MAC_RCV && pe->peerMac() == pm)) {
		myMac->mode(MAC_RCV);
		pm = pe->peerMac();
		myMac->peer(pm);
		PollEvent *pae = new PollEvent(pm, myMac); // POLLACK event
		double t = myMac->pollTxtime(MAC_POLLACKSIZE) + 
			max(myMac->tx_rx(), pm->rx_tx());
		s.schedule(pm->pah(), pae, t);
	}
	// could send NACKPOLL but don't (at least for now)
}

/*
 * Handle a POLLACK from a peer node's MAC.
 */
void
PollAckHandler::handle(Event *e)
{
	PollEvent *pe = (PollEvent *) e;
	Scheduler& s = Scheduler::instance();
	MultihopMac *myMac = (MultihopMac *)(&mac_);
	MultihopMac *pm = myMac->peer(); // now set to some random peer
	
	cout << "handling pollack\n";
	if (myMac->mode() == MAC_POLLING ||
	    (myMac->mode() == MAC_SND && pe->peerMac() == pm)) {
		myMac->backoffTime_(0);
		myMac->mode(MAC_SND);
		myMac->peer(pe->peerMac());
		s.cancel(myMac->pendingPE()); // cancel pending timeout
		myMac->send(myMac->pkt()); // send saved packet
	}
}

void 
PollNackHandler::handle(Event *e)
{
	
}

void
BackoffHandler::handle(Packet *p)
{
	Scheduler& s = Scheduler::instance();
	double now = s.clock();
	cout << "backing off...\n";
	backoffTime_ = 2*backoffTime_;
	double bTime = (1+Random::integer(MAC_TICK)*1./MAC_TICK) *
		backoffTime_ + 0.5;
	pendingPollEvent_ = new PollEvent(pm, this);
	s.schedule(pollTimeoutHandler, pendingPollEvent_, bTime);
}

void 
PollTimeoutHandler::handle(Event *e)
{
	MultihopMac *myMac = (MultihopMac *) &mac_;;
	myMac->poll(myMac->pkt());
}

/*
 * Actually send the data frame.
 */
void 
MultihopMac::send(Packet *p)
{
	if (mode_ != MAC_SND) {
		cout << "not ready to send\n";
		return;
	}	
	cout << "mhmac sending\n";
	
	Scheduler& s = Scheduler::instance();
	double txt = txtime(p);
	channel_->send(p, peer_->mh(), txt); // target is peer's mac handler
	s.schedule(callback_, &intr_, txt); // callback to higher layer (LL)
	mode_ = MAC_IDLE;
}

/*
 * Handle receives from peer and pass packet up to LL.
 */
void
MultihopMacHandler::handle(Event *e)
{
	cout << "mhmachandler\n";
	Scheduler& s = Scheduler::instance();
	Packet *p = (Packet *) e;
	// If this is an LL-ack, terminate polllink setup

	s.schedule(p->target(), e, 0); // pass packet up to LL (which acks)
	MultihopMac &myMac = (MultihopMac &) mac_;
	myMac.mode(MAC_IDLE);
}
	
/*
 * This is the call from the higher layer of the protocol stack (i.e., LL)
 */
void
MultihopMac::recv(Packet* p, Handler *h)
{
	cout << "mhmac recv from higher layer\n";
	callback_ = h;
	poll(p);		/* poll first */
}


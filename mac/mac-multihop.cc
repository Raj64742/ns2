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
#include "ip.h"

/* 
 * For debugging.
 */
void 
dump_iphdr(hdr_ip *iph) 
{
        printf("\tsrc = %d, ", iph->src_);
        printf("\tdst = %d\n", iph->dst_);
}       

static class MultihopMacClass : public TclClass {
public:
	MultihopMacClass() : TclClass("Mac/Multihop") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new MultihopMac);
	}
} class_mac_multihop;

MultihopMac::MultihopMac() : Mac(), mode_(MAC_IDLE), peer_(0),
	pkt_(0), pendingPollEvent_(0),
	ph_(this), pah_(this), pnh_(this), pth_(this), bh_(this), mh_(this)
{
	/* Bind a bunch of variables to access from Tcl */
//	bind("mode_", &mode_);
	bind_time("tx_rx_", &tx_rx_);
	bind_time("rx_tx_", &rx_tx_);
	bind_time("rx_rx_", &rx_rx_);
	bind_time("backoffBase_", &backoffBase_);
	backoffTime_ = backoffBase_;
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

	pendingPollEvent_ = new PollEvent(pm, this);
	
	cout << now << " polling " << ((hdr_cmn *)(p->access(0)))->uid()  << " size " << ((hdr_cmn*)p->access(0))->size() << " mode " << mode_ << "\n";
	hdr_ip  *iph  = (hdr_ip *)p->access(24);
	dump_iphdr(iph);
	
	pkt_ = new Packet;
	memcpy(pkt_, p, sizeof(Packet));
	double timeout = max(pm->rx_tx(), tx_rx_) + 4*pollTxtime(MAC_POLLSIZE);
	s.schedule(&bh_, pendingPollEvent_, timeout);

	if (mode_ == MAC_IDLE) {
		mode_ = MAC_POLLING;
		peer_ = pm;
		s.schedule(pm->ph(), (Event *)pe, pollTxtime(MAC_POLLSIZE));
	}
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
	
	cout << now << " In pollhandler\n";

	/*
	 * Send POLLACK if either IDLE or currently receiving 
	 * from same mac as the poller.
	 */
	if (myMac->mode() == MAC_IDLE) {
		cout << now << " Handling poll\n";
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
double now = s.clock();
	MultihopMac *myMac = (MultihopMac *)(&mac_);
	int mode = myMac->mode();
	MultihopMac *pm = myMac->peer(); // now set to some random peer
	
	cout << "In pollack for " << ((hdr_cmn *)(myMac->pkt()->access(0)))->uid() << " mode " << mode << "\n";
	if (mode == MAC_POLLING || mode == MAC_IDLE) {
		cout << now << " handling pollack for " << ((hdr_cmn *)(myMac->pkt()->access(0)))->uid() << "\n";
		myMac->backoffTime(myMac->backoffBase());
		myMac->mode(MAC_SND);
		myMac->peer(pe->peerMac());
		s.cancel(myMac->pendingPE()); // cancel pending timeout
		free(myMac->pendingPE());
		myMac->pendingPE(NULL);
		myMac->send(myMac->pkt()); // send saved packet
	}
}

void 
PollNackHandler::handle(Event *e)
{
	
}

void
BackoffHandler::handle(Event *e)
{
	Scheduler& s = Scheduler::instance();
double now = s.clock();
	MultihopMac *myMac = (MultihopMac *)(&mac_);
	if (myMac->mode() == MAC_POLLING) 
		myMac->mode(MAC_IDLE);
	double bTime = myMac->backoffTime(2*myMac->backoffTime());
	bTime = (1+Random::integer(MAC_TICK)*1./MAC_TICK)*bTime + 
		2*myMac->backoffBase();
/*	if (bTime/myMac->backoffBase() > myMac->maxAttempt()) {
		cout << "giving up attempts\n";
		
		return;
	}
*/     
	Packet *p = myMac->pkt();
	cout << now << " backing off " << ((hdr_cmn *)(myMac->pkt()->access(0)))->uid() << " for " << bTime << " s " << ((hdr_cmn*)p->access(0))->size() << "\n";
	dump_iphdr((hdr_ip *)p->access(24));
	
	s.schedule(myMac->pth(), myMac->pendingPE(), bTime);
}

void 
PollTimeoutHandler::handle(Event *e)
{
	Scheduler& s = Scheduler::instance();
	double  now = s.clock();
	MultihopMac *myMac = (MultihopMac *) &mac_;;
	cout << now << " timeout handler for " << ((hdr_cmn *)(myMac->pkt()->access(0)))->uid() << "\n";
	myMac->poll(myMac->pkt());
}



/*
 * Actually send the data frame.
 */
void 
MultihopMac::send(Packet *p)
{
	Scheduler& s = Scheduler::instance();
double now = s.clock();
	if (mode_ != MAC_SND) {
		cout << now << " not ready to send\n";
		return;
	}	
	cout << now << " mhmac sending " << ((hdr_cmn *)(p->access(0)))->uid() << " size " << ((hdr_cmn*)p->access(off_cmn_))->size() << "\n";

	hdr_ip  *iph  = (hdr_ip *)p->access(24);
	dump_iphdr(iph);
	
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
	Scheduler& s = Scheduler::instance();
double now = s.clock();	
	Packet *p = (Packet *) e;
	// If this is an LL-ack, terminate polllink setup

	cout << now << " recv " << ((hdr_cmn *)(p->access(0)))->uid() << " from peer " << " size " << ((hdr_cmn*)p->access(0))->size() << "\n";
	hdr_ip  *iph  = (hdr_ip *)p->access(24);
	dump_iphdr(iph);	
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
	Scheduler& s = Scheduler::instance();
double now = s.clock();
	cout << now << " mhmac recv from higher layer " << ((hdr_cmn *)(p->access(0)))->uid()  << "\n";
	callback_ = h;
	poll(p);		/* poll first */
}


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
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the research group may be used
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mac/mac-multihop.cc,v 1.4 1997/10/26 05:44:23 hari Exp $ (UCB)";
#endif

#include "template.h"
#include "channel.h"
#include "mac-multihop.h"

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
	TclObject* create(int, const char*const*) {
		return (new MultihopMac);
	}
} class_mac_multihop;

MultihopMac::MultihopMac() : Mac(), mode_(MAC_IDLE), peer_(0),
	pendingPollEvent_(0), pkt_(0),
	ph_(this), pah_(this), pnh_(this), pth_(this), bh_(this)
{
	/* Bind a bunch of variables to access from Tcl */
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

/*
 * Returns 1 iff the specified MAC is in the prescribed state, AND all
 * the other MACs are IDLE.
 */
int
MultihopMac::checkInterfaces(int state) 
{
	MultihopMac *p;
	
	if (!(mode_ & state))
		return 0;
	else if (macList_ == 0)
		return 1;
	for (p = (MultihopMac *)macList_; p != this && p != NULL; 
	     p = (MultihopMac *)(p->macList())) {
		if (p->mode() != MAC_IDLE) {
			return 0;
		}
	}
	return 1;
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
	MultihopMac *pm = (MultihopMac*) getPeerMac(p);
	PollEvent *pe = new PollEvent(pm, this);

	pendingPollEvent_ = new PollEvent(pm, this);
	
#ifdef notdef
double now = s.clock();
cout << now << " polling " << ((hdr_cmn *)(p->access(0)))->uid()  << " size " << ((hdr_cmn*)p->access(0))->size() << " mode " << mode_ << "\n";
hdr_ip  *iph  = (hdr_ip *)p->access(24);
dump_iphdr(iph);
#endif

	pkt_ = p->copy();
	double timeout = max(pm->rx_tx(), tx_rx_) + 4*pollTxtime(MAC_POLLSIZE);
	s.schedule(&bh_, pendingPollEvent_, timeout);

	if (checkInterfaces(MAC_IDLE)) {
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
	MultihopMac* pm = mac_->peer(); // here, random unless in MAC_RCV mode
	
	/*
	 * Send POLLACK if either IDLE or currently receiving 
	 * from same mac as the poller.
	 */
	if (mac_->checkInterfaces(MAC_IDLE)) { // all interfaces must be IDLE
		mac_->mode(MAC_RCV);
		pm = pe->peerMac();
		mac_->peer(pm);
		PollEvent *pae = new PollEvent(pm, mac_); // POLLACK event
		double t = mac_->pollTxtime(MAC_POLLACKSIZE) + 
			max(mac_->tx_rx(), pm->rx_tx());
		s.schedule(pm->pah(), pae, t);
	} else {
//		printf("ignoring poll %d\n", mac_->label());
	// could send NACKPOLL but don't (at least for now)
	}
}

/*
 * Handle a POLLACK from a peer node's MAC.
 */
void
PollAckHandler::handle(Event *e)
{
	PollEvent *pe = (PollEvent *) e;
	Scheduler& s = Scheduler::instance();
	
#ifdef notdef
	double now = s.clock();
	int mode = mac_->mode();
//cout << "In pollack for " << ((hdr_cmn *)(mac_->pkt()->access(0)))->uid() << " mode " << mode << "\n";
#endif

	if (mac_->checkInterfaces(MAC_POLLING | MAC_IDLE)) {

#ifdef notdef
//cout << now << " handling pollack for " << ((hdr_cmn *)(mac_->pkt()->access(0)))->uid() << "\n";
#endif

		mac_->backoffTime(mac_->backoffBase());
		mac_->mode(MAC_SND);
		mac_->peer(pe->peerMac());
		s.cancel(mac_->pendingPE()); // cancel pending timeout
		free(mac_->pendingPE());
		mac_->pendingPE(NULL);
		mac_->send(mac_->pkt()); // send saved packet
	}
}

void 
PollNackHandler::handle(Event *)
{
}

void
BackoffHandler::handle(Event *)
{
	Scheduler& s = Scheduler::instance();
	if (mac_->mode() == MAC_POLLING) 
		mac_->mode(MAC_IDLE);
	double bTime = mac_->backoffTime(2*mac_->backoffTime());
	bTime = (1+Random::integer(MAC_TICK)*1./MAC_TICK)*bTime + 
		2*mac_->backoffBase();

#ifdef notdef
double now = s.clock();
if (bTime/mac_->backoffBase() > mac_->maxAttempt()) {
	cout << "giving up attempts\n";
	return;
}
Packet *p = mac_->pkt();
cout << now << " backing off " << ((hdr_cmn *)(mac_->pkt()->access(0)))->uid() << " for " << bTime << " s " << ((hdr_cmn*)p->access(0))->size() << "\n";
dump_iphdr((hdr_ip *)p->access(24));
#endif
//	printf("backing off %d\n", mac_->label());
	s.schedule(mac_->pth(), mac_->pendingPE(), bTime);
}

void 
PollTimeoutHandler::handle(Event *)
{

#ifdef notdef
Scheduler& s = Scheduler::instance();
double  now = s.clock();
cout << now << " timeout handler for " << ((hdr_cmn *)(mac_->pkt()->access(0)))->uid() << "\n";
#endif
	mac_->poll(mac_->pkt());
}


/*
 * Actually send the data frame.
 */
void 
MultihopMac::send(Packet *p)
{

	Scheduler& s = Scheduler::instance();

#ifdef notdef
double now = s.clock();
#endif

	if (mode_ != MAC_SND) {
//cout << now << " not ready to send\n";
		return;
	}	

#ifdef notdef
//cout << now << " mhmac sending " << ((hdr_cmn *)(p->access(0)))->uid() << " size " << ((hdr_cmn*)p->access(off_cmn_))->size() << "\n";

hdr_ip  *iph  = (hdr_ip *)p->access(24);
//dump_iphdr(iph);
#endif

	double txt = txtime(p);
	channel_->send(p, txt); // target is peer's mac handler
	s.schedule(callback_, &intr_, txt); // callback to higher layer (LL)
	mode_ = MAC_IDLE;
}

/*
 * This is the call from the higher layer of the protocol stack (i.e., LL)
 */
void
MultihopMac::recv(Packet* p, Handler *h)
{
	if (h == 0) {		// from MAC classifier (pass pkt to LL)
		mode_ = MAC_IDLE;
		target_->recv(p);
		return;
	}
	callback_ = h;
	hdr_mac *mach = (hdr_mac *)p->access(off_mac_);
	mach->macSA() = label_;
	if (mach->ftype() == MF_ACK) {
		mode_ = MAC_SND;
		send(p);
	} else {
		mach->ftype() = MF_DATA;
		poll(p);		/* poll first */
	}
}

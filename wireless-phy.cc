/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-  *
 *
 * Copyright (c) 1996 Regents of the University of California.
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
 *
 * Ported from CMU/Monarch's code, nov'98 -Padma Haldar.
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

#include <packet.h>
#include <ip.h>
#include <agent.h>
#include <trace.h>

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


WirelessPhy::WirelessPhy() : Phy()
{
	node_ = 0;
	propagation_ = 0;
	modulation_ = 0;
	bandwidth_ = 2*1e6;                 // 100 kb
	//	Pt_ = 16.267 * 1e-3;   // 16.267 mW for 100m range with TwoRay model
	Pt_ = pow(10, 2.45) * 1e-3;         // 24.5 dbm, ~ 281.8mw
	Pr_ = Pt_;
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
  if(argc == 3) {
	  if (strcasecmp(argv[1], "setTxPower") == 0) {
		  Pt_ = atof(argv[2]);
		  return TCL_OK;
	  }
	  else if (strcasecmp(argv[1], "setRxPower") == 0) {
		  Pr_ = atof(argv[2]);
		  return TCL_OK;
	  }
          else if( (obj = TclObject::lookup(argv[2])) == 0) {
		  fprintf(stderr, "WirelessPhy: %s lookup of %s failed\n", 
			  argv[1], argv[2]);
		  return TCL_ERROR;
	  }
	  if (strcmp(argv[1], "propagation") == 0) {
		  assert(propagation_ == 0);
		  propagation_ = (Propagation*) obj;
		  return TCL_OK;
	  }
	  else if (strcasecmp(argv[1], "antenna") == 0) {
		  ant_ = (Antenna*) obj;
		  return TCL_OK;
	  }
	  else if (strcasecmp(argv[1], "node") == 0) {
		  assert(node_ == 0);
		  node_ = (MobileNode*) obj;
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
	 * Decrease node's energy
	 */
	if(node_->energy_model()) {
		double txtime = (8. * hdr_cmn::access(p)->size()) / bandwidth_;
		(node_->energy_model())->DecrTxEnergy(txtime,Pt_);
	}

	/*
	 *  Stamp the packet with the interface arguments
	 */
	p->txinfo_.stamp(node_, ant_->copy(), Pt_, lambda_);
	
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
    
    Pr = propagation_->Pr(&p->txinfo_, &s, this);
    
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
	     p->txinfo_.getNode()->index(),
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
  p->txinfo_.getAntenna()->release();
  //*RxPr = Pr;

  /* WILD HACK: The following two variables are a wild hack.
     They will go away in the next release...
     They're used by the mac-802_11 object to determine
     capture.  This will be moved into the net-if family of 
     objects in the future. */
  p->txinfo_.RxPr = Pr;
  p->txinfo_.CPThresh = CPThresh_;

  /*
   * Decrease energy if packet successfully received
   */
  if(pkt_recvd 	&& node_->energy_model()) {
	  double rcvtime = (8. * hdr_cmn::access(p)->size()) / bandwidth_;
	  (node_->energy_model())->DecrRcvEnergy(rcvtime,Pr);
  }

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











/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
 *
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
 *	Engineering Group at Lawrence Berkeley Laboratory.
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
 * $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mac/mac-802_11.cc,v 1.43 2003/09/30 18:42:06 haldar Exp $
 *
 * Ported from CMU/Monarch's code, nov'98 -Padma.
 */

#include "delay.h"
#include "connector.h"
#include "packet.h"
#include "random.h"
#include "mobilenode.h"

// #define DEBUG 99

#include "arp.h"
#include "ll.h"
#include "mac.h"
#include "mac-timers.h"
#include "mac-802_11.h"
#include "cmu-trace.h"


// XXX Can't we make these macros inline methods? Otherwise why should we have
// inline methods at all??

// change wrt Mike's code

/*#define CHECK_BACKOFF_TIMER()						\
{									\
	if(is_idle() && mhBackoff_.paused())				\
		mhBackoff_.resume(difs_);				\
	if(! is_idle() && mhBackoff_.busy() && ! mhBackoff_.paused())	\
		mhBackoff_.pause();					\
}
*/
/*
#define TRANSMIT(p, t)                                                  \
{                                                                       \
	tx_active_ = 1;                                                  \
                                                                        \
        /*                                                              \
         * If I'm transmitting without doing CS, such as when           \
         * sending an ACK, any incoming packet will be "missed"         \
         * and hence, must be discarded.                                \
                                                                      \
        if(rx_state_ != MAC_IDLE) {                                      \
                struct hdr_mac802_11 *dh = HDR_MAC802_11(p);                  \
                                                                        \
                assert(dh->dh_fc.fc_type == MAC_Type_Control);          \
                assert(dh->dh_fc.fc_subtype == MAC_Subtype_ACK);        \
                                                                        \
                assert(pktRx_);                                          \
                struct hdr_cmn *ch = HDR_CMN(pktRx_);                    \
                                                                        \
                ch->error() = 1;         force packet discard       \
        }                                                               \
                                                                        \
                                                                      \
         * pass the packet on the "interface" which will in turn        \
         * place the packet on the channel.                             \
         *                                                              \
         * NOTE: a handler is passed along so that the Network          \
         *       Interface can distinguish between incoming and         \
         *       outgoing packets.                                      \
                                                                      \
        downtarget_->recv(p->copy(), this);                             \
                                                                        \
        mhSend_.start(t);                                                \
                                                                        \
	mhIF_.start(txtime(p));                                         \
}
*/

/*#define SET_RX_STATE(x)			\
{					\
	rx_state_ = (x);		\
	CHECK_BACKOFF_TIMER();		\
}

#define SET_TX_STATE(x)				\
{						\
	tx_state_ = (x);			\
	CHECK_BACKOFF_TIMER();			\
}
*/


/* ======================================================================
   Global Variables
   ====================================================================== */

/*static PHY_MIB PMIB =
{
	DSSS_CWMin, DSSS_CWMax, DSSS_SlotTime, DSSS_CCATime,
	DSSS_RxTxTurnaroundTime, DSSS_SIFSTime, DSSS_PreambleLength,
	DSSS_PLCPHeaderLength, DSSS_PLCPDataRate
};	

static MAC_MIB MMIB =
{
	MAC_RTSThreshold, MAC_ShortRetryLimit,
	MAC_LongRetryLimit, MAC_FragmentationThreshold,
	MAC_MaxTransmitMSDULifetime, MAC_MaxReceiveLifetime,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
*/
 /* our backoff timer doesn't count down in idle times during a
!  * frame-exchange sequence as the mac tx state isn't idle; genreally
!  * these idle times are less than DIFS and won't contribute to
!  * counting down the backoff period, but this could be a real
!  * problem if the frame exchange ends up in a timeout! in that case,
!  * i.e. if a timeout happens we've not been counting down for the
!  * duration of the timeout, and in fact begin counting down only
!  * DIFS after the timeout!! we lose the timeout interval - which
!  * will is not the REAL case! also, the backoff timer could be NULL
!  * and we could have a pending transmission which we could have
!  * sent! one could argue this is an implementation artifact which
!  * doesn't violate the spec.. and the timeout interval is expected
!  * to be less than DIFS .. which means its not a lot of time we
!  * lose.. anyway if everyone hears everyone the only reason a ack will
!  * be delayed will be due to a collision => the medium won't really be
!  * idle for a DIFS for this to really matter!!
  */

 inline void
 Mac802_11::checkBackoffTimer()
 {
       if(is_idle() && mhBackoff_.paused())
               mhBackoff_.resume(phymib_.getDIFS());
       if(! is_idle() && mhBackoff_.busy() && ! mhBackoff_.paused())
               mhBackoff_.pause();
  }

 inline void
 Mac802_11::transmit(Packet *p, double timeout)
 {
       tx_active_ = 1;

       if (EOTtarget_) {
               assert (eotPacket_ == NULL);
               eotPacket_ = p->copy();
       }

         /*
!          * If I'm transmitting without doing CS, such as when
!          * sending an ACK, any incoming packet will be "missed"
!          * and hence, must be discarded.
          */
         if(rx_state_ != MAC_IDLE) {
                 struct hdr_mac802_11 *dh = HDR_MAC802_11(p);

                 assert(dh->dh_fc.fc_type == MAC_Type_Control);
                 assert(dh->dh_fc.fc_subtype == MAC_Subtype_ACK);

                 assert(pktRx_);
                 struct hdr_cmn *ch = HDR_CMN(pktRx_);

                 ch->error() = 1;        /* force packet discard */
         }

         /*
!          * pass the packet on the "interface" which will in turn
!          * place the packet on the channel.
!          *
!          * NOTE: a handler is passed along so that the Network
!          *       Interface can distinguish between incoming and
!          *       outgoing packets.

          *       Interface can distinguish between incoming and
!          *       outgoing packets.
          */
         downtarget_->recv(p->copy(), this);

         mhSend_.start(timeout);

       mhIF_.start(txtime(p));

}
 inline void
 Mac802_11::setRxState(MacState newState)
 {
       rx_state_ = newState;
       checkBackoffTimer();
 }

 inline void
 Mac802_11::setTxState(MacState newState)
  {
       tx_state_ = newState;
       checkBackoffTimer();
 }

 #ifdef MIKE // need to add rst_cw etc.
 inline void
 Mac802_11::postBackoff(int pri)
  {
       rst_cw(pri);
       assert(!mhBackoff_.busy(pri));
       mhBackoff_.start(pri, cw_[pri], CWOffset_[pri], true);
 }
 #endif

 /*
  * This macro is only used in one place, and should be removed.
  */

 #define CFP_BEACON(__p) (                                                                       \
         (((struct beacon_frame *)HDR_MAC802_11(__p))->bf_fc.fc_type == MAC_Type_Management)     \
         &&                                                                                      \
         (((struct beacon_frame *)HDR_MAC802_11(__p))->bf_fc.fc_subtype == MAC_Subtype_Beacon)   \
 )

// Mike change ends


/* ======================================================================
   TCL Hooks for the simulator
   ====================================================================== */
static class Mac802_11Class : public TclClass {
public:
	Mac802_11Class() : TclClass("Mac/802_11") {}
	TclObject* create(int, const char*const*) {
	// change wrt Mike's code
	// return (new Mac802_11(&PMIB, &MMIB));
	return (new Mac802_11());
	// Mike change ends!

}
} class_mac802_11;


// change wrt Mike's code

  /* ======================================================================
!    Mac  and Phy MIB Class Functions
     ====================================================================== */

 PHY_MIB::PHY_MIB(Mac802_11 *parent)
  {
       /*
        * Bind the phy mib objects.  Note that these will be bound
!        * to Mac/802_11 variables
        */

       parent->bind("CWMin_", &CWMin);
       parent->bind("CWMax_", &CWMax);
       parent->bind("SlotTime_", &SlotTime);
       parent->bind("SIFS_", &SIFSTime);
       parent->bind("PreambleLength_", &PreambleLength);
       parent->bind("PLCPHeaderLength_", &PLCPHeaderLength);
       parent->bind_bw("PLCPDataRate_", &PLCPDataRate);
 }

 MAC_MIB::MAC_MIB(Mac802_11 *parent)
 {
       /*
        * Bind the phy mib objects.  Note that these will be bound
!        * to Mac/802_11 variables
        */

       parent->bind("RTSThreshold_", &RTSThreshold);
       parent->bind("ShortRetryLimit_", &ShortRetryLimit);
       parent->bind("LongRetryLimit_", &LongRetryLimit);
 }
// Mike change ends

/* ======================================================================
   Mac Class Functions
   ====================================================================== */
// change wrt Mike's code
//Mac802_11::Mac802_11(PHY_MIB *p, MAC_MIB *m) : Mac(), mhIF_(this), mhNav_(this), mhRecv_(this), mhSend_(this), mhDefer_(this, p->SlotTime), mhBackoff_(this, p->SlotTime)
Mac802_11::Mac802_11() : Mac(), mhIF_(this), mhNav_(this), mhRecv_(this), mhSend_(this), mhDefer_(this), mhBackoff_(this),macmib_(this), phymib_(this), mhBeacon_(this)
// Mike change ends
{
	// change wrt Mike
	//macmib_ = m;
	//phymib_ = p;
	// change ends
	
	nav_ = 0.0;
	
	tx_state_ = rx_state_ = MAC_IDLE;
	tx_active_ = 0;
	
	// change wrt Mike
	eotPacket_ = NULL;
	// change ends


	pktRTS_ = 0;
	pktCTRL_ = 0;
		
	// change wrt Mike's code
	//cw_ = phymib_->CWMin;
	cw_ = phymib_.getCWMin();
	// change ends


	ssrc_ = slrc_ = 0;
	
	// change wrt Mike's code

	//sifs_ = phymib_->SIFSTime;
	//pifs_ = sifs_ + phymib_->SlotTime;
	//difs_ = sifs_ + 2*phymib_->SlotTime;
	
	// see (802.11-1999, 9.2.10) 
	//eifs_ = sifs_ + (8 * ETHER_ACK_LEN / phymib_->PLCPDataRate) + difs_;
	
	//tx_sifs_ = sifs_ - phymib_->RxTxTurnaroundTime;
	//tx_pifs_ = tx_sifs_ + phymib_->SlotTime;
	//tx_difs_ = tx_sifs_ + 2 * phymib_->SlotTime;
	
	sta_seqno_ = 1;
	cache_ = 0;
	cache_node_count_ = 0;
	
	// chk if basic/data rates are set
	// otherwise use bandwidth_ as default;
	
	Tcl& tcl = Tcl::instance();
	tcl.evalf("Mac/802_11 set basicRate_");
	if (strcmp(tcl.result(), "0") != 0) 
		bind_bw("basicRate_", &basicRate_);
	else
		basicRate_ = bandwidth_;

	tcl.evalf("Mac/802_11 set dataRate_");
	if (strcmp(tcl.result(), "0") != 0) 
		bind_bw("dataRate_", &dataRate_);
	else
		dataRate_ = bandwidth_;

	// change wrt Mike
        EOTtarget_ = 0;
       	bss_id_ = IBSS_ID;
	//-ak-----------
	//printf("bssid in constructor %d\n",bss_id_);


	// change ends


}


int
Mac802_11::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		// change wrt Mike
		//if (strcmp(argv[1], "log-target") == 0) {
		 if (strcmp(argv[1], "eot-target") == 0) {
                         EOTtarget_ = (NsObject*) TclObject::lookup(argv[2]);
                         if (EOTtarget_ == 0)
                                 return TCL_ERROR;
                         return TCL_OK;
               } else if (strcmp(argv[1], "bss_id") == 0) {
                       bss_id_ = atoi(argv[2]);

//-ak-----		       
//printf("in command bssid %d \n",bss_id_);

                       return TCL_OK;
                 } else if (strcmp(argv[1], "log-target") == 0) {
		// change ends
 
 		logtarget_ = (NsObject*) TclObject::lookup(argv[2]);
			if(logtarget_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		} else if(strcmp(argv[1], "nodes") == 0) {
			if(cache_) return TCL_ERROR;
			cache_node_count_ = atoi(argv[2]);
			cache_ = new Host[cache_node_count_ + 1];
			assert(cache_);
			bzero(cache_, sizeof(Host) * (cache_node_count_+1 ));
			return TCL_OK;
		}
	}
	return Mac::command(argc, argv);
}

/* ======================================================================
   Debugging Routines
   ====================================================================== */
void
Mac802_11::trace_pkt(Packet *p) {
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_mac802_11* dh = HDR_MAC802_11(p);
	u_int16_t *t = (u_int16_t*) &dh->dh_fc;

	fprintf(stderr, "\t[ %2x %2x %2x %2x ] %x %s %d\n",
		*t, dh->dh_duration,
		// change wrt Mike
		//ETHER_ADDR(dh->dh_da), ETHER_ADDR(dh->dh_sa),
		 ETHER_ADDR(dh->dh_ra), ETHER_ADDR(dh->dh_ta),
		// change ends
		index_, packet_info.name(ch->ptype()), ch->size());
}

void
Mac802_11::dump(char *fname)
{
	fprintf(stderr,
		"\n%s --- (INDEX: %d, time: %2.9f)\n",
		fname, index_, Scheduler::instance().clock());

	fprintf(stderr,
		"\ttx_state_: %x, rx_state_: %x, nav: %2.9f, idle: %d\n",
		tx_state_, rx_state_, nav_, is_idle());

	fprintf(stderr,
		"\tpktTx_: %x, pktRx_: %x, pktRTS_: %x, pktCTRL_: %x, callback: %x\n",
		(int) pktTx_, (int) pktRx_, (int) pktRTS_,
		(int) pktCTRL_, (int) callback_);

	fprintf(stderr,
		"\tDefer: %d, Backoff: %d (%d), Recv: %d, Timer: %d Nav: %d\n",
		mhDefer_.busy(), mhBackoff_.busy(), mhBackoff_.paused(),
		mhRecv_.busy(), mhSend_.busy(), mhNav_.busy());
	fprintf(stderr,
		"\tBackoff Expire: %f\n",
		mhBackoff_.expire());
}


/* ======================================================================
   Packet Headers Routines
   ====================================================================== */
inline int
Mac802_11::hdr_dst(char* hdr, int dst )
{
	struct hdr_mac802_11 *dh = (struct hdr_mac802_11*) hdr;
	//dst = (u_int32_t)(dst);

	// change wrt Mike
	/*if(dst > -2)
		STORE4BYTE(&dst, (dh->dh_da));

	return ETHER_ADDR(dh->dh_da);*/
	
       if (dst > -2) {
               if ((bss_id() == IBSS_ID) || (addr() == bss_id())) {
                       /* if I'm AP (2nd condition above!), the dh_3a
!                        * is already set by the MAC whilst fwding; if
!                        * locally originated pkt, it might make sense
!                        * to set the dh_3a to myself here! don't know
!                        * how to distinguish between the two here - and
!                        * the info is not critical to the dst station
!                        * anyway!
                        */
                       STORE4BYTE(&dst, (dh->dh_ra));
               } else {
                       /* in BSS mode, the AP forwards everything;
!                        * therefore, the real dest goes in the 3rd
!                        * address, and the AP address goes in the
!                        * destination address
                        */
                       STORE4BYTE(&bss_id_, (dh->dh_ra));
                       STORE4BYTE(&dst, (dh->dh_3a));
               }
       }


       return (u_int32_t)ETHER_ADDR(dh->dh_ra);
	// change ends
}

inline int 
Mac802_11::hdr_src(char* hdr, int src )
{
	struct hdr_mac802_11 *dh = (struct hdr_mac802_11*) hdr;
	// change wrt Mike's code
	/*if(src > -2)
		STORE4BYTE(&src, (dh->dh_sa));
	return ETHER_ADDR(dh->dh_sa);*/
        if(src > -2)
               STORE4BYTE(&src, (dh->dh_ta));
        return ETHER_ADDR(dh->dh_ta);


	// change ends

}

inline int 
Mac802_11::hdr_type(char* hdr, u_int16_t type)
{
	struct hdr_mac802_11 *dh = (struct hdr_mac802_11*) hdr;
	if(type)
		STORE2BYTE(&type,(dh->dh_body));
	return GET2BYTE(dh->dh_body);
}


/* ======================================================================
   Misc Routines
   ====================================================================== */
// change wrt Mike's code
#undef INTERLEAVING
// change ends

inline int
Mac802_11::is_idle()
{
	// change wrt Mike's code
 #ifdef INTERLEAVING
 #define INTERLEAVING_TIME 0.030
       Scheduler &s = Scheduler::instance();
       double st = s.clock();

       double nextCycle = floor(st / (INTERLEAVING_TIME*2) + 1)
                               * INTERLEAVING_TIME*2;
       double nextSwitch = nextCycle - INTERLEAVING_TIME
               - phymib_.getDIFS()
               - phymib_.getSIFS()
                              + txtime(phymib_.getCTSlen(), basicRate_)
               - txtime(phymib_.getCTSlen(), basicRate_) * 2
               - (Random::random() % cw_) * phymib_.getSlotTime();
       if ((st >= nextSwitch) && (st < nextCycle)) {
               if (nextCycle > nav_) {
                       nav_ = nextCycle;
                       if (mhNav_.busy()) {
                               mhNav_.stop();
                       }
                       mhNav_.start(nextCycle - st);
               }
               return(0);
       }

 #endif

	// change ends

	if(rx_state_ != MAC_IDLE)
		return 0;
	if(tx_state_ != MAC_IDLE)
		return 0;
	if(nav_ > Scheduler::instance().clock())
		return 0;
	
	return 1;
}

void
Mac802_11::discard(Packet *p, const char* why)
{
	hdr_mac802_11* mh = HDR_MAC802_11(p);
	hdr_cmn *ch = HDR_CMN(p);

#if 0
	/* old logic 8/8/98 -dam */
	/*
	 * If received below the RXThreshold, then just free.
	 */
	if(p->txinfo_.Pr < p->txinfo_.ant.RXThresh) {
		Packet::free(p);
		//p = 0;
		return;
	}
#endif // 0

	/* if the rcvd pkt contains errors, a real MAC layer couldn't
	   necessarily read any data from it, so we just toss it now */
	if(ch->error() != 0) {
		Packet::free(p);
		//p = 0;
		return;
	}

	switch(mh->dh_fc.fc_type) {
	case MAC_Type_Management:
		drop(p, why);
		return;
	case MAC_Type_Control:
		switch(mh->dh_fc.fc_subtype) {
		case MAC_Subtype_RTS:
			// change wrt Mike's code
			//if((u_int32_t)ETHER_ADDR(mh->dh_sa) == 
			 if((u_int32_t)ETHER_ADDR(mh->dh_ta) ==  (u_int32_t)index_) {
				drop(p, why);
				return;
			}
			//change ends

			/* fall through - if necessary */
		case MAC_Subtype_CTS:
		case MAC_Subtype_ACK:
			// change wrt Mike's code
			//if((u_int32_t)ETHER_ADDR(mh->dh_da) == 
			if((u_int32_t)ETHER_ADDR(mh->dh_ra) == \
			 (u_int32_t)index_) {
				drop(p, why);
				return;
			}
			break;
		default:
			fprintf(stderr, "invalid MAC Control subtype\n");
			exit(1);
		}
		break;
	case MAC_Type_Data:
		switch(mh->dh_fc.fc_subtype) {
		case MAC_Subtype_Data:
			// change wrt Mike's code
			/*if((u_int32_t)ETHER_ADDR(mh->dh_da) == \
			   (u_int32_t)index_ ||
			   (u_int32_t)ETHER_ADDR(mh->dh_sa) == \
			   (u_int32_t)index_ ||
			   (u_int32_t)ETHER_ADDR(mh->dh_da) == MAC_BROADCAST) {
				drop(p);
				return;
			*/
			if((u_int32_t)ETHER_ADDR(mh->dh_ra) == \
                           (u_int32_t)index_ ||
                          (u_int32_t)ETHER_ADDR(mh->dh_ta) == \
                           (u_int32_t)index_ ||
                          (u_int32_t)ETHER_ADDR(mh->dh_ra) == MAC_BROADCAST) {
                                drop(p);
                                return;
			}
			break;
		default:
			fprintf(stderr, "invalid MAC Data subtype\n");
			exit(1);
		}
		break;
	default:
		fprintf(stderr, "invalid MAC type (%x)\n", mh->dh_fc.fc_type);
		trace_pkt(p);
		exit(1);
	}
	Packet::free(p);
}

void
Mac802_11::capture(Packet *p)
{
	/*
	 * Update the NAV so that this does not screw
	 * up carrier sense.
	 */
	// change wrt Mike's code
	//set_nav(usec(eifs_ + txtime(p)));
	
	set_nav(usec(phymib_.getEIFS() + txtime(p)));
	Packet::free(p);
}

void
Mac802_11::collision(Packet *p)
{
	switch(rx_state_) {
	case MAC_RECV:
		// change wrt Mike's code
		//SET_RX_STATE(MAC_COLL);
		setRxState(MAC_COLL);
		/* fall through */
	case MAC_COLL:
		assert(pktRx_);
		assert(mhRecv_.busy());
		/*
		 *  Since a collision has occurred, figure out
		 *  which packet that caused the collision will
		 *  "last" the longest.  Make this packet,
		 *  pktRx_ and reset the Recv Timer if necessary.
		 */
		if(txtime(p) > mhRecv_.expire()) {
			mhRecv_.stop();
			discard(pktRx_, DROP_MAC_COLLISION);
			pktRx_ = p;
			mhRecv_.start(txtime(pktRx_));
		}
		else {
			discard(p, DROP_MAC_COLLISION);
		}
		break;
	default:
		assert(0);
	}
}

void
Mac802_11::tx_resume()
{
	double rTime;
	assert(mhSend_.busy() == 0);
	assert(mhDefer_.busy() == 0);

	if(pktCTRL_) {
		/*
		 *  Need to send a CTS or ACK.
		 */
		// change wrt Mike's code
		//mhDefer_.start(sifs_);
		mhDefer_.start(phymib_.getSIFS());


	} else if(pktRTS_) {
		if(mhBackoff_.busy() == 0) {
			// change wrt Mike's code
			/*
			rTime = (Random::random() % cw_) * phymib_->SlotTime;
			mhDefer_.start(difs_ + rTime);
			*/

			 rTime = (Random::random() % cw_)
                               * phymib_.getSlotTime();
                       mhDefer_.start( phymib_.getDIFS() + rTime);

		}
	} else if(pktTx_) {
		if(mhBackoff_.busy() == 0) {
			hdr_cmn *ch = HDR_CMN(pktTx_);
			struct hdr_mac802_11 *mh = HDR_MAC802_11(pktTx_);
			
			// change wrt Mike's code
			/*
			if ((u_int32_t) ch->size() < macmib_->RTSThreshold ||
			    (u_int32_t) ETHER_ADDR(mh->dh_da) == MAC_BROADCAST) {
				rTime = (Random::random() % cw_) * phymib_->SlotTime;
				mhDefer_.start(difs_ + rTime);
			} else {
				mhDefer_.start(sifs_);
			}
			*/
			if ((u_int32_t) ch->size() < macmib_.getRTSThreshold()
                                || (u_int32_t) ETHER_ADDR(mh->dh_ra)
                                               == MAC_BROADCAST)
                       {
                               rTime = (Random::random() % cw_)
                                       * phymib_.getSlotTime();
                               mhDefer_.start(phymib_.getDIFS() + rTime);
                        } else {
                               mhDefer_.start(phymib_.getSIFS());
                        }


		}
	} else if(callback_) {
		Handler *h = callback_;
		callback_ = 0;
		h->handle((Event*) 0);
	}
	// change wrt Mike's code
	//SET_TX_STATE(MAC_IDLE);
	setTxState(MAC_IDLE);

}

void
Mac802_11::rx_resume()
{
	assert(pktRx_ == 0);
	assert(mhRecv_.busy() == 0);
	// change wrt Mike's code
	//SET_RX_STATE(MAC_IDLE);
	setRxState(MAC_IDLE);

}


/* ======================================================================
   Timer Handler Routines
   ====================================================================== */
void
Mac802_11::backoffHandler()
{
///-ak-------
//	printf("backoff andler \n");

	if(pktCTRL_) {
		assert(mhSend_.busy() || mhDefer_.busy());
		return;
	}

	if(check_pktRTS() == 0)
		return;

	if(check_pktTx() == 0)
		return;
}
// change wrt Mike's code
 void Mac802_11::beaconHandler()
 {
       /* schedule timer for the next beacon! */
 //    mhBeacon_.start(cfp_period_);
 }

void
Mac802_11::deferHandler()
{
	assert(pktCTRL_ || pktRTS_ || pktTx_);

	if(check_pktCTRL() == 0)
		return;

	assert(mhBackoff_.busy() == 0);
	//if (mhBackoff_.busy() != 0)
	//{
	//	printf("deferHandler:mhBackoff_ busy!\n");
	//	return;
	//}
	if(check_pktRTS() == 0)
		return;

	if(check_pktTx() == 0)
		return;
}

void
Mac802_11::navHandler()
{
	if(is_idle() && mhBackoff_.paused())
		// change wrt Mike's code
		//mhBackoff_.resume(difs_);
		mhBackoff_.resume(phymib_.getDIFS());

}

void
Mac802_11::recvHandler()
{
	recv_timer();
}

void
Mac802_11::sendHandler()
{
	send_timer();
}


void
Mac802_11::txHandler()
{
	// change wrtt Mike's code
       if (EOTtarget_) {
               assert(eotPacket_);
               EOTtarget_->recv(eotPacket_, (Handler *) 0);
               eotPacket_ = NULL;
       }


	tx_active_ = 0;
}


/* ======================================================================
   The "real" Timer Handler Routines
   ====================================================================== */
void
Mac802_11::send_timer()
{
	switch(tx_state_) {
	/*
	 * Sent a RTS, but did not receive a CTS.
	 */
	case MAC_RTS:
		RetransmitRTS();
		break;
	/*
	 * Sent a CTS, but did not receive a DATA packet.
	 */
	case MAC_CTS:
		assert(pktCTRL_);
		Packet::free(pktCTRL_); pktCTRL_ = 0;
		break;
	/*
	 * Sent DATA, but did not receive an ACK packet.
	 */
	case MAC_SEND:
		RetransmitDATA();
		break;
	/*
	 * Sent an ACK, and now ready to resume transmission.
	 */
	case MAC_ACK:
		assert(pktCTRL_);
		Packet::free(pktCTRL_); pktCTRL_ = 0;
		break;
	case MAC_IDLE:
		break;
	default:
		assert(0);
	}
	tx_resume();
}


/* ======================================================================
   Outgoing Packet Routines
   ====================================================================== */
int
Mac802_11::check_pktCTRL()
{
	struct hdr_mac802_11 *mh;
	double timeout;

	if(pktCTRL_ == 0)
		return -1;
	if(tx_state_ == MAC_CTS || tx_state_ == MAC_ACK)
		return -1;

	mh = HDR_MAC802_11(pktCTRL_);
							  
	switch(mh->dh_fc.fc_subtype) {
	/*
	 *  If the medium is not IDLE, don't send the CTS.
	 */
	case MAC_Subtype_CTS:
		if(!is_idle()) {
			discard(pktCTRL_, DROP_MAC_BUSY); pktCTRL_ = 0;
			return 0;
		}
		// change wrt Mike's code
		//SET_TX_STATE(MAC_CTS);
		setTxState(MAC_CTS);

		
		/*
		 * timeout:  cts + data tx time calculated by
		 *           adding cts tx time to the cts duration
		 *           minus ack tx time -- this timeout is
		 *           a guess since it is unspecified
		 *           (note: mh->dh_duration == cf->cf_duration)
		 */
		// change wrt Mike's code
		/*timeout = txtime(ETHER_CTS_LEN, basicRate_)
			+ DSSS_MaxPropagationDelay			// XXX
			+ sec(mh->dh_duration)
			+ DSSS_MaxPropagationDelay			// XXX
			- sifs_
			- txtime(ETHER_ACK_LEN, basicRate_);*/
		
		 timeout = txtime(phymib_.getCTSlen(), basicRate_)
                        + DSSS_MaxPropagationDelay                      // XXX
                        + sec(mh->dh_duration)
                        + DSSS_MaxPropagationDelay                      // XXX
                       - phymib_.getSIFS()
                       - txtime(phymib_.getACKlen(), basicRate_);


		break;
		/*
		 * IEEE 802.11 specs, section 9.2.8
		 * Acknowledments are sent after an SIFS, without regard to
		 * the busy/idle state of the medium.
		 */
	case MAC_Subtype_ACK:
		
		// change wrt Mike's code
		/*SET_TX_STATE(MAC_ACK);
		timeout = txtime(ETHER_ACK_LEN, basicRate_);*/

		setTxState(MAC_ACK);
                timeout = txtime(phymib_.getACKlen(), basicRate_);
		
		break;
	default:
		fprintf(stderr, "check_pktCTRL:Invalid MAC Control subtype\n");
		exit(1);
	}
	// change wrt Mike's code 
//       TRANSMIT(pktCTRL_, timeout);
	transmit(pktCTRL_, timeout);


	return 0;
}

int
Mac802_11::check_pktRTS()
{
	struct hdr_mac802_11 *mh;
	double timeout;

	assert(mhBackoff_.busy() == 0);

	if(pktRTS_ == 0)
 		return -1;
	//struct hdr_cmn *ch = HDR_CMN(pktRTS_);
	mh = HDR_MAC802_11(pktRTS_);

 	switch(mh->dh_fc.fc_subtype) {
	case MAC_Subtype_RTS:
		if(! is_idle()) {
			inc_cw();
			mhBackoff_.start(cw_, is_idle());
			return 0;
		}
		// change wrt Mike's code
		/*SET_TX_STATE(MAC_RTS);
		timeout = txtime(ETHER_RTS_LEN, basicRate_)
			+ DSSS_MaxPropagationDelay			// XXX
			+ sifs_
			+ txtime(ETHER_CTS_LEN, basicRate_)
			+ DSSS_MaxPropagationDelay;			// XXX
		*/
		setTxState(MAC_RTS);
               timeout = txtime(phymib_.getRTSlen(), basicRate_)
                       + DSSS_MaxPropagationDelay                      // XXX
                       + phymib_.getSIFS()
                       + txtime(phymib_.getCTSlen(), basicRate_)
                       + DSSS_MaxPropagationDelay;
		break;
	default:
		fprintf(stderr, "check_pktRTS:Invalid MAC Control subtype\n");
		exit(1);
	}
	// change wrt Mike's code
	// TRANSMIT(pktRTS_, timeout);
	transmit(pktRTS_, timeout);
  

	return 0;
}

int
Mac802_11::check_pktTx()
{
	struct hdr_mac802_11 *mh;
	double timeout;
	
	assert(mhBackoff_.busy() == 0);

	if(pktTx_ == 0)
		return -1;

	mh = HDR_MAC802_11(pktTx_);
       	//int len = HDR_CMN(pktTx_)->size();


//-ak-----------
//	printf("chckpkt TX \n");


	switch(mh->dh_fc.fc_subtype) {
	case MAC_Subtype_Data:
		if(! is_idle()) {
			// change wrt Mike's code
			//sendRTS(ETHER_ADDR(mh->dh_da));
			sendRTS(ETHER_ADDR(mh->dh_ra));

			inc_cw();
			mhBackoff_.start(cw_, is_idle());
			return 0;
		}
		// change wrt Mike's code
		//SET_TX_STATE(MAC_SEND);
		/*if((u_int32_t)ETHER_ADDR(mh->dh_da) != MAC_BROADCAST)
			timeout = txtime(pktTx_)
				+ DSSS_MaxPropagationDelay		// XXX
				+ sifs_
				+ txtime(ETHER_ACK_LEN, basicRate_)
				+ DSSS_MaxPropagationDelay;		*/// XXX
		setTxState(MAC_SEND);
               if((u_int32_t)ETHER_ADDR(mh->dh_ra) != MAC_BROADCAST)
                        timeout = txtime(pktTx_)
                                + DSSS_MaxPropagationDelay              // XXX
                               + phymib_.getSIFS()
                               + txtime(phymib_.getACKlen(), basicRate_)
                               + DSSS_MaxPropagationDelay;             // XXX
		else
			timeout = txtime(pktTx_);
		break;
	default:
		fprintf(stderr, "check_pktTx:Invalid MAC Control subtype\n");
		//printf("pktRTS:%x, pktCTS/ACK:%x, pktTx:%x\n",pktRTS_, pktCTRL_,pktTx_);
		exit(1);
	}
	// change wrt Mike's code
        //TRANSMIT(pktTx_, timeout);
	transmit(pktTx_, timeout);
	return 0;
}
/*void
Mac802_11::sendSCANREQ(int dst)
{
        Packet *p = Packet::alloc();
        hdr_cmn* ch = HDR_CMN(p);
        struct probereq_frame *pbr = (struct probereq_frame*)p->access(hdr_mac::offset_);

        assert(pktTx_);
        assert(pktRTS_ == 0);


        ch->uid() = 0;
        ch->ptype() = PT_MAC;
        // change wrt Mike's code
        //ch->size() = ETHER_RTS_LEN;
        ch->size() = phymib_.getRTSlen();
        ch->iface() = -2;
        ch->error() = 0;

        bzero(srf, MAC_HDR_LEN);

        pbr->preq_fc.fc_protocol_version = MAC_ProtocolVersion;
        pbr->preq_fc.fc_type       = MAC_Type_Management;
        pbr->preq_fc.fc_subtype    = MAC_Subtype_ProbeReq;
        pbr->preq_fc.fc_to_ds      = 0;
        pbr->preq_fc.fc_from_ds    = 0;
        pbr->preq_fc.fc_more_frag  = 0;
        pbr->preq_fc.fc_retry      = 0;
        pbr->preq_fc.fc_pwr_mgt    = 0;
	
	//rf->rf_duration = RTS_DURATION(pktTx_);
        STORE4BYTE(&dst, (srf->rf_ra));
*/
       /*  store rts tx time */
  

/*      ch->txtime() = txtime(ch->size(), basicRate_);

        STORE4BYTE(&index_, (srf->rf_ta));*/
        /* calculate rts duration field */

        // change wrt Mike's code
        /*
        pbr->rf_duration = usec(sifs_
                               + txtime(ETHER_CTS_LEN, basicRate_)
                               + sifs_
                               + txtime(pktTx_)
                               + sifs_
                               + txtime(ETHER_ACK_LEN, basicRate_));
        */
/*
           pbr->preq_duration = usec(phymib_.getSIFS()
                              + txtime(phymib_.getCTSlen(), basicRate_)
                              + phymib_.getSIFS()
                               + txtime(pktTx_)
                              + phymib_.getSIFS()
                              + txtime(phymib_.getACKlen(), basicRate_));

        pktRTS_ = p;

}

/*
 * Low-level transmit functions that actually place the packet onto
 * the channel.
 */
void
Mac802_11::sendRTS(int dst)
{
	Packet *p = Packet::alloc();
	hdr_cmn* ch = HDR_CMN(p);
	struct rts_frame *rf = (struct rts_frame*)p->access(hdr_mac::offset_);
	
	assert(pktTx_);
	assert(pktRTS_ == 0);

	/*
	 *  If the size of the packet is larger than the
	 *  RTSThreshold, then perform the RTS/CTS exchange.
	 *
	 *  XXX: also skip if destination is a broadcast
	 */
	// change wrt Mike's code
	/*if( (u_int32_t) HDR_CMN(pktTx_)->size() < macmib_->RTSThreshold ||
	    (u_int32_t) dst == MAC_BROADCAST) {*/
	if( (u_int32_t) HDR_CMN(pktTx_)->size() < macmib_.getRTSThreshold() ||
            (u_int32_t) dst == MAC_BROADCAST) {
		Packet::free(p);
		//p = 0;
		return;
	}

	ch->uid() = 0;
	ch->ptype() = PT_MAC;
	// change wrt Mike's code
	//ch->size() = ETHER_RTS_LEN;
	ch->size() = phymib_.getRTSlen();
	ch->iface() = -2;
	ch->error() = 0;

	bzero(rf, MAC_HDR_LEN);

	rf->rf_fc.fc_protocol_version = MAC_ProtocolVersion;
 	rf->rf_fc.fc_type	= MAC_Type_Control;
 	rf->rf_fc.fc_subtype	= MAC_Subtype_RTS;
 	rf->rf_fc.fc_to_ds	= 0;
 	rf->rf_fc.fc_from_ds	= 0;
 	rf->rf_fc.fc_more_frag	= 0;
 	rf->rf_fc.fc_retry	= 0;
 	rf->rf_fc.fc_pwr_mgt	= 0;
 	rf->rf_fc.fc_more_data	= 0;
 	rf->rf_fc.fc_wep	= 0;
 	rf->rf_fc.fc_order	= 0;

	//rf->rf_duration = RTS_DURATION(pktTx_);
	STORE4BYTE(&dst, (rf->rf_ra));
	
	/* store rts tx time */
 	ch->txtime() = txtime(ch->size(), basicRate_);
	
	STORE4BYTE(&index_, (rf->rf_ta));
	
	


	/* calculate rts duration field */
	
	// change wrt Mike's code
	/*
	rf->rf_duration = usec(sifs_
			       + txtime(ETHER_CTS_LEN, basicRate_)
			       + sifs_
			       + txtime(pktTx_)
			       + sifs_
			       + txtime(ETHER_ACK_LEN, basicRate_));
	*/
	   rf->rf_duration = usec(phymib_.getSIFS()
                              + txtime(phymib_.getCTSlen(), basicRate_)
                              + phymib_.getSIFS()
                               + txtime(pktTx_)
                              + phymib_.getSIFS()
                              + txtime(phymib_.getACKlen(), basicRate_));

	pktRTS_ = p;
}

void
Mac802_11::sendCTS(int dst, double rts_duration)
{
	Packet *p = Packet::alloc();
	hdr_cmn* ch = HDR_CMN(p);
	struct cts_frame *cf = (struct cts_frame*)p->access(hdr_mac::offset_);

	assert(pktCTRL_ == 0);

	ch->uid() = 0;
	ch->ptype() = PT_MAC;
	// change wrt Mike's code
	//ch->size() = ETHER_CTS_LEN;
	ch->size() = phymib_.getCTSlen();


	ch->iface() = -2;
	ch->error() = 0;
	//ch->direction() = hdr_cmn::DOWN;
	bzero(cf, MAC_HDR_LEN);

	cf->cf_fc.fc_protocol_version = MAC_ProtocolVersion;
	cf->cf_fc.fc_type	= MAC_Type_Control;
	cf->cf_fc.fc_subtype	= MAC_Subtype_CTS;
 	cf->cf_fc.fc_to_ds	= 0;
 	cf->cf_fc.fc_from_ds	= 0;
 	cf->cf_fc.fc_more_frag	= 0;
 	cf->cf_fc.fc_retry	= 0;
 	cf->cf_fc.fc_pwr_mgt	= 0;
 	cf->cf_fc.fc_more_data	= 0;
 	cf->cf_fc.fc_wep	= 0;
 	cf->cf_fc.fc_order	= 0;
	
	//cf->cf_duration = CTS_DURATION(rts_duration);
	STORE4BYTE(&dst, (cf->cf_ra));
	
	/* store cts tx time */
	ch->txtime() = txtime(ch->size(), basicRate_);
	
	/* calculate cts duration */
	// change wrt Mike's code
	/*cf->cf_duration = usec(sec(rts_duration)
			       - sifs_
			       - txtime(ETHER_CTS_LEN, basicRate_));*/
	cf->cf_duration = usec(sec(rts_duration)
                              - phymib_.getSIFS()
                              - txtime(phymib_.getCTSlen(), basicRate_));


	
	pktCTRL_ = p;
	
}

void
Mac802_11::sendACK(int dst)
{
	Packet *p = Packet::alloc();
	hdr_cmn* ch = HDR_CMN(p);
	struct ack_frame *af = (struct ack_frame*)p->access(hdr_mac::offset_);

	assert(pktCTRL_ == 0);

	ch->uid() = 0;
	ch->ptype() = PT_MAC;
	// CHANGE WRT Mike's code
	//ch->size() = ETHER_ACK_LEN;
	ch->size() = phymib_.getACKlen();
	ch->iface() = -2;
	ch->error() = 0;
	
	bzero(af, MAC_HDR_LEN);

	af->af_fc.fc_protocol_version = MAC_ProtocolVersion;
 	af->af_fc.fc_type	= MAC_Type_Control;
 	af->af_fc.fc_subtype	= MAC_Subtype_ACK;
 	af->af_fc.fc_to_ds	= 0;
 	af->af_fc.fc_from_ds	= 0;
 	af->af_fc.fc_more_frag	= 0;
 	af->af_fc.fc_retry	= 0;
 	af->af_fc.fc_pwr_mgt	= 0;
 	af->af_fc.fc_more_data	= 0;
 	af->af_fc.fc_wep	= 0;
 	af->af_fc.fc_order	= 0;

	//af->af_duration = ACK_DURATION();
	STORE4BYTE(&dst, (af->af_ra));

	/* store ack tx time */
 	ch->txtime() = txtime(ch->size(), basicRate_);
	
	/* calculate ack duration */
 	af->af_duration = 0;	
	
	pktCTRL_ = p;
}

void
Mac802_11::sendDATA(Packet *p)
{
	hdr_cmn* ch = HDR_CMN(p);
	struct hdr_mac802_11* dh = HDR_MAC802_11(p);

	assert(pktTx_ == 0);

	/*
	 * Update the MAC header
	 */
	// change wrt Mike's code
	//ch->size() += ETHER_HDR_LEN11;
	ch->size() += phymib_.getHdrLen11();

	dh->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	dh->dh_fc.fc_type       = MAC_Type_Data;
	dh->dh_fc.fc_subtype    = MAC_Subtype_Data;
	//printf(".....p = %x, mac-subtype-%d\n",p,dh->dh_fc.fc_subtype);
	
	dh->dh_fc.fc_to_ds      = 0;
	dh->dh_fc.fc_from_ds    = 0;
	dh->dh_fc.fc_more_frag  = 0;
	dh->dh_fc.fc_retry      = 0;
	dh->dh_fc.fc_pwr_mgt    = 0;
	dh->dh_fc.fc_more_data  = 0;
	dh->dh_fc.fc_wep        = 0;
	dh->dh_fc.fc_order      = 0;

	/* store data tx time */
 	ch->txtime() = txtime(ch->size(), dataRate_);

// change wrt Mike's code
//	if((u_int32_t)ETHER_ADDR(dh->dh_da) != MAC_BROADCAST) {

	if((u_int32_t)ETHER_ADDR(dh->dh_ra) != MAC_BROADCAST) {
		/* store data tx time for unicast packets */
		ch->txtime() = txtime(ch->size(), dataRate_);
		
		//dh->dh_duration = DATA_DURATION();
		// chnage wrt Mike's code
		//dh->dh_duration = usec(txtime(ETHER_ACK_LEN, basicRate_)
 				       //+ sifs_);
		dh->dh_duration = usec(txtime(phymib_.getACKlen(), basicRate_)
                                      + phymib_.getSIFS());



	} else {
		/* store data tx time for broadcast packets (see 9.6) */
		ch->txtime() = txtime(ch->size(), basicRate_);
		
		dh->dh_duration = 0;
	}
	pktTx_ = p;
}

/* ======================================================================
   Retransmission Routines
   ====================================================================== */
void
Mac802_11::RetransmitRTS()
{
	assert(pktTx_);
	assert(pktRTS_);
	assert(mhBackoff_.busy() == 0);
// change wrt Mike's code
//	macmib_->RTSFailureCount++;
	macmib_.RTSFailureCount++;


	ssrc_ += 1;			// STA Short Retry Count
// change wrt Mike's code
//	if(ssrc_ >= macmib_->ShortRetryLimit) {
		
	if(ssrc_ >= macmib_.getShortRetryLimit()) {
		discard(pktRTS_, DROP_MAC_RETRY_COUNT_EXCEEDED); pktRTS_ = 0;
		/* tell the callback the send operation failed 
		   before discarding the packet */
		hdr_cmn *ch = HDR_CMN(pktTx_);
		if (ch->xmit_failure_) {
                        /*
                         *  Need to remove the MAC header so that 
                         *  re-cycled packets don't keep getting
                         *  bigger.
                         */
			// change wrt Mike's code
                        //ch->size() -= ETHER_HDR_LEN11;
			ch->size() -= phymib_.getHdrLen11();
                        ch->xmit_reason_ = XMIT_REASON_RTS;
                        ch->xmit_failure_(pktTx_->copy(),
                                          ch->xmit_failure_data_);
                }
		//printf("(%d)....discarding RTS:%x\n",index_,pktRTS_);
		discard(pktTx_, DROP_MAC_RETRY_COUNT_EXCEEDED); pktTx_ = 0;
		ssrc_ = 0;
		rst_cw();
	} else {
		//printf("(%d)...retxing RTS:%x\n",index_,pktRTS_);
		struct rts_frame *rf;
		rf = (struct rts_frame*)pktRTS_->access(hdr_mac::offset_);
		rf->rf_fc.fc_retry = 1;

		inc_cw();
		mhBackoff_.start(cw_, is_idle());
	}
}

void
Mac802_11::RetransmitDATA()
{
	struct hdr_cmn *ch;
	struct hdr_mac802_11 *mh;
// change wrt Mike's code
//	u_int32_t *rcount, *thresh;
	u_int32_t *rcount, thresh;
	assert(mhBackoff_.busy() == 0);

	assert(pktTx_);
	assert(pktRTS_ == 0);

	ch = HDR_CMN(pktTx_);
	mh = HDR_MAC802_11(pktTx_);

	/*
	 *  Broadcast packets don't get ACKed and therefore
	 *  are never retransmitted.
	 */
// change wrt Mike's code
//	if((u_int32_t)ETHER_ADDR(mh->dh_da) == MAC_BROADCAST) {
		
	if((u_int32_t)ETHER_ADDR(mh->dh_ra) == MAC_BROADCAST) {
		Packet::free(pktTx_); pktTx_ = 0;

		/*
		 * Backoff at end of TX.
		 */
		rst_cw();
		mhBackoff_.start(cw_, is_idle());

		return;
	}

// change wrt Mike's code
//	macmib_->ACKFailureCount++;
	macmib_.ACKFailureCount++;

	// chnage wrt Mike's code
	/*
	if((u_int32_t) ch->size() <= macmib_->RTSThreshold) {
		rcount = &ssrc_;
		thresh = &macmib_->ShortRetryLimit;
	}
	else {
		rcount = &slrc_;
		thresh = &macmib_->LongRetryLimit;
	}*/

	if((u_int32_t) ch->size() <= macmib_.getRTSThreshold()) {
                rcount = &ssrc_;
               thresh = macmib_.getShortRetryLimit();
        }
        else {
                rcount = &slrc_;
               thresh = macmib_.getLongRetryLimit();
        }




	(*rcount)++;


// change wrt Mike's code
	//if(*rcount > *thresh) {
	//	macmib_->FailedCount++;
	if(*rcount > thresh) {
               macmib_.FailedCount++;
		/* tell the callback the send operation failed 
		   before discarding the packet */
		hdr_cmn *ch = HDR_CMN(pktTx_);
		if (ch->xmit_failure_) {
// change wrt Mike's code
                        //ch->size() -= ETHER_HDR_LEN11;
                        ch->size() -= phymib_.getHdrLen11();
			ch->xmit_reason_ = XMIT_REASON_ACK;
                        ch->xmit_failure_(pktTx_->copy(),
                                          ch->xmit_failure_data_);
                }

		discard(pktTx_, DROP_MAC_RETRY_COUNT_EXCEEDED); pktTx_ = 0;
		//printf("(%d)DATA discarded: count exceeded\n",index_);
		*rcount = 0;
		rst_cw();
	}
	else {
		struct hdr_mac802_11 *dh;
		dh = HDR_MAC802_11(pktTx_);
		dh->dh_fc.fc_retry = 1;


// change wrt Mike's code
		//sendRTS(ETHER_ADDR(mh->dh_da));
		sendRTS(ETHER_ADDR(mh->dh_ra));
		//printf("(%d)retxing data:%x..sendRTS..\n",index_,pktTx_);
		inc_cw();
		mhBackoff_.start(cw_, is_idle());
	}
}

/* ======================================================================
   Incoming Packet Routines
   ====================================================================== */
void
Mac802_11::send(Packet *p, Handler *h)
{
	double rTime;
	struct hdr_mac802_11* dh = HDR_MAC802_11(p);

	/* 
	 * drop the packet if the node is in sleep mode
	 XXX sleep mode can't stop node from sending packets
	 */
	EnergyModel *em = netif_->node()->energy_model();
	if (em && em->sleep()) {
		em->set_node_sleep(0);
		em->set_node_state(EnergyModel::INROUTE);
	}
	
	callback_ = h;
	sendDATA(p);
// change wrt Mike's code
	//sendRTS(ETHER_ADDR(dh->dh_da));
	sendRTS(ETHER_ADDR(dh->dh_ra));



	//----ak---------
	//printf("bssid in send function %d\n",bss_id());

	/*
	 * Assign the data packet a sequence number.
	 */
	dh->dh_scontrol = sta_seqno_++;

	/*
	 *  If the medium is IDLE, we must wait for a DIFS
	 *  Space before transmitting.
	 */
	if(mhBackoff_.busy() == 0) {
		if(is_idle()) {
			/*
			 * If we are already deferring, there is no
			 * need to reset the Defer timer.
			 */
			if(mhDefer_.busy() == 0) {
// change wrt Mike's code
			/*	rTime = (Random::random() % cw_) * (phymib_->SlotTime);
				mhDefer_.start(difs_ + rTime);*/
			rTime = (Random::random() % cw_)
                                       * (phymib_.getSlotTime());
                               mhDefer_.start(phymib_.getDIFS() + rTime);


			}
			
		}
	/*
	 * If the medium is NOT IDLE, then we start
	 * the backoff timer.
	 */
		else {
			mhBackoff_.start(cw_, is_idle());
		}
	}
}

void
Mac802_11::recv(Packet *p, Handler *h)
{
	struct hdr_cmn *hdr = HDR_CMN(p);
	/*
	 * Sanity Check
	 */
	assert(initialized());

	/*
	 *  Handle outgoing packets.
	 */
	if(hdr->direction() == hdr_cmn::DOWN) {
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

	/*
	 *  If the interface is currently in transmit mode, then
	 *  it probably won't even see this packet.  However, the
	 *  "air" around me is BUSY so I need to let the packet
	 *  proceed.  Just set the error flag in the common header
	 *  to that the packet gets thrown away.
	 */
	if(tx_active_ && hdr->error() == 0) {
		hdr->error() = 1;
	}

	if(rx_state_ == MAC_IDLE) {
// change wrt Mike's code	
	//SET_RX_STATE(MAC_RECV);
	setRxState(MAC_RECV);


		pktRx_ = p;

		/*
		 * Schedule the reception of this packet, in
		 * txtime seconds.
		 */
		mhRecv_.start(txtime(p));
	} else {
		/*
		 *  If the power of the incoming packet is smaller than the
		 *  power of the packet currently being received by at least
                 *  the capture threshold, then we ignore the new packet.
		 */
		if(pktRx_->txinfo_.RxPr / p->txinfo_.RxPr >= p->txinfo_.CPThresh) {
			capture(p);
		} else {
			collision(p);
		}
	}
}

void
Mac802_11::recv_timer()
{
	u_int32_t src; 
	hdr_cmn *ch = HDR_CMN(pktRx_);
	hdr_mac802_11 *mh = HDR_MAC802_11(pktRx_);
// chnage wrt Mike's code
//	u_int32_t dst = ETHER_ADDR(mh->dh_da);
	 u_int32_t dst = ETHER_ADDR(mh->dh_ra);

	// XXX debug
	//struct cts_frame *cf = (struct cts_frame*)pktRx_->access(hdr_mac::offset_);
	//u_int32_t src = ETHER_ADDR(mh->dh_ta);
	
	u_int8_t  type = mh->dh_fc.fc_type;
	u_int8_t  subtype = mh->dh_fc.fc_subtype;

	assert(pktRx_);
	assert(rx_state_ == MAC_RECV || rx_state_ == MAC_COLL);
	
        /*
         *  If the interface is in TRANSMIT mode when this packet
         *  "arrives", then I would never have seen it and should
         *  do a silent discard without adjusting the NAV.
         */
        if(tx_active_) {
                Packet::free(pktRx_);
                goto done;
        }

	/*
	 * Handle collisions.
	 */
	if(rx_state_ == MAC_COLL) {
		discard(pktRx_, DROP_MAC_COLLISION);
// change wrt Mike's code		
		//set_nav(usec(eifs_));
		
		set_nav(usec(phymib_.getEIFS()));
		goto done;
	}

	/*
	 * Check to see if this packet was received with enough
	 * bit errors that the current level of FEC still could not
	 * fix all of the problems - ie; after FEC, the checksum still
	 * failed.
	 */
	if( ch->error() ) {
		Packet::free(pktRx_);
// change wrt Mike's code
//		set_nav(usec(eifs_));
		set_nav(usec(phymib_.getEIFS()));
		goto done;
	}

	/*
	 * IEEE 802.11 specs, section 9.2.5.6
	 *	- update the NAV (Network Allocation Vector)
	 */
	if(dst != (u_int32_t)index_) {
		set_nav(mh->dh_duration);
	}

        /* tap out - */
        if (tap_ && type == MAC_Type_Data &&
            MAC_Subtype_Data == subtype ) 
		tap_->tap(pktRx_);
	/*
	 * Adaptive Fidelity Algorithm Support - neighborhood infomation 
	 * collection
	 *
	 * Hacking: Before filter the packet, log the neighbor node
	 * I can hear the packet, the src is my neighbor
	 */
	if (netif_->node()->energy_model() && 
	    netif_->node()->energy_model()->adaptivefidelity()) {
// change wrt Mike's code
		//src = ETHER_ADDR(mh->dh_sa);
		src = ETHER_ADDR(mh->dh_ta);
		netif_->node()->energy_model()->add_neighbor(src);
	}
	/*
	 * Address Filtering
	 */
	if(dst != (u_int32_t)index_ && dst != MAC_BROADCAST) {
		/*
		 *  We don't want to log this event, so we just free
		 *  the packet instead of calling the drop routine.
		 */
		discard(pktRx_, "---");
		goto done;
	}

	switch(type) {

	case MAC_Type_Management:
		
		//-ak-----
		discard(pktRx_, DROP_MAC_PACKET_ERROR);
		goto done;
		/*switch(subtype)
		case MAC_Subtype_ProbeReq;
			recvPREQ(pktRx_);
			break;
		case MAC_Subtype_ProbeRes;
			recvPRES(pktRx_);
			break;*/

	case MAC_Type_Control:
		switch(subtype) {
		case MAC_Subtype_RTS:
			recvRTS(pktRx_);
			break;
		case MAC_Subtype_CTS:
			recvCTS(pktRx_);
			break;
		case MAC_Subtype_ACK:
			recvACK(pktRx_);
			break;
		default:
			fprintf(stderr,"recvTimer1:Invalid MAC Control Subtype %x\n",
				subtype);
			exit(1);
		}
		break;
	case MAC_Type_Data:
		switch(subtype) {
		case MAC_Subtype_Data:
			recvDATA(pktRx_);
			break;
		default:
			fprintf(stderr, "recv_timer2:Invalid MAC Data Subtype %x\n",
				subtype);
			exit(1);
		}
		break;
	default:
		fprintf(stderr, "recv_timer3:Invalid MAC Type %x\n", subtype);
		exit(1);
	}
 done:
	pktRx_ = 0;
	rx_resume();
}


void
Mac802_11::recvRTS(Packet *p)
{
	struct rts_frame *rf = (struct rts_frame*)p->access(hdr_mac::offset_);

	if(tx_state_ != MAC_IDLE) {
		discard(p, DROP_MAC_BUSY);
		return;
	}

	/*
	 *  If I'm responding to someone else, discard this RTS.
	 */
	if(pktCTRL_) {
		discard(p, DROP_MAC_BUSY);
		return;
	}

	sendCTS(ETHER_ADDR(rf->rf_ta), rf->rf_duration);

	/*
	 *  Stop deferring - will be reset in tx_resume().
	 */
	if(mhDefer_.busy()) mhDefer_.stop();

	tx_resume();

	mac_log(p);
}

/*
 * txtime()	- pluck the precomputed tx time from the packet header
 */
double
Mac802_11::txtime(Packet *p)
 {
	 struct hdr_cmn *ch = HDR_CMN(p);
	 double t = ch->txtime();
	 if (t < 0.0) {
		 drop(p, "XXX");
 		exit(1);
	 }
	 return t;
 }

 
/*
 * txtime()	- calculate tx time for packet of size "psz" bytes 
 *		  at rate "drt" bps
 */
double
Mac802_11::txtime(double psz, double drt)
{

// change wrt Mike's code
//	double dsz = psz - PLCP_HDR_LEN;
//	int plcp_hdr = PLCP_HDR_LEN << 3;
	double dsz = psz - phymib_.getPLCPhdrLen();
        int plcp_hdr = phymib_.getPLCPhdrLen() << 3;
	
	int datalen = (int)dsz << 3;
// change wrt Mike's code	
//	double t = (((double)plcp_hdr)/phymib_->PLCPDataRate) + (((double)datalen)/drt);
	double t = (((double)plcp_hdr)/phymib_.getPLCPDataRate())
                                       + (((double)datalen)/drt);


	return(t);
}



void
Mac802_11::recvCTS(Packet *p)
{
	if(tx_state_ != MAC_RTS) {
		discard(p, DROP_MAC_INVALID_STATE);
		return;
	}

	assert(pktRTS_);
	Packet::free(pktRTS_); pktRTS_ = 0;

	assert(pktTx_);
	// debug
	//struct hdr_mac802_11 *mh = HDR_MAC802_11(pktTx_);
	//printf("(%d):recvCTS:pktTx_-%x,mac-subtype-%d & pktCTS_:%x\n",index_,pktTx_,mh->dh_fc.fc_subtype,p);
	
	mhSend_.stop();

	/*
	 * The successful reception of this CTS packet implies
	 * that our RTS was successful.  Hence, we can reset
	 * the Short Retry Count and the CW.
	 */
	//ssrc_ = 0;
	//rst_cw();

	tx_resume();

	mac_log(p);
}

void
Mac802_11::recvDATA(Packet *p)
{
	struct hdr_mac802_11 *dh = HDR_MAC802_11(p);
	u_int32_t dst, src, size;

	{	struct hdr_cmn *ch = HDR_CMN(p);

// chnage wrt Mike's code
		//dst = ETHER_ADDR(dh->dh_da);
		//src = ETHER_ADDR(dh->dh_sa);
		 dst = ETHER_ADDR(dh->dh_ra);
                 src = ETHER_ADDR(dh->dh_ta);


		size = ch->size();

		/*
		 * Adjust the MAC packet size - ie; strip
		 * off the mac header
		 */
// change wrt Mike's code	
//	ch->size() -= ETHER_HDR_LEN11;
		 ch->size() -= phymib_.getHdrLen11();
		ch->num_forwards() += 1;
	}

	/*
	 *  If we sent a CTS, clean up...
	 */
	if(dst != MAC_BROADCAST) {

// change wrt Mike's code
//		if(size >= macmib_->RTSThreshold) {
		if(size >= macmib_.getRTSThreshold()) {
			if (tx_state_ == MAC_CTS) {
				assert(pktCTRL_);
				Packet::free(pktCTRL_); pktCTRL_ = 0;
				mhSend_.stop();
				/*
				 * Our CTS got through.
				 */
				//printf("(%d): RECVING DATA!\n",index_);
				//ssrc_ = 0;
				//rst_cw();
			}
			else {
				discard(p, DROP_MAC_BUSY);
				//printf("(%d)..discard DATA\n",index_);
				return;
			}
			sendACK(src);
			tx_resume();
		}
		/*
		 *  We did not send a CTS and there's no
		 *  room to buffer an ACK.
		 */
		else {
			if(pktCTRL_) {
				discard(p, DROP_MAC_BUSY);
				return;
			}
			sendACK(src);
			if(mhSend_.busy() == 0)
				tx_resume();
		}
	}

	/* ============================================================
	    Make/update an entry in our sequence number cache.
	   ============================================================ */

	/* Changed by Debojyoti Dutta. This upper loop of if{}else was 
	   suggested by Joerg Diederich <dieder@ibr.cs.tu-bs.de>. 
	   Changed on 19th Oct'2000 */

        if(dst != MAC_BROADCAST) {
                if (src < (u_int32_t) cache_node_count_) {
                        Host *h = &cache_[src];

                        if(h->seqno && h->seqno == dh->dh_scontrol) {
                                discard(p, DROP_MAC_DUPLICATE);
                                return;
                        }
                        h->seqno = dh->dh_scontrol;
                } else {
			static int count = 0;
			if (++count <= 10) {
				printf ("MAC_802_11: accessing MAC cache_ array out of range (src %u, dst %u, size %d)!\n", src, dst, cache_node_count_);
				if (count == 10)
					printf ("[suppressing additional MAC cache_ warnings]\n");
			};
		};
	}

	/*

	 *  Pass the packet up to the link-layer.
	 *  XXX - we could schedule an event to account
	 *  for this processing delay.
	*/
	
	/* in BSS mode, if a station receives a packet via
!        * the AP, and higher layers are interested in looking
!        * at the src address, we might need to put it at
!        * the right place - lest the higher layers end up
!        * believing the AP address to be the src addr! a quick
!        * grep didn't turn up any higher layers interested in
!        * the src addr though!
!        * anyway, here if I'm the AP and the destination
!        * address (in dh_3a) isn't me, then we have to fwd
!        * the packet; we pick the real destination and set
!        * set it up for the LL; we save the real src into
!        * the dh_3a field for the 'interested in the info'
!        * receiver; we finally push the packet towards the
!        * LL to be added back to my queue - accomplish this
!        * by reversing the direction!*/

	if ((bss_id() == addr()) && ((u_int32_t)ETHER_ADDR(dh->dh_ra)!= MAC_BROADCAST)&& ((u_int32_t)ETHER_ADDR(dh->dh_3a) != addr()))
       {
               struct hdr_cmn *ch = HDR_CMN(p);
               u_int32_t dst = ETHER_ADDR(dh->dh_3a);
               u_int32_t src = ETHER_ADDR(dh->dh_ta);
		/* if it is a broadcast pkt then send a copy up
+                * my stack also
+                */
               if (dst == MAC_BROADCAST) {
                       uptarget_->recv(p->copy(), (Handler*) 0);
               }

               ch->next_hop() = dst;
               STORE4BYTE(&src, (dh->dh_3a));
               ch->addr_type() = NS_AF_ILINK;
               ch->direction() = hdr_cmn::DOWN;
       }


	//p->incoming = 1;
	// XXXXX NOTE: use of incoming flag has been depracated; In order to track direction of pkt flow, direction_ in hdr_cmn is used instead. see packet.h for details. 
	
	uptarget_->recv(p, (Handler*) 0);
}


void
Mac802_11::recvACK(Packet *p)
{
	
	struct hdr_cmn *ch = HDR_CMN(p);

	if(tx_state_ != MAC_SEND) {
	discard(p, DROP_MAC_INVALID_STATE);
	return;
	}
	//printf("(%d)...................recving ACK:%x\n",index_,p);
	assert(pktTx_);
	Packet::free(pktTx_); pktTx_ = 0;

	mhSend_.stop();

	/*
	 * The successful reception of this ACK packet implies
	 * that our DATA transmission was successful.  Hence,
	 * we can reset the Short/Long Retry Count and the CW.
	 */
// chnage wrt Mike's code
	//if((u_int32_t) ch->size() <= macmib_->RTSThreshold)
	if((u_int32_t) ch->size() <= macmib_.getRTSThreshold())
		ssrc_ = 0;
	else
		slrc_ = 0;

	/*
	 * Backoff before sending again.
	 */
	rst_cw();
	assert(mhBackoff_.busy() == 0);
	mhBackoff_.start(cw_, is_idle());

	tx_resume();

	mac_log(p);
}

#include <delay.h>
#include <connector.h>
#include <packet.h>
#include <random.h>

// #define DEBUG

//#include <debug.h>
#include <arp.h>
#include <new-ll.h> 
#include <new-mac.h>
#include <mac-timers.h>
#include <new-mac-802_11.h>
#include <cmu-trace.h>


/* ======================================================================
   Macros
   ====================================================================== */
#define CHECK_BACKOFF_TIMER()						\
{									\
	if(is_idle() && mhBackoff_.paused())				\
		mhBackoff_.resume(difs_);					\
	if(! is_idle() && mhBackoff_.busy() && ! mhBackoff_.paused())	\
		mhBackoff_.pause();					\
}

#define TRANSMIT(p, t)                                                  \
{                                                                       \
	tx_active_ = 1;                                                  \
                                                                        \
        /*                                                              \
         * If I'm transmitting without doing CS, such as when           \
         * sending an ACK, any incoming packet will be "missed"         \
         * and hence, must be discarded.                                \
         */                                                             \
        if(rx_state_ != newMAC_IDLE) {                                      \
                struct hdr_mac802_11 *dh = HDR_MAC(p);                  \
                                                                        \
                assert(dh->dh_fc.fc_type == MAC_Type_Control);          \
                assert(dh->dh_fc.fc_subtype == MAC_Subtype_ACK);        \
                                                                        \
                assert(pktRx_);                                          \
                struct hdr_cmn *ch = HDR_CMN(pktRx_);                    \
                                                                        \
                ch->error() = 1;        /* force packet discard */      \
        }                                                               \
                                                                        \
        /*                                                              \
         * pass the packet on the "interface" which will in turn        \
         * place the packet on the channel.                             \
         *                                                              \
         * NOTE: a handler is passed along so that the Network          \
         *       Interface can distinguish between incoming and         \
         *       outgoing packets.                                      \
         */                                                             \
										downtarget_->recv(p, this);                                     \
                                                                        \
        mhSend_.start(t);                                                \
                                                                        \
	mhIF_.start(TX_Time(p));                                         \
}

#define SET_RX_STATE(x)			\
{					\
	rx_state_ = (x);			\
					\
	CHECK_BACKOFF_TIMER();		\
}

#define SET_TX_STATE(x)				\
{						\
	tx_state_ = (x);				\
						\
	CHECK_BACKOFF_TIMER();			\
}


/* ======================================================================
   Global Variables
   ====================================================================== */
extern char* pt_names[];

static PHY_MIB PMIB =
{
	DSSS_CWMin, DSSS_CWMax, DSSS_SlotTime, DSSS_CCATime,
	DSSS_RxTxTurnaroundTime, DSSS_SIFSTime, DSSS_PreambleLength,
	DSSS_PLCPHeaderLength
};	

static MAC_MIB MMIB =
{
	0 /* newMAC_RTSThreshold */, MAC_ShortRetryLimit,
	MAC_LongRetryLimit, MAC_FragmentationThreshold,
	MAC_MaxTransmitMSDULifetime, MAC_MaxReceiveLifetime,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};



/* ======================================================================
   TCL Hooks for the simulator
   ====================================================================== */
static class newMac802_11Class : public TclClass {
public:
	newMac802_11Class() : TclClass("newMac/new802_11") {}
	TclObject* create(int, const char*const*) {
		return (new newMac802_11(&PMIB, &MMIB));
	}
} class_mac802_11;


/* ======================================================================
   Mac Class Functions
   ====================================================================== */
newMac802_11::newMac802_11(PHY_MIB *p, MAC_MIB *m) : newMac(),
	mhIF_(this), mhNav_(this), mhRecv_(this), mhSend_(this),
	mhDefer_(this, p->SlotTime), mhBackoff_(this, p->SlotTime)
{
	macmib_ = m;
	phymib_ = p;

	nav_ = 0.0;

	tx_state_ = rx_state_ = newMAC_IDLE;
	tx_active_ = 0;

	pktRTS_ = 0;
	pktCTRL_ = 0;

	cw_ = phymib_->CWMin;
	ssrc_ = slrc_ = 0;

	sifs_ = phymib_->SIFSTime;
	pifs_ = sifs_ + phymib_->SlotTime;
	difs_ = sifs_ + 2*phymib_->SlotTime;
	eifs_ = sifs_ + difs_ + DATA_Time(ETHER_ACK_LEN +
		phymib_->PreambleLength/8 + phymib_->PLCPHeaderLength/8);
	tx_sifs_ = sifs_ - phymib_->RxTxTurnaroundTime;
	tx_pifs_ = tx_sifs_ + phymib_->SlotTime;
	tx_difs_ = tx_sifs_ + 2 * phymib_->SlotTime;

	sta_seqno_ = 1;
	cache_ = 0;
	cache_node_count_ = 0;
}
	

int
newMac802_11::command(int argc, const char*const* argv)
{
	if (argc == 3) {

		if (strcmp(argv[1], "log-target") == 0) {
			logtarget_ = (NsObject*) TclObject::lookup(argv[2]);
			if(logtarget_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		}

		if(strcmp(argv[1], "nodes") == 0) {
			if(cache_) return TCL_ERROR;
			cache_node_count_ = atoi(argv[2]);
			cache_ = new Host[cache_node_count_ + 1];
			assert(cache_);
			bzero(cache_, sizeof(Host) * (cache_node_count_+1 ));
			return TCL_OK;
			
		}
	}
	return newMac::command(argc, argv);
}


/* ======================================================================
   Debugging Routines
   ====================================================================== */
void
newMac802_11::trace_pkt(Packet *p) {
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_mac802_11* dh = HDR_MAC(p);
	u_int16_t *t = (u_int16_t*) &dh->dh_fc;

	fprintf(stderr, "\t[ %2x %2x %2x %2x ] %x %s %d\n",
		*t, dh->dh_duration,
		ETHER_ADDR(dh->dh_da), ETHER_ADDR(dh->dh_sa),
		index_, pt_names[ch->ptype()], ch->size());
}

void
newMac802_11::dump(char *fname)
{
	fprintf(stderr,
		"\n%s --- (INDEX: %d, time: %2.9f)\n",
		fname, index_, Scheduler::instance().clock());

	fprintf(stderr,
		"\ttx_state_: %x, rx_state_: %x, nav: %2.9f, idle: %d\n",
		tx_state_, rx_state_, nav_, is_idle());

	fprintf(stderr,
		"\tpktTx_: %x, pktRx_: %x, pktRTS_: %x, pktCTRL_: %x, upcall: %x\n",
		(int) pktTx_, (int) pktRx_, (int) pktRTS_,
		(int) pktCTRL_, (int) upcall_);

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
newMac802_11::hdr_dst(char* hdr, u_int32_t dst)
{
	struct hdr_mac802_11 *dh = (struct hdr_mac802_11*) hdr;

	if(dst)
		//ETHER_ADDR(dh->dh_da) = dst;
		STORE4BYTE(&dst, (dh->dh_da));

	return ETHER_ADDR(dh->dh_da);
}

inline int 
newMac802_11::hdr_src(char* hdr, u_int32_t src)
{
	struct hdr_mac802_11 *dh = (struct hdr_mac802_11*) hdr;

	if(src)
		//ETHER_ADDR(dh->dh_sa) = src; 
		STORE4BYTE(&src, (dh->dh_sa));

	return ETHER_ADDR(dh->dh_sa);
	//return *(dh->dh_sa);
}

inline int 
newMac802_11::hdr_type(char* hdr, u_int16_t type)
{
	struct hdr_mac802_11 *dh = (struct hdr_mac802_11*) hdr;

	if(type)
		*((u_int16_t*) dh->dh_body) = type;

	return *((u_int16_t*) dh->dh_body);
}


/* ======================================================================
   Misc Routines
   ====================================================================== */
inline int
newMac802_11::is_idle()
{
	if(rx_state_ != newMAC_IDLE)
		return 0;

	if(tx_state_ != newMAC_IDLE)
		return 0;

	if(nav_ > Scheduler::instance().clock())
		return 0;

	return 1;
}


void
newMac802_11::discard(Packet *p, const char* why)
{
	hdr_mac802_11* mh = HDR_MAC(p);
	hdr_cmn *ch = HDR_CMN(p);

#if 0
	/* old logic 8/8/98 -dam */
	/*
	 * If received below the RXThreshold, then just free.
	 */
	if(p->txinfo.Pr < p->txinfo.ant.RXThresh) {
		Packet::free(p);
		return;
	}
#endif 0

	/* if the rcvd pkt contains errors, a real MAC layer couldn't
	   necessarily read any data from it, so we just toss it now */
	if(ch->error() != 0) {
		Packet::free(p);
		return;
	}

	switch(mh->dh_fc.fc_type) {

	case MAC_Type_Management:
		drop(p, why);
		//drop(p);
		return;

	case MAC_Type_Control:
		switch(mh->dh_fc.fc_subtype) {

		case MAC_Subtype_RTS:
			if((u_int32_t)ETHER_ADDR(mh->dh_sa) == index_) {
			  drop(p, why);
			  //drop(p);
				
				return;
			}

		/* fall through - if necessary */

		case MAC_Subtype_CTS:
		case MAC_Subtype_ACK:
			if((u_int32_t)ETHER_ADDR(mh->dh_da) == index_) {
				//drop(p, why);
				drop(p);
				
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
			if((u_int32_t)ETHER_ADDR(mh->dh_da) == index_ ||
			   (u_int32_t)ETHER_ADDR(mh->dh_sa) == index_ ||
			   (u_int32_t)ETHER_ADDR(mh->dh_da) == MAC_BROADCAST) {
				//if (*(mh->dh_da) == (u_char)index_ ||
				//  *(mh->dh_sa) == (u_char)index_ ||
				//  *(mh->dh_sa) == (u_char)MAC_BROADCAST) {
				//drop(p, why);
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
newMac802_11::capture(Packet *p)
{
	/*
	 * Update the NAV so that this does not screw
	 * up carrier sense.
	 */
	set_nav(usec(eifs_ + TX_Time(p)));

	Packet::free(p);
}


void
newMac802_11::collision(Packet *p)
{
	switch(rx_state_) {

	case newMAC_RECV:
		SET_RX_STATE(newMAC_COLL);

		/* fall through */

	case newMAC_COLL:
		assert(pktRx_);
		assert(mhRecv_.busy());

		/*
		 *  Since a collision has occurred, figure out
		 *  which packet that caused the collision will
		 *  "last" the longest.  Make this packet,
		 *  pktRx_ and reset the Recv Timer if necessary.
		 */
		if(TX_Time(p) > mhRecv_.expire()) {
			mhRecv_.stop();
			discard(pktRx_, DROP_MAC_COLLISION);
			pktRx_ = p;
			mhRecv_.start(TX_Time(pktRx_));
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
newMac802_11::tx_resume()
{
	assert(mhSend_.busy() == 0);
	assert(mhDefer_.busy() == 0);

	if(pktCTRL_) {
		/*
		 *  Need to send a CTS or ACK.
		 */
		mhDefer_.start(sifs_);
	}
	else if(pktRTS_) {

		if(mhBackoff_.busy() == 0)
			mhDefer_.start(difs_);

	}
	else if(pktTx_) {

		if(mhBackoff_.busy() == 0)
			mhDefer_.start(difs_);

	}
	else if(upcall_) {
		Handler *h = upcall_;
		upcall_ = 0;
		h->handle((Event*) 0);
	}

	SET_TX_STATE(newMAC_IDLE);
}


void
newMac802_11::rx_resume()
{
	assert(pktRx_ == 0);
	assert(mhRecv_.busy() == 0);

	SET_RX_STATE(newMAC_IDLE);
}


/* ======================================================================
   Timer Handler Routines
   ====================================================================== */
void
newMac802_11::backoffHandler()
{
	if(pktCTRL_) {
		assert(mhSend_.busy() || mhDefer_.busy());
		return;
	}

	if(check_pktRTS() == 0)
		return;

	if(check_pktTx() == 0)
		return;
}

void
newMac802_11::deferHandler()
{
	assert(pktCTRL_ || pktRTS_ || pktTx_);

	if(check_pktCTRL() == 0)
		return;

	assert(mhBackoff_.busy() == 0);

	if(check_pktRTS() == 0)
		return;

	if(check_pktTx() == 0)
		return;
}

void
newMac802_11::navHandler()
{
	if(is_idle() && mhBackoff_.paused())
		mhBackoff_.resume(difs_);
}

void
newMac802_11::recvHandler()
{
	recv_timer();
}

void
newMac802_11::sendHandler()
{
	send_timer();
}


void
newMac802_11::txHandler()
{
	tx_active_ = 0;
}


/* ======================================================================
   The "real" Timer Handler Routines
   ====================================================================== */
void
newMac802_11::send_timer()
{

	switch(tx_state_) {

	/*
	 * Sent a RTS, but did not receive a CTS.
	 */
	case newMAC_RTS:
		RetransmitRTS();

		break;

	/*
	 * Sent a CTS, but did not receive a DATA packet.
	 */
	case newMAC_CTS:
		assert(pktCTRL_);
		Packet::free(pktCTRL_); pktCTRL_ = 0;

		break;

	/*
	 * Sent DATA, but did not receive an ACK packet.
	 */
	case newMAC_SEND:
		RetransmitDATA();

		break;
		
	/*
	 * Sent an ACK, and now ready to resume transmission.
	 */
	case newMAC_ACK:
		assert(pktCTRL_);
		Packet::free(pktCTRL_); pktCTRL_ = 0;
		break;


	case newMAC_IDLE:
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
newMac802_11::check_pktCTRL()
{
	struct hdr_mac802_11 *mh;
	double timeout;

	if(pktCTRL_ == 0)
		return -1;
	if(tx_state_ == newMAC_CTS || tx_state_ == newMAC_ACK)
		return -1;

	mh = HDR_MAC(pktCTRL_);

	switch(mh->dh_fc.fc_subtype) {

	/*
	 *  If the medium is not IDLE, don't send the CTS.
	 */
	case MAC_Subtype_CTS:
		if(! is_idle()) {
			discard(pktCTRL_, DROP_MAC_BUSY); pktCTRL_ = 0;
			return 0;
		}

		SET_TX_STATE(newMAC_CTS);

		timeout = (mh->dh_duration * 1e-6) + CTS_Time; // XXX

		break;

	/*
	 * IEEE 802.11 specs, section 9.2.8
	 * Acknowledments are sent after an SIFS, without regard to
	 * the busy/idle state of the medium.
	 */
	case MAC_Subtype_ACK:
		SET_TX_STATE(newMAC_ACK);
		timeout = ACK_Time;

		break;

	default:
		fprintf(stderr, "Invalid MAC Control subtype\n");
		exit(1);
	}

        TRANSMIT(pktCTRL_, timeout);
	
	return 0;
}


int
newMac802_11::check_pktRTS()
{
	struct hdr_mac802_11 *mh;
	double timeout;

	assert(mhBackoff_.busy() == 0);

	if(pktRTS_ == 0)
 		return -1;
	mh = HDR_MAC(pktRTS_);

 	switch(mh->dh_fc.fc_subtype) {

	case MAC_Subtype_RTS:
		if(! is_idle()) {
			inc_cw();
			mhBackoff_.start(cw_, is_idle());
			return 0;
		}

		SET_TX_STATE(newMAC_RTS);
		timeout = CTSTimeout;

		break;

	default:
		fprintf(stderr, "Invalid MAC Control subtype\n");
		exit(1);
	}

        TRANSMIT(pktRTS_, timeout);

	return 0;
}


int
newMac802_11::check_pktTx()
{
	struct hdr_mac802_11 *mh;
	double timeout;

	assert(mhBackoff_.busy() == 0);

	if(pktTx_ == 0)
		return -1;

	mh = HDR_MAC(pktTx_);
       	int len = HDR_CMN(pktTx_)->size();

	switch(mh->dh_fc.fc_subtype) {

	case MAC_Subtype_Data:

		if(! is_idle()) {
			sendRTS(ETHER_ADDR(mh->dh_da));

			inc_cw();
			mhBackoff_.start(cw_, is_idle());
			return 0;
		}

		SET_TX_STATE(newMAC_SEND);

		if((u_int32_t)ETHER_ADDR(mh->dh_da) != MAC_BROADCAST)
			timeout = ACKTimeout(len);
		else
			timeout = TX_Time(pktTx_);

		break;

	default:
		fprintf(stderr, "Invalid MAC Control subtype\n");
		exit(1);
	}

        TRANSMIT(pktTx_, timeout);

	return 0;
}


/*
 * Low-level transmit functions that actually place the packet onto
 * the channel.
 */
void
newMac802_11::sendRTS(int dst)
{
	Packet *p = Packet::alloc();
	hdr_cmn* ch = HDR_CMN(p);
	struct rts_frame *rf = (struct rts_frame*)p->access(off_newmac_);
	
	assert(pktTx_);
	assert(pktRTS_ == 0);

	/*
	 *  If the size of the packet is larger than the
	 *  RTSThreshold, then perform the RTS/CTS exchange.
	 *
	 *  XXX: also skip if destination is a broadcast
	 */
	if( (u_int32_t) HDR_CMN(pktTx_)->size() < macmib_->RTSThreshold ||
	    (u_int32_t) dst == MAC_BROADCAST) {
		Packet::free(p);
		return;
	}

	ch->uid() = 0;
	ch->ptype() = PT_MAC;
	ch->size() = ETHER_RTS_LEN;
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

	rf->rf_duration = RTS_DURATION(pktTx_);
	//ETHER_ADDR(rf->rf_ra) = dst;
	STORE4BYTE(&dst, (rf->rf_ra));

	//ETHER_ADDR(rf->rf_ta) = index_;
	STORE4BYTE(&index_, (rf->rf_ta));
	

	// rf->rf_fcs;

	pktRTS_ = p;
}

void
newMac802_11::sendCTS(int dst, double rts_duration)
{
	Packet *p = Packet::alloc();
	hdr_cmn* ch = HDR_CMN(p);
	struct cts_frame *cf = (struct cts_frame*)p->access(off_newmac_);

	assert(pktCTRL_ == 0);

	ch->uid() = 0;
	ch->ptype() = PT_MAC;
	ch->size() = ETHER_CTS_LEN;
	ch->iface() = -2;
	ch->error() = 0;

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

	cf->cf_duration = CTS_DURATION(rts_duration);
	//ETHER_ADDR(cf->cf_ra) = dst;
	STORE4BYTE(&dst, (cf->cf_ra));
	

	// cf->cf_fcs;

	pktCTRL_ = p;
}

void
newMac802_11::sendACK(int dst)
{
	Packet *p = Packet::alloc();
	hdr_cmn* ch = HDR_CMN(p);
	struct ack_frame *af = (struct ack_frame*)p->access(off_newmac_);

	assert(pktCTRL_ == 0);

	ch->uid() = 0;
	ch->ptype() = PT_MAC;
	ch->size() = ETHER_ACK_LEN;
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

	af->af_duration = ACK_DURATION();
	//ETHER_ADDR(af->af_ra) = dst;
	STORE4BYTE(&dst, (af->af_ra));
	// af->af_fcs;

	pktCTRL_ = p;
}

void
newMac802_11::sendDATA(Packet *p)
{
	hdr_cmn* ch = HDR_CMN(p);
	struct hdr_mac802_11* dh = HDR_MAC(p);

	assert(pktTx_ == 0);

	/*
	 * Update the MAC header
	 */
	ch->size() += ETHER_HDR_LEN;

	dh->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	dh->dh_fc.fc_type       = MAC_Type_Data;
	dh->dh_fc.fc_subtype    = MAC_Subtype_Data;
	dh->dh_fc.fc_to_ds      = 0;
	dh->dh_fc.fc_from_ds    = 0;
	dh->dh_fc.fc_more_frag  = 0;
	dh->dh_fc.fc_retry      = 0;
	dh->dh_fc.fc_pwr_mgt    = 0;
	dh->dh_fc.fc_more_data  = 0;
	dh->dh_fc.fc_wep        = 0;
	dh->dh_fc.fc_order      = 0;

	if((u_int32_t)ETHER_ADDR(dh->dh_da) != MAC_BROADCAST)
		dh->dh_duration = DATA_DURATION();
	else
		dh->dh_duration = 0;

	pktTx_ = p;
}


/* ======================================================================
   Retransmission Routines
   ====================================================================== */
void
newMac802_11::RetransmitRTS()
{
	assert(pktTx_);
	assert(pktRTS_);

	assert(mhBackoff_.busy() == 0);

	macmib_->RTSFailureCount++;

	ssrc_ += 1;			// STA Short Retry Count

	if(ssrc_ >= macmib_->ShortRetryLimit) {

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
                        ch->size() -= ETHER_HDR_LEN;
                        ch->xmit_reason_ = XMIT_REASON_RTS;
                        ch->xmit_failure_(pktTx_->copy(),
                                          ch->xmit_failure_data_);
                }

		discard(pktTx_, DROP_MAC_RETRY_COUNT_EXCEEDED); pktTx_ = 0;

		ssrc_ = 0;
		rst_cw();
	}
	else {
		struct rts_frame *rf;

		rf = (struct rts_frame*)pktRTS_->access(off_newmac_);
		rf->rf_fc.fc_retry = 1;

		inc_cw();
		mhBackoff_.start(cw_, is_idle());
	}
}


void
newMac802_11::RetransmitDATA()
{
	struct hdr_cmn *ch;
	struct hdr_mac802_11 *mh;
	u_int32_t *rcount, *thresh;

	assert(mhBackoff_.busy() == 0);

	assert(pktTx_);
	assert(pktRTS_ == 0);

	ch = HDR_CMN(pktTx_);
	mh = HDR_MAC(pktTx_);

	/*
	 *  Broadcast packets don't get ACKed and therefore
	 *  are never retransmitted.
	 */
	if((u_int32_t)ETHER_ADDR(mh->dh_da) == MAC_BROADCAST) {
		Packet::free(pktTx_); pktTx_ = 0;

		/*
		 * Backoff at end of TX.
		 */
		rst_cw();
		mhBackoff_.start(cw_, is_idle());

		return;
	}

	macmib_->ACKFailureCount++;

	if((u_int32_t) ch->size() <= macmib_->RTSThreshold) {
		rcount = &ssrc_;
		thresh = &macmib_->ShortRetryLimit;
	}
	else {
		rcount = &slrc_;
		thresh = &macmib_->LongRetryLimit;
	}

	(*rcount)++;

	if(*rcount > *thresh) {

		macmib_->FailedCount++;

		/* tell the callback the send operation failed 
		   before discarding the packet */
		hdr_cmn *ch = HDR_CMN(pktTx_);
		if (ch->xmit_failure_) {
                        ch->size() -= ETHER_HDR_LEN;
                        ch->xmit_reason_ = XMIT_REASON_ACK;
                        ch->xmit_failure_(pktTx_->copy(),
                                          ch->xmit_failure_data_);
                }

		discard(pktTx_, DROP_MAC_RETRY_COUNT_EXCEEDED); pktTx_ = 0;

		*rcount = 0;
		rst_cw();
	}
	else {
		struct hdr_mac802_11 *dh;

		dh = HDR_MAC(pktTx_);
		dh->dh_fc.fc_retry = 1;

		sendRTS(ETHER_ADDR(mh->dh_da));
		inc_cw();
		mhBackoff_.start(cw_, is_idle());
	}
}


/* ======================================================================
   Incoming Packet Routines
   ====================================================================== */
void
newMac802_11::send(Packet *p, Handler *h)
{
	struct hdr_mac802_11* dh = HDR_MAC(p);

	upcall_ = h;
	sendDATA(p);
	sendRTS(ETHER_ADDR(dh->dh_da));

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
			if(mhDefer_.busy() == 0)
				mhDefer_.start(difs_);
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
newMac802_11::recv(Packet *p, Handler *h)
{
	struct hdr_cmn *hdr = HDR_CMN(p);

	/*
	 * Sanity Check
	 */
	assert(initialized());

	/*
	 *  Handle outgoing packets.
	 */
	if(hdr->direction() == -1) {
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

	if(rx_state_ == newMAC_IDLE) {
		SET_RX_STATE(newMAC_RECV);
		pktRx_ = p;

		/*
		 * Schedule the reception of this packet, in
		 * txtime seconds.
		 */
		mhRecv_.start(TX_Time(p));
	}
	else {
		/*
		 *  If the power of the incoming packet is smaller than the
		 *  power of the packet currently being received by at least
                 *  the capture threshold, then we ignore the new packet.
		 */
		if(pktRx_->txinfo.RxPr / p->txinfo.RxPr >= p->txinfo.CPThresh) {
			capture(p);
		}
		else {
			collision(p);
		}
	}
}








void
newMac802_11::recv_timer()
{
	hdr_cmn *ch = HDR_CMN(pktRx_);
	hdr_mac802_11 *mh = HDR_MAC(pktRx_);
	u_int32_t dst = ETHER_ADDR(mh->dh_da);
	u_int8_t  type = mh->dh_fc.fc_type;
	u_int8_t  subtype = mh->dh_fc.fc_subtype;

	assert(pktRx_);
	assert(rx_state_ == newMAC_RECV || rx_state_ == newMAC_COLL);
	
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
	if(rx_state_ == newMAC_COLL) {
		discard(pktRx_, DROP_MAC_COLLISION);
		set_nav(usec(eifs_));
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
		set_nav(usec(eifs_));
		goto done;
	}

	/*
	 * IEEE 802.11 specs, section 9.2.5.6
	 *	- update the NAV (Network Allocation Vector)
	 */
	if(dst != index_) {
		set_nav(mh->dh_duration);
	}

        /* tap out */
        if (tap_ && type == MAC_Type_Data &&
            MAC_Subtype_Data == subtype) tap_->tap(pktRx_);

	/*
	 * Address Filtering
	 */
	if(dst != index_ && dst != MAC_BROADCAST) {
		/*
		 *  We don't want to log this event, so we just free
		 *  the packet instead of calling the drop routine.
		 */
		discard(pktRx_, "---");
		goto done;
	}

	switch(type) {

	case MAC_Type_Management:
		discard(pktRx_, DROP_MAC_PACKET_ERROR);
		goto done;
		break;

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
			fprintf(stderr, "Invalid MAC Control Subtype %x\n",
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
			fprintf(stderr, "Invalid MAC Data Subtype %x\n",
				subtype);
			exit(1);
		}
		break;

	default:
		fprintf(stderr, "Invalid MAC Type %x\n", subtype);
		exit(1);
	}

	done:

	pktRx_ = 0;
	rx_resume();
}


void
newMac802_11::recvRTS(Packet *p)
{
	struct rts_frame *rf = (struct rts_frame*)p->access(off_newmac_);

	if(tx_state_ != newMAC_IDLE) {
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


void
newMac802_11::recvCTS(Packet *p)
{
	if(tx_state_ != newMAC_RTS) {
		discard(p, DROP_MAC_INVALID_STATE);
		return;
	}

	assert(pktRTS_);
	Packet::free(pktRTS_); pktRTS_ = 0;

	assert(pktTx_);

	mhSend_.stop();

	/*
	 * The successful reception of this CTS packet implies
	 * that our RTS was successful.  Hence, we can reset
	 * the Short Retry Count and the CW.
	 */
	ssrc_ = 0;
	rst_cw();

	tx_resume();

	mac_log(p);
}

void
newMac802_11::recvDATA(Packet *p)
{
	struct hdr_mac802_11 *dh = HDR_MAC(p);
	u_int32_t dst, src, size;

	{	struct hdr_cmn *ch = HDR_CMN(p);

		dst = ETHER_ADDR(dh->dh_da);
		src = ETHER_ADDR(dh->dh_sa);
		size = ch->size();

		/*
		 * Adjust the MAC packet size - ie; strip
		 * off the mac header
		 */
		ch->size() -= ETHER_HDR_LEN;
		ch->num_forwards() += 1;
	}

	/*
	 *  If we sent a CTS, clean up...
	 */
	if(dst != MAC_BROADCAST) {
		if(size >= macmib_->RTSThreshold) {
			if (tx_state_ == newMAC_CTS) {
				assert(pktCTRL_);
				Packet::free(pktCTRL_); pktCTRL_ = 0;

				mhSend_.stop();

				/*
				 * Our CTS got through.
				 */
				ssrc_ = 0;
				rst_cw();
			}
			else {
				discard(p, DROP_MAC_BUSY);
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
	if(dst != MAC_BROADCAST) {
		Host *h = &cache_[src];

		if(h->seqno && h->seqno == dh->dh_scontrol) {
			discard(p, DROP_MAC_DUPLICATE);
			return;
		}
		h->seqno = dh->dh_scontrol;
	}

	/*
	 *  Pass the packet up to the link-layer.
	 *  XXX - we could schedule an event to account
	 *  for this processing delay.
	 */
	//p->incoming = 1;
	uptarget_->recv(p, (Handler*) 0);
}


void
newMac802_11::recvACK(Packet *p)
{
	struct hdr_cmn *ch = HDR_CMN(p);

	if(tx_state_ != newMAC_SEND) {
		discard(p, DROP_MAC_INVALID_STATE);
		return;
	}

	assert(pktTx_);
	Packet::free(pktTx_); pktTx_ = 0;

	mhSend_.stop();

	/*
	 * The successful reception of this ACK packet implies
	 * that our DATA transmission was successful.  Hence,
	 * we can reset the Short/Long Retry Count and the CW.
	 */
	if((u_int32_t) ch->size() <= macmib_->RTSThreshold)
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


/* Centralized wireless TDMA protocl (for sigle hop only) 
   mac-tdma.cc
   by Xuan Chen (xuanc@isi.edu), ISI/USC 
*/

#include "delay.h"
#include "connector.h"
#include "packet.h"
#include "random.h"

// #define DEBUG

//#include <debug.h>
#include "arp.h"
#include "ll.h"
#include "mac.h"
#include "mac-tdma.h"
#include "cmu-trace.h"

#define SET_RX_STATE(x)			\
{					\
	rx_state_ = (x);			\
}

#define SET_TX_STATE(x)				\
{						\
	tx_state_ = (x);				\
}

/* Phy specs from 802.11 */
static PHY_MIB PMIB = {
	DSSS_CWMin, DSSS_CWMax, DSSS_SlotTime, DSSS_CCATime,
	DSSS_RxTxTurnaroundTime, DSSS_SIFSTime, DSSS_PreambleLength,
	DSSS_PLCPHeaderLength
};	

/* Timers */
void MacTdmaTimer::start(Packet *p, double time){
  Scheduler &s = Scheduler::instance();
  
  assert(busy_ == 0);
  
  busy_ = 1;
  paused_ = 0;
  stime = s.clock();
  rtime = time;
  assert(rtime >= 0.0);
  
  s.schedule(this, p, rtime);
}

void MacTdmaTimer::stop(Packet *p) {
  Scheduler &s = Scheduler::instance();
  
  assert(busy_);
  
  if(paused_ == 0)
    s.cancel((Event *)p);
  
  busy_ = 0;
  paused_ = 0;
  stime = 0.0;
  rtime = 0.0;
}

/* Defer timer */
void DeferTdmaTimer::handle(Event *e)
{       
  busy_ = 0;
  paused_ = 0;
  stime = 0.0;
  rtime = 0.0;
  
  mac->deferHandler(e);
}

/* Receive Timer */
void RxTdmaTimer::handle(Event *e) {       
  busy_ = 0;
  paused_ = 0;
  stime = 0.0;
  rtime = 0.0;
  
  mac->recvHandler(e);
}

/* Send Timer */
void TxTdmaTimer::handle(Event *e) {       
  busy_ = 0;
  paused_ = 0;
  stime = 0.0;
  rtime = 0.0;
  
  mac->sendHandler(e);
}

/* ======================================================================
   TCL Hooks for the simulator
   ====================================================================== */
static class MacTdmaClass : public TclClass {
public:
	MacTdmaClass() : TclClass("Mac/Tdma") {}
	TclObject* create(int, const char*const*) {
		return (new MacTdma(&PMIB));
	}
} class_mac_tdma;


/* ======================================================================
   Mac Class Functions
   ====================================================================== */
MacTdma::MacTdma(PHY_MIB* p) : Mac(), mhDefer_(this), mhTx_(this), mhRx_(this){
  /* Setup the phy specs. */
  phymib_ = p;

  /* Much simplified centralized scheduling algorithm for single hop
     topology, like WLAN etc. Haven't concern any dynamic factors--
     ie. recomputation, etc. */
  // Calculate the slot time based on the MAX allowed data length.
  slot_time_ = DATA_Time(MAC_TDMA_MAX_DATA_LEN);

  // Start the scheduling.
  start_time_ = NOW;
  slot_num_ = slot_pointer_++;
  tdma_schedule_[slot_num_] = (char) addr();

  //  printf("<%d>, Mac/Tdma, slot_time_ = %f, slot_pointer_=%d, slot_num_=%d, start_time_=%f, NOW=%f, tdma_schedule_(%d): %d\n", addr(), slot_time_, slot_pointer_, slot_num_, start_time_, NOW, slot_num_, tdma_schedule_[slot_num_]);

  tx_state_ = rx_state_ = MAC_IDLE;
  tx_active_ = 0;

  /* Initialy, the radio is off. */
  radio_active = 0;
}

/* similar to 802.11, no cached node lookup. */
int MacTdma::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "log-target") == 0) {
			logtarget_ = (NsObject*) TclObject::lookup(argv[2]);
			if(logtarget_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		}
	}
	return Mac::command(argc, argv);
}


/* ======================================================================
   Debugging Routines
   ====================================================================== */
void MacTdma::trace_pkt(Packet *p) {
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_mac_tdma* dh = HDR_MAC_TDMA(p);
	u_int16_t *t = (u_int16_t*) &dh->dh_fc;

	fprintf(stderr, "\t[ %2x %2x %2x %2x ] %x %s %d\n",
		*t, dh->dh_duration,
		ETHER_ADDR(dh->dh_da), ETHER_ADDR(dh->dh_sa),
		index_, packet_info.name(ch->ptype()), ch->size());
}

void MacTdma::dump(char *fname){
  fprintf(stderr, "\n%s --- (INDEX: %d, time: %2.9f)\n", fname, index_, Scheduler::instance().clock());
	
  fprintf(stderr, "\ttx_state_: %x, rx_state_: %x, idle: %d\n", tx_state_, rx_state_, is_idle());
  fprintf(stderr, "\tpktTx_: %x, pktRx_: %x, callback: %x\n", (int) pktTx_, (int) pktRx_, (int) callback_);
}


/* ======================================================================
   Packet Headers Routines
   ====================================================================== */
inline int MacTdma::hdr_dst(char* hdr, int dst ){
  struct hdr_mac_tdma *dh = (struct hdr_mac_tdma*) hdr;
	//dst = (u_int32_t)(dst);
  
  if(dst > -2)
    STORE4BYTE(&dst, (dh->dh_da));
  
  return ETHER_ADDR(dh->dh_da);
}

inline int MacTdma::hdr_src(char* hdr, int src ){
  struct hdr_mac_tdma *dh = (struct hdr_mac_tdma*) hdr;
  //src = (u_int32_t)(src);
  
  if(src > -2)
    STORE4BYTE(&src, (dh->dh_sa));
  
  return ETHER_ADDR(dh->dh_sa);
}

inline int MacTdma::hdr_type(char* hdr, u_int16_t type) {
  struct hdr_mac_tdma *dh = (struct hdr_mac_tdma*) hdr;
  
  if(type)
    //*((u_int16_t*) dh->dh_body) = type;
    STORE2BYTE(&type,(dh->dh_body));
  
  //return *((u_int16_t*) dh->dh_body);
  return GET2BYTE(dh->dh_body);
}

/* Test if the channel is idle. */
inline int MacTdma::is_idle() {
  if(rx_state_ != MAC_IDLE)
    return 0;
  
  if(tx_state_ != MAC_IDLE)
    return 0;
  
  return 1;
}

/* To handle incoming packet. */
void MacTdma::recv(Packet* p, Handler* h) {
  struct hdr_cmn *ch = HDR_CMN(p);
	
  //  printf("<%d>, %f, packet recved, direction %d, [UP: %d, DOWN: %d]\n", index_, NOW, ch->direction(), hdr_cmn::UP, hdr_cmn::DOWN);

  /* Incoming packets from phy layer, send UP to ll layer. 
     Now, it is in receiving mode. 
  */
  
  if (ch->direction() == hdr_cmn::UP) {
    sendUp(p);
    return;
  }
	
  /* Packets coming down from ll layer (from ifq actually),
     send them to phy layer. 
     Now, it is in transmitting mode. */
  callback_ = h;
  state(MAC_SEND);
  sendDown(p);
}

void MacTdma::sendUp(Packet* p) {
  struct hdr_cmn *ch = HDR_CMN(p);

  /* Can't receive while transmitting. Should not happen...?*/
  if (tx_state_ && ch->error() == 0) {
    printf("<%d>, can't receive while transmitting!\n", index_);
    ch->error() = 1;
  };

  /* Detect if there is any collision happened. should not happen...?*/
  if (rx_state_ == MAC_IDLE) {
    SET_RX_STATE(MAC_RECV);     // Change the state to recv.
    pktRx_ = p;                 // Save the packet for timer reference.

    /* Schedule the reception of this packet, 
       since we just see the packet header. */
    double rtime = TX_Time(p);
    //    printf("<%d>, %f, packet receive time needed: %f\n", index_, NOW, rtime);
    assert(rtime >= 0);

    /* Start the timer for receiving, will end when receiving finishes. */
    mhRx_.start(p, rtime);
  }
  else {
    /* Note: we don't take the channel status into account, ie. no collision,
       as collision should not happen...
    */
    printf("<%d>, receiving, but the channel is not idle....???\n", index_);
  }
}

/* Actually receive data packet when rxTimer times out. */
void MacTdma::recvDATA(Packet *p){
  /*Adjust the MAC packet size: strip off the mac header.*/
  struct hdr_cmn *ch = HDR_CMN(p);
  ch->size() -= ETHER_HDR_LEN;
  ch->num_forwards() += 1;
  
  /* Pass the packet up to the link-layer.*/
  uptarget_->recv(p, (Handler*) 0);
}

/* Send packet down to the physical layer. 
   Need to calculate a certain time slot for transmission. */
void MacTdma::sendDown(Packet* p) {
  double current_time, frame_time, stxt, txt, txdelay, txd,lltxt;
  int frame_num;
  u_int32_t dst, src, size;
  
  struct hdr_cmn* ch = HDR_CMN(p);
  struct hdr_mac_tdma* dh = HDR_MAC_TDMA(p);

  /* Update the MAC header, same as 802.11 */
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
    dh->dh_duration = DATA_DURATION;
  else
    dh->dh_duration = 0;

  dst = ETHER_ADDR(dh->dh_da);
  src = ETHER_ADDR(dh->dh_sa);
  size = ch->size();

  /* We need to figure out the value for the slot time. */
  /* The TDMA frame length. */
  frame_time = slot_pointer_ * slot_time_;
  current_time = NOW;
  /* Scheduled time to send the packet. */
  frame_num = (int)((current_time - start_time_) / frame_time);
  stxt = start_time_ + frame_num * frame_time + slot_num_ * slot_time_;
  txt = stxt;

  if (stxt < current_time)
    txt = stxt + frame_time;
  txdelay = txt - current_time;
  // We can do it in two ways. 
  txd = netif_->txtime(p);
  //txd = txtime(p);
  lltxt = txdelay + txd;
  
  //  printf("<%d>, %f, frame_time=%f, frame_num=%d, current_time = %f, stxt=%f, txt=%f, txdelay=%f\n", index_, NOW, frame_time, frame_num, current_time, stxt, txt, txdelay);
  //printf("<%d>, %f, packet scheduled to send at %f\n", index_, NOW, txt);
  mhDefer_.start(p, txdelay);
}

/* Actually send the packet. */
void MacTdma::send(Packet *p) {
  u_int32_t dst, src, size;
  struct hdr_cmn* ch = HDR_CMN(p);
  struct hdr_mac_tdma* dh = HDR_MAC_TDMA(p);
  double stime;

  /* Perform carrier sence...should not be collision...? */
  if(!is_idle()) {
    /* Note: we don't take the channel status into account, ie. no collision,
       as collision should not happen...
    */
    printf("<%d>, transmitting, but the channel is not idle...???\n", index_);
    return;
  }
  
  dst = ETHER_ADDR(dh->dh_da);
  src = ETHER_ADDR(dh->dh_sa);
  size = ch->size();
  stime = TX_Time(p);
  //  printf("<%d>, %f, send a packet [from %d to %d], size = %d, time needed: %f\n", index_, NOW, src, dst, size, stime);

  /* Turn on the radio and transmit! */
  pktTx_ = p;
  SET_TX_STATE(MAC_SEND);						     
  radio_active = 1;

  /* Start a timer that expires when the packet transmission is complete. */
  mhTx_.start(p->copy(), stime);
  downtarget_->recv(pktTx_, this);        
}

/* Timers' handlers */
/* Defer the packet sending to assigned slot. */
void MacTdma::deferHandler(Event *e) {
  send((Packet *)e);
}

void MacTdma::recvHandler(Event *e) {
  u_int32_t dst, src, size;
  struct hdr_cmn *ch = HDR_CMN(pktRx_);
  struct hdr_mac_tdma *dh = HDR_MAC_TDMA(pktRx_);

  /* Check if any collision happened while receiving. */
  if (rx_state_ == MAC_COLL) 
    ch->error() = 1;

  SET_RX_STATE(MAC_IDLE);
  
  /* check if this packet was unicast and not intended for me, drop it.*/   
  dst = ETHER_ADDR(dh->dh_da);
  src = ETHER_ADDR(dh->dh_sa);
  size = ch->size();

  //  printf("<%d>, %f, recv a packet [from %d to %d], size = %d\n", index_, NOW, src, dst, size);

  /* Not a packet destinated to me. */
  if (((u_int32_t)dst != MAC_BROADCAST) && (dst != index_)) {
    drop(pktRx_);
    return;
  }
  
  /* Now forward packet upwards. */
  recvDATA(pktRx_);
}

/* After transmission a certain packet. Turn off the radio. */
void MacTdma::sendHandler(Event *e) {
  //  printf("<%d>, %f, send a packet finished.\n", index_, NOW);

  /* Once transmission is complete, drop the packet. 
     p  is just for schedule a event. */
  SET_TX_STATE(MAC_IDLE);
  Packet::free((Packet *)e);
  radio_active = 0;

  /* unlock IFQ. */
  if(callback_) {
    Handler *h = callback_;
    callback_ = 0;
    h->handle((Event*) 0);
  } 
}

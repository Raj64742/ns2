// Copyright (c) 2000 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.

// smac is designed and developed by Wei Ye (SCADDS/ISI)
// and is ported into ns by Padma Haldar.

// This module implements Sensor-MAC
//  http://www.isi.edu/scadds/papers/smac_report.pdf
//
// It has the following functions.
//  1) Both virtual and physical carrier sense
//  2) RTS/CTS for hidden terminal problem
//  3) Backoff and retry
//  4) Broadcast packets are sent directly without using RTS/CTS/ACK.
//  5) A long unicast message is divided into multiple TOS_MSG (by upper
//     layer). The RTS/CTS reserves the medium for the entire message.
//     ACK is used for each TOS_MSG for immediate error recovery.
//  6) Node goes to sleep when its neighbor is communicating with another
//     node.
//  7) Each node follows a periodic listen/sleep schedule
// 


#include "smac.h"

static class MacSmacClass : public TclClass {
public:
  MacSmacClass() : TclClass("Mac/SMAC") {}
  TclObject* create(int, const char*const*) {
    return (new SMAC());
  }
} class_macSMAC;


// Timers call on expiration

int SmacTimer::busy()
{
  if (status_ != TIMER_PENDING)
    return 0;
  else
    return 1;
}

void SmacGeneTimer::expire(Event *e) {
  a_->handleGeneTimer();
}

void SmacRecvTimer::expire(Event *e) {
  stime_ = rtime_ = 0;
  a_->handleRecvTimer();
}

void SmacRecvTimer::sched(double time) {
  TimerHandler::sched(time);
  stime_ = Scheduler::instance().clock();
  rtime_ = time;
}

double SmacRecvTimer::timeToExpire() {
  return ((stime_ + rtime_) - Scheduler::instance().clock());
}

void SmacSendTimer::expire(Event *e) {
  a_->handleSendTimer();
}

void SmacNavTimer::expire(Event *e) {
  a_->handleNavTimer();
}

void SmacNeighNavTimer::sched(double time) {
  TimerHandler::sched(time);
  stime_ = Scheduler::instance().clock();
  rtime_ = time;
}

void SmacNeighNavTimer::expire(Event *e) {
  stime_ = rtime_ = 0;
  a_->handleNeighNavTimer();
}

double SmacNeighNavTimer::timeToExpire() {
  return ((stime_ + rtime_) - Scheduler::instance().clock());
}

void SmacCsTimer::expire(Event *e) {
  a_->handleCsTimer();
}

// if pending, cancel timer
void SmacCsTimer::checkToCancel() {
  if (status_ == TIMER_PENDING)
    cancel();
}

void SmacChkSendTimer::expire(Event *e) {
  a_->handleChkSendTimer();
}

// void SmacCounterTimer::expire(Event *e) {
//   a_->handleCounterTimer();
// }


SMAC::SMAC() : Mac(), mhNav_(this), mhNeighNav_(this), mhSend_(this), mhRecv_(this), mhGene_(this), 
	       mhCS_(this), mhChkSend_(this) {
  
  state_ = IDLE;
  radioState_ = RADIO_IDLE;
  tx_active_ = 0;
  mac_collision_ = 0;

  sendAddr_ = -1;
  recvAddr_ = -1;

  nav_ = 0;
  neighNav_ = 0;
  
  numRetry_ = 0;
  numExtend_ = 0;
  lastRxFrag_ = -3; // since -1, -2 and 0 could be valid pkt uid's
  //numFrags_ = 0;
  //succFrags_ = 0;

  dataPkt_ = 0;
  pktRx_ = 0;
  pktTx_ = 0;
  
#ifdef SMAC_NO_SYNC
  txData_ = 0;
#endif //NO_SYNC
  
  // calculate packet duration. Following equations assume 4b/6b coding.
  // All calculations yield in usec

  // since ns uses clock tick of 1 usec, I thnk we dont need to add 5ms (1 tinyOS clock-tick) to compensate for rounding errors
  // like in tinyOS but will keep unchanged for now
  
  //durDataPkt_ = ((SIZEOF_SMAC_DATAPKT) * 12 + 18) / 1.0e4 + CLKTICK2SEC(1);
  durDataPkt_ = ((SIZEOF_SMAC_DATAPKT) * 12 + 18) / 1.0e4 ;
  
  //durCtrlPkt_ = ((SIZEOF_SMAC_CTRLPKT) * 12 + 18) / 1.0e4 + CLKTICK2SEC(1);
  durCtrlPkt_ = ((SIZEOF_SMAC_CTRLPKT) * 12 + 18) / 1.0e4 ;
  
  timeWaitCtrl_ = durCtrlPkt_ + CLKTICK2SEC(4) ;    // timeout time

  
  //numSched_ = 0;
  //numNeighb_ = 0;
  
  // listen for a whole period to choose a schedule first
  // mhGene_.sched((LISTENTIME + SLEEPTIME) * (SYNCPERIOD + 1) +
  // Random::random() % SYNC_CW);
}


int SMAC::command(int argc, const char*const* argv)
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


// XXXX smac handler functions

void SMAC::handleSendTimer() {
  assert(pktTx_);
  
  struct hdr_smac *sh = HDR_SMAC(pktTx_);
  
  // Packet tx is done so radio should go back to idle
  radioState_ = RADIO_IDLE;
  tx_active_ = 0;
  
  switch(sh->type) {
    
  case RTS_PKT:
    sentRTS(pktTx_);
    break;

   case CTS_PKT:
     sentCTS(pktTx_);
     break;
    
   case DATA_PKT:
     sentDATA(pktTx_);
     break;
  
   case ACK_PKT:
     sentACK(pktTx_);
     break;
     
     //case SYNC_PKT:
     //schedTab_[currSched_].txSync_ = 0;
     //schedTab_[currSched_].numPeriods = SYNCPERIOD;
     //break;
     
  default:
    fprintf(stderr, "unknown mac pkt type, %d\n", sh->type);
    break;
  }
  
  pktTx_ = 0;
}


void SMAC::handleRecvTimer() {
  assert(pktRx_);
  
  struct hdr_cmn *ch = HDR_CMN(pktRx_);
  struct hdr_smac *sh = HDR_SMAC(pktRx_);

  if (state_ == SLEEP) {
    discard(pktRx_, DROP_MAC_SLEEP);
    radioState_ = RADIO_SLP;
    goto done;
  }
  
  // if the radio interface is tx'ing when this packet arrives
  // I would never have seen it and should do a silent discard 
  
  if (radioState_ == RADIO_TX) {
    Packet::free(pktRx_);
    goto done;
  }
  
  if (mac_collision_) {
    discard(pktRx_, DROP_MAC_COLLISION);
    mac_collision_ = 0;
    updateNav(CLKTICK2SEC(EIFS));
    
    if (state_ == CR_SENSE) 
      sleep(); // have to wait until next wakeup time
    else 
      radioState_ = RADIO_IDLE;
    
    goto done;
  }
  
  if (ch->error()) {
    Packet::free(pktRx_);
    updateNav(CLKTICK2SEC(EIFS));

    if (state_ == CR_SENSE) 
      sleep();
    else 
      radioState_ = RADIO_IDLE;
      
    goto done;
  }
  
  // set radio from rx to idle again
  radioState_ = RADIO_IDLE;

  switch (sh->type) {
  case DATA_PKT:
    handleDATA(pktRx_);
    break;
  case RTS_PKT:
    handleRTS(pktRx_);
    break;
  case CTS_PKT:
    handleCTS(pktRx_);
    break;
  case ACK_PKT:
    handleACK(pktRx_);
    break;
    //  case SYNC_PKT:
    //      if (numSched_ == 0) { // in CHOOSE_SCHED state
    //        mhGene_.stop();  // cancel timer
    //        setMySched(pktRx_);
    //      } else {
    //        handleSYNC(pktRx_);
    //      }
    //      break;
  default:
    fprintf(stderr, "Unknown smac pkt type, %d\n", sh->type);
    break;
  }
  
 done:
  pktRx_ = 0;
  
}

void SMAC::handleGeneTimer() 
{
  
#ifdef SMAC_NO_SYNC
  if (state_ == WAIT_CTS) {  // CTS timeout
#else
  //if (numSched_ == 0) {
  //setMyScehd(0); // I'm the primary synchroniser
  //} 
  //else if (state_ == WAIT_CTS) {  // CTS timeout
#endif // !NO_SYNC
    
    if (numRetry_ < RETRY_LIMIT) {
      numRetry_++;
      // wait until receiver's next wakeup
      state_ = IDLE;
      
#ifdef SMAC_NO_SYNC
      checkToSend();
#endif
      
    } else {
      state_ = IDLE;
      dataPkt_ = 0;
      numRetry_ = 0;
      //numFrags_ = 0;
      // signal upper layer about failure of tx 
      // txMsgFailed();
      txMsgDone();

    }
    
  } else if (state_ == WAIT_ACK) { // ack timeout
    
    if (numExtend_ < EXTEND_LIMIT) { // extend time
      printf("SMAC %d: no ACK received. Extend Tx time.\n", index_);
      numExtend_++;
      
      updateNeighNav(durDataPkt_ + durCtrlPkt_);
      //neighNav_ = (durDataPkt_ + durCtrlPkt_);
      
    } else { // reached extension limit, can't extend time
      //numFrags_--;
      
    }
    if (neighNav_ < (durDataPkt_ + durCtrlPkt_)) {
      
      // used up reserved time, stop tx
      
      discard(dataPkt_, DROP_MAC_RETRY_COUNT_EXCEEDED);
      dataPkt_ = 0;
      pktTx_ = 0;
      state_ = IDLE;
      
      // signal upper layer the number of transmitted frags
      //txMsgFailed(succFrags); -> no frag for now

      txMsgDone();
      
    } else { // still have time
      // keep sending until use up remaining time
      sendDATA();
    }
  }
}

void SMAC::handleNavTimer() {
  // medium is now free
  nav_ = 0; // why have this variable?? probably not required use the timer instead
  
#ifdef SMAC_NO_SYNC
  if (state_ == SLEEP)
    wakeup();

  // try to send waiting data, if any
  checkToSend();
#endif // NO_SYNC
 
}
 


int SMAC::checkToSend() {
  if (txData_ == 1) {
    assert(dataPkt_);
    struct hdr_smac *mh = HDR_SMAC(dataPkt_);
    
    if (radioState_ != RADIO_SLP && radioState_ != RADIO_IDLE)
      goto done;  // cannot send if radio is sending or recving
    
    if (state_ != SLEEP && state_ != IDLE && state_ != WAIT_DATA )
      goto done; // cannot send if not in any of these states
    
    if (!(mhNav_.busy()) && !(mhNeighNav_.busy()) &&
	(state_ == SLEEP || state_ == IDLE)) {
      
      if (state_ == SLEEP) wakeup();
      
      if ((u_int32_t)mh->dstAddr == MAC_BROADCAST)
	howToSend_ = BCASTDATA;
      else
	howToSend_ = UNICAST;
      
      state_ = CR_SENSE;
    
      // start cstimer
      double cw = (Random::random() % DATA_CW) * SLOTTIME;
      mhCS_.sched(CLKTICK2SEC(DWAIT) + cw);
      
      return 1;
    
    } else {
      return 0;
    }
    
  done:
    // there could be corner cases when smac requires a timer to check if it has any pkt to send.
    //if (!(mhChkSend_.busy()))
    //mhChkSend_.sched(CLKTICK2SEC(CHK_INTERVAL));
    return 0;
    
  } else {
    return 0;
  }
}


void SMAC::handleChkSendTimer() {
  checkToSend();
}

void SMAC::handleNeighNavTimer() {
  
  // Timer to track my neighbor's NAV
  neighNav_ = 0;         // probably don't need to use this variable
  
  if (state_ == WAIT_DATA) { // data timeout
    state_ = IDLE;
    
    // signal upper layer that rx msg is done
    // didnot get any/all data
    rxMsgDone(0); 
  } else 
#ifdef SMAC_NO_SYNC
    checkToSend();
#endif

}


void SMAC::handleCsTimer() {
  
  // carrier sense successful
  
#ifdef MAC_DEBUG
  if (howToSend_ != BCASTSYNC && dataPkt_ == 0)
    numCSError++;
#endif // MAC_DEBUG
  
  switch(howToSend_) {
    //  case BCASTSYNC:
    //      if (sendSYNC())
    //        state_ = IDLE;
    //      break;
    
  case BCASTDATA:
    startBcast();
    break;
    
  case UNICAST:
    startUcast();
    break;
  }
}

//  void SMAC::handleCounterTimer(int id) {

//    if (mhCounter_[id].value == SLEEPTIME) { //woken up from sleep
//      // listentime starts now
//      if (!(mhNav_.busy()) && !(mhNeighNav_.busy()) &&
//  	(state_ == SLEEP || state_ == IDLE)) {
//        if (state_ == SLEEP &&
//  	  (id == 0 || schedTab_[id].txSync == 1)) {
//  	wakeup();
//        }
//        if (schedTab_[id].txSync == 1) {
//  	// start carrier sense for sending sync
//  	howToSend_ = BCASTSYNC;
//  	currSched_ = id;
//  	state_ = CR_SENSE;
//  	mhCs_.sched(DIFS + Random::random() % SYNC_CW);
//        }
//      }
//      // start to listen now
//      mhCounter_[id].start(LISTENTIME);
    
//    } else if (mhCounter_[id].value == LISTENTIME) { //listentime over
//      // can start datatime now
//      if (schedTab_[id].txData == 1 &&
//  	(!(mhNav_.busy()) && !(mhNeighNav_.busy())) &&
//  	(state_ == SLEEP || state_ == IDLE)) {
//        // schedule sending data
//        if (state_ == SLEEP)
//  	wakeup();
//        struct hdr_smac *mh = (struct hdr_smac *)dataPkt_->access(mac_hdr::offset_);
//        if (mh->addr == MAC_BROADCAST)
//  	howToSend_ = BCASTDATA;
//        else
//  	howToSend_ = UNICAST;
//        currSched_ = id;
//        state_ = CR_SENSE;
//        // start cstimer
//        mhCs_.start(DWAIT + Random::random() % DATA_CW);
//      }
//      mhCounter_.start(DATATIME);
    
//    } else if (mhCounter_[id].value == DATATIME) { //datatime over
//      if (id == 0 && state_ == IDLE)
//        sleep();
//      // now time to go to sleep
//      mhCounter_.start(SLEEPTIME);

//      // check if ready to send out sync 
//      if (schedTab_[id].numPeriods > 0) {
//        schedTab__[id].numPeriods--;
//        if (schedTab_[id].numPeriods == 0) {
//  	schedTab_[id].txSync = 1;
//        }
//      }
//    }
//  }



// recv function for mac layer

void SMAC::recv(Packet *p, Handler *h) {
  
  struct hdr_cmn *ch = HDR_CMN(p);

  assert(initialized());

  // handle outgoing pkt
  if ( ch->direction() == hdr_cmn::DOWN) {
    sendMsg(p, h);
    return;
  }
  
  // handle incoming pkt
  // we have just recvd the first bit of a pkt on the network interface  
  
  // if the interface is in tx mode it probably would not see this pkt
  if (radioState_ == RADIO_TX && ch->error() == 0) {
    assert(tx_active_);
    ch->error() = 1;
    pktRx_ = p;
    mhRecv_.sched(txtime(p));
    return;
  }
  
  // cancel carrier sense timer and wait for entire pkt
  if (state_ == CR_SENSE) {
    printf("Cancelling CS- node %d\n", index_);
    // cancels only if timer is pending; smac could be in CR_SENSE with timer cancelled
    // incase it has already received a pkt and receiving again
    mhCS_.checkToCancel();
  }
  
  // if the interface is already in process of recv'ing pkt
  if (radioState_ == RADIO_RX) {
    assert(pktRx_); 
    assert(mhRecv_.busy());
    
    // if power of the incoming pkt is smaller than the power 
    // of the pkt currently being recvd by atleast the capture 
    // threshold then we ignore the new pkt.
    
    if (pktRx_->txinfo_.RxPr / p->txinfo_.RxPr >= p->txinfo_.CPThresh) 
      capture(p);
    else
      collision(p);
  } 
  
  else {
    radioState_ = RADIO_RX;
    pktRx_ = p;

    // air around me is going to be busy till I finish recv'ing this pkt
    //double tt = txtime(p);
    //updateNav(tt);
    
    mhRecv_.sched(txtime(p));
  }
}


void SMAC::capture(Packet *p) {
  // we update NAV for this pkt txtime
  updateNav(CLKTICK2SEC(EIFS) + txtime(p));
  Packet::free(p);
}


void SMAC::collision(Packet *p) {
  if (!mac_collision_)
    mac_collision_ = 1;
  
    // since a collision has occured figure out which packet that caused 
    // the collision will "last" longer. Make this pkt pktRx_ and reset the
    // recv timer.
  if (txtime(p) > mhRecv_.timeToExpire()) {
    mhRecv_.resched(txtime(p));
    discard(pktRx_, DROP_MAC_COLLISION);
    // shouldn't we free pkt here ???
    pktRx_ = p;

  }
  else 
    discard(p, DROP_MAC_COLLISION);
  // shouldn't we free pkt here ???
}


void SMAC::discard(Packet *p, const char* why)
{
  hdr_cmn *ch = HDR_CMN(p);
  hdr_smac *sh = HDR_SMAC(p);
  
  /* if the rcvd pkt contains errors, a real MAC layer couldn't
     necessarily read any data from it, so we just toss it now */
  if(ch->error() != 0) {
    Packet::free(p);
    //p = 0;
    return;
  }

  switch(sh->type) {
    
  case RTS_PKT:
    if (drop_RTS(p, why))
      return;
    break;
    
  case CTS_PKT:
  case ACK_PKT:
    if (drop_CTS(p, why))
      return;
    break;
  
  case DATA_PKT:
    if (drop_DATA(p, why))
      return;
    break;
    
  default:
    fprintf(stderr, "invalid MAC type (%x)\n", sh->type);
    //trace_pkt(p);
    exit(1);
  }
  Packet::free(p);
}


int SMAC::drop_RTS(Packet *p, const char* why) 
{
  struct smac_control_frame *cf = (smac_control_frame *)p->access(hdr_mac::offset_);
  
  if (cf->srcAddr == index_) {
    drop(p, why);
    return 1;
  }
  return 0;
}

int SMAC::drop_CTS(Packet *p, const char* why) 
{
  struct smac_control_frame *cf = (smac_control_frame *)p->access(hdr_mac::offset_);

  if (cf->dstAddr == index_) {
    drop(p, why);
    return 1;
  }
  return 0;
}

int SMAC::drop_DATA(Packet *p, const char* why) 
{
  hdr_smac *sh = HDR_SMAC(p);

  if ( (sh->dstAddr == index_) ||
       (sh->srcAddr == index_) ||
       ((u_int32_t)sh->dstAddr == MAC_BROADCAST)) {
    drop(p);
    return 1;
  }
  return 0;
}

//void SMAC::drop_SYNC(Packet *p, const char* why) 
//{}


// mac receiving functions

void SMAC::handleRTS(Packet *p) {
  // internal handler for RTS

  struct smac_control_frame *cf = (smac_control_frame *)p->access(hdr_mac::offset_);
  
  if(cf->dstAddr == index_) {
    if((state_ == IDLE || state_ == CR_SENSE) && nav_ == 0) {
      recvAddr_ = cf->srcAddr; // remember sender's addr

      if(sendCTS(cf->duration)) {
	state_ = WAIT_DATA;
	lastRxFrag_ = -3; //reset frag no 
      }
    }
  } else { 
    // pkt destined to another node
    // don't go to sleep unless hear first data fragment
    // so I know how long to sleep
    if (state_ == CR_SENSE)
      state_ = IDLE;
    updateNav(durCtrlPkt_ + durDataPkt_);
  }

}

void SMAC::handleCTS(Packet *p) {
  // internal handler for CTS
  struct smac_control_frame *cf = (smac_control_frame *)p->access(hdr_mac::offset_);
  if(cf->dstAddr == index_) { // for me
    if(state_ == WAIT_CTS && cf->srcAddr == sendAddr_) {
      // cancel CTS timer
      mhGene_.cancel();

      if(sendDATA()) {
	state_ = WAIT_ACK;
	//schedTab_[currSched_].txData = 0;
	txData_ = 0;
      }
    }
  } else { // for others
    updateNav(cf->duration);
    if(state_ == IDLE || state_ == CR_SENSE)
      sleep();
  }
}

void SMAC::handleDATA(Packet *p) {
  // internal handler for DATA packet
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_smac * sh = HDR_SMAC(p);
  
  if((u_int32_t)sh->dstAddr == MAC_BROADCAST) { // brdcast pkt
    state_ = IDLE;
    // hand pkt over to higher layer
    rxMsgDone(p);
    
  } else if (sh->dstAddr == index_) { // unicast pkt
    if(state_ == WAIT_DATA && sh->srcAddr == recvAddr_) {
    // Should track neighbors' NAV, in case tx extended
      updateNeighNav(sh->duration);
      sendACK(sh->duration);
      
      //if (sh->duration > durCtrlPkt_) { // wait for more frag
      //rxFragDone(p);   no frag for now
      //state_ = IDLE;
      //} else {   // no more fragments
      
      state_ = IDLE;
      if(lastRxFrag_ != ch->uid()) {
	lastRxFrag_ = ch->uid();
	rxMsgDone(p);
      }
      else {
	printf("Recd duplicate data pkt at %d from %d! free pkt\n",index_,sh->srcAddr);
	Packet::free(p);
	checkToSend();
      }
    } else if (state_ == IDLE || state_ == CR_SENSE ) {
      printf("got data pkt in %d state XXX %d\n", state_, index_);
      //updateNav(sh->duration + 0.00001);  // incase I have a pkt to send
      sendACK(sh->duration);
      state_ = IDLE;
      if(lastRxFrag_ != ch->uid()) {
	lastRxFrag_ = ch->uid();
	rxMsgDone(p);
      }
      else {
	printf("Recd duplicate data pkt! free pkt\n");
	Packet::free(p);
	checkToSend();
      }
    } else { // some other state
      // not sure we can handle this
      // so drop pkt
      printf("Got data pkt in !WAIT_DATA/!CR_SENSE/!IDLE state(%d) XXX %d\n", state_, index_);
      printf("Dropping data pkt\n");
      Packet::free(p);
    }
  } else { // unicast pkt destined to other node
    updateNav(sh->duration);
    if (state_ == IDLE || state_ == CR_SENSE)
      sleep();
  }
}



void SMAC::handleACK(Packet *p) {
  
  // internal handler for ack
  struct smac_control_frame *cf = (smac_control_frame *)p->access(hdr_mac::offset_);
  
  if (cf->dstAddr == index_) {
    if (state_ == WAIT_ACK && cf->srcAddr == sendAddr_) {
      // cancel ACK timer
      mhGene_.cancel();
      dataPkt_ = 0;
      //numFrags_--;
      //succFrags_++;
      
      // if (numFrags_ > 0) { //need to send more frags
// 	if (neighNav__ < (durDataPkt_ + durCtrlPkt_)) {
// 	  // used up reserved time, have to stop
// 	  state_ = IDLE;
// 	  // txMsgFailed(succFrags_);
// 	  txMsgDone();
// 	} else { // continue on next fragment
// 	  state_ = WAIT_NEXTFRAG;
// 	  txFragDone(dataPkt_);
// 	}

//       } else {
      state_ = IDLE;
      txMsgDone();
      //}
    }
    
  } else { // destined to another node
    if (cf->duration > 0) {
      updateNav(cf->duration);
      if (state_ == IDLE || state_ == CR_SENSE)
	sleep();
    }
  }
}


//void SMAC::handleSYN(Packet *p) {
//
//}


void SMAC::rxMsgDone(Packet *p) {
  // no more fragments
  // defragment all pkts and send them up
  // fragmentation/de-frag to be implemented
  
  if (p)
    uptarget_->recv(p, (Handler*)0);
  
#ifdef SMAC_NO_SYNC
  // check if any pkt waiting to get tx'ed
  checkToSend();
#endif // smac_no-sync
  
}

//void SMAC::rxFragDone(Packet *p) {
// more fragments to come
//}



// mac transmission functions

void SMAC::transmit(Packet *p) {
  
  radioState_ = RADIO_TX;
  tx_active_ = 1;
  pktTx_ = p;
  
  downtarget_->recv(p->copy(), this);
  //Scheduler::instance().schedule(downtarget_, p, 0.000001);
  mhSend_.sched(txtime(p));

}

bool SMAC::chkRadio() {
  // check radiostate
  if (radioState_ == RADIO_IDLE || radioState_ == RADIO_SLP)
    return (1);

  return (0); // phy interface is ready to tx
}


int SMAC::startBcast()
{
  // broadcast data directly; don't use RTS/CTS

  hdr_smac *mh = HDR_SMAC(dataPkt_);
  
  mh->duration = 0;

  if(chkRadio()) {
    transmit(dataPkt_);
    return 1;
  }
  
  return 0;
}


int SMAC::startUcast()
{
  // start unicast data; send RTS first
  hdr_smac *mh = HDR_SMAC(dataPkt_);
  
  sendAddr_ = mh->dstAddr;
  numRetry_ = 0;
  succFrags_ = 0;
  numExtend_ = 0;
  
  if(sendRTS()) {
    state_ = WAIT_CTS;
    return 1;
  }
  
  return 0;
}


void  SMAC::txMsgDone() 
{
#ifdef SMAC_NO_SYNC
  // check if any data is waiting to get tx'ed
  if(checkToSend())
    return;
  else if (callback_) { // signal upper layer
    Handler *h = callback_;
    callback_ = 0;
    h->handle((Event*) 0);
  }
#else
  if (callback_) { // signal upper layer
    Handler *h = callback_;
    callback_ = 0;
    h->handle((Event*) 0);
  }
#endif
  
}



// void SMAC::txFragDone() 
// {
//   // send next fragment

// }


bool SMAC::sendMsg(Packet *pkt, Handler *h) {
  struct hdr_smac *mh = HDR_SMAC(pkt);

  callback_ = h;
  if ((u_int32_t)mh->dstAddr == MAC_BROADCAST) {
    return (bcastMsg(pkt));
  } else {
    return (unicastMsg(1, pkt));     // for now no fragmentation

    // fragmentation limit is 40 bytes per pkt.
    // max_msg_size is tentatively 1000 bytes; weiye will confirm this
  }
}


bool SMAC::bcastMsg(Packet *p) {
  //if (dataPkt_ != 0 || p == 0) 
  //return 0;
  assert(p);
  
  //if (state_ != IDLE && state_ != SLEEP && state_!= WAIT_DATA)
  //return 0;
  
  struct hdr_smac *sh = HDR_SMAC(p);
  
  sh->type = DATA_PKT;
  sh->length = SIZEOF_SMAC_DATAPKT;
  dataPkt_ = p;

  //for(int i =0; i < numSched_; i++) {
  //schedTab_[i].txData = 1;
  //}

#ifdef SMAC_NO_SYNC

  txData_ = 1;

  // check if can send now
  if (checkToSend())
    return 1;
  else 
    return 0;
#else
  numBcast_ = numSched_;
  return 1;
#endif // NO_SYNC

}

bool SMAC::unicastMsg(int numfrags, Packet *p) {
  //if (dataPkt != 0 || p == 0)
  //return 0;

  assert(p);

  //if (state_ != IDLE && state_ != SLEEP && state_!= WAIT_DATA)
  //return 0;

  // search for schedule of dest node
  //  struct hdr_smac *mh = (struct hdr_smac *)p->access(mac_hdr::offset_);
  //    int dest = mh->addr;
  //    for (int i=0; i < numNeighb_; i++) {
  //      if (neighbList[i].nodeid == dest) {
  //        found = 1;
  //        schedTab_[neighbList[i].schedId].txData = 1;
  //        break;
  //      }
  //    }
  //    if (found == 0)
  //      return 0;  // unknown neighbor
  
  char * mh = (char *)p->access(hdr_mac::offset_);
  int dst = hdr_dst(mh);
  int src = hdr_src(mh);
  
  struct hdr_smac *sh = HDR_SMAC(p);
  sh->type = DATA_PKT;
  sh->length = SIZEOF_SMAC_DATAPKT;
  sh->dstAddr = dst;
  sh->srcAddr = src;
  //numFrags_ = numfrags;
  dataPkt_ = p;

#ifdef SMAC_NO_SYNC
  txData_ = 1;

  // check if can send now
  if (checkToSend())
    return 1;
  else
    return 0;
#else
  return 1;
#endif //NO_SYNC

}

bool SMAC::sendRTS() {
  // construct RTS pkt
  Packet *p = Packet::alloc();
  struct smac_control_frame *cf = (struct smac_control_frame *)p->access(hdr_mac::offset_);
  struct hdr_cmn *ch = HDR_CMN(p);

  ch->uid() = 0;
  ch->ptype() = PT_MAC;
  ch->size() = SIZEOF_SMAC_CTRLPKT;
  ch->iface() = UNKN_IFACE.value();
  ch->direction() = hdr_cmn::DOWN;
  ch->error() = 0;	/* pkt not corrupt to start with */
  

  //bzero(cf, MAC_HDR_LEN);
  
  cf->length = SIZEOF_SMAC_CTRLPKT;
  cf->type = RTS_PKT;
  
  cf->srcAddr = index_;  // mac_id
  cf->dstAddr = sendAddr_;

  // reserved time for CTS + all fragments + all acks
  //cf->duration = (numFrags_ + 1) * durCtrlPkt_ + numFrags_ * durDataPkt_;
  cf->duration = (2 * durCtrlPkt_ + durDataPkt_ + 0.001 ); 

  // send RTS
  if (chkRadio()) {
    transmit(p);
    return 1;
    
  } else 
    return 0;

}


bool SMAC::sendCTS(double duration) {
  
  // construct CTS
  Packet *p = Packet::alloc();
  struct smac_control_frame *cf = (struct smac_control_frame *)p->access(hdr_mac::offset_);
  struct hdr_cmn *ch = HDR_CMN(p);

  ch->uid() = 0;
  ch->ptype() = PT_MAC;
  ch->size() = SIZEOF_SMAC_CTRLPKT;
  ch->iface() = UNKN_IFACE.value();
  ch->direction() = hdr_cmn::DOWN;
  ch->error() = 0;	/* pkt not corrupt to start with */

  //bzero(cf, MAC_HDR_LEN);
  
  cf->length = SIZEOF_SMAC_CTRLPKT;
  cf->type = CTS_PKT;
  
  cf->srcAddr = index_;
  cf->dstAddr = recvAddr_;

  // input duration is the duration field from received RTS pkt  
  cf->duration = duration - durCtrlPkt_ ;  
  
  // send CTS
  if (chkRadio()) {
    transmit(p);
    return 1;

  } else
    return 0;
}


bool SMAC::sendDATA() {
  // assuming data pkt is already constructed
  struct hdr_smac * sh = HDR_SMAC(dataPkt_);
  
  //sh->duration = numFrags_ * durCtrlPkt_ + (numFrags_ - 1) * durDataPkt_;
  sh->duration =  durCtrlPkt_;
  
  // send DATA
  if (chkRadio()) {
    transmit(dataPkt_);
    return 1;

  } else 
    return 0;

}


bool SMAC::sendACK(double duration) {
  // construct ACK pkt
  Packet *p = Packet::alloc();
  struct smac_control_frame *cf = (struct smac_control_frame *)p->access(hdr_mac::offset_);
  struct hdr_cmn *ch = HDR_CMN(p);

  ch->uid() = 0;
  ch->ptype() = PT_MAC;
  ch->size() = SIZEOF_SMAC_CTRLPKT;
  ch->iface() = UNKN_IFACE.value();
  ch->direction() = hdr_cmn::DOWN;
  ch->error() = 0;	/* pkt not corrupt to start with */
  
  //bzero(cf, MAC_HDR_LEN);
  
  cf->length = SIZEOF_SMAC_CTRLPKT;
  cf->type = ACK_PKT;
  
  cf->srcAddr = index_;
  cf->dstAddr = recvAddr_;
  
  // input duration is the duration field from recvd data pkt
  // stick to neighbNav -- should update it when rx data packet
  cf->duration = duration - durCtrlPkt_;
  //cf->duration = mhNeighNav_.timeToExpire() - durCtrlPkt_;
  
  // send ACK
  if (chkRadio()) {
    transmit(p);
    return 1;
  } else
    return 0;

}

void SMAC::sentRTS(Packet *p)
{
  // just sent RTS, set timer for CTS timeout
  mhGene_.sched(timeWaitCtrl_);
  Packet::free(p);
  
}

void SMAC::sentCTS(Packet *p)
{
  // just sent CTS, track my neighbors' NAV
  // they update NAV and go to sleep after recv CTS
  // no data timeout, just use neighbors' NAV
  // since they went to sleep, just wait data for the entire time

  struct smac_control_frame *cf = (struct smac_control_frame *)p->access(hdr_mac::offset_);
  
  updateNeighNav(cf->duration);
  Packet::free(p);
}

void SMAC::sentDATA(Packet *p)
{
  struct hdr_smac *mh = HDR_SMAC(p);
  
  if (howToSend_ == BCASTDATA) { // if data was brdcast
    state_ = IDLE;

#ifdef SMAC_NO_SYNC
    txData_ = 0;
#else 
    schedTab_[currSched_].txData_ = 0;
    //numBcast--;
    //if (numBcast == 0) {
#endif //NO_SYNC
    
    dataPkt_ = 0;
    Packet::free(p);
     
    // signal upper layer
    txMsgDone(); 
     
    //}
  } else {
    
    // unicast is done; track my neighbors' NAV
    // they update NAV and go to sleep after recv first data fragment

    updateNeighNav(mh->duration);
      
    //waiting for ACK, set timer for ACK timeout
    mhGene_.sched(timeWaitCtrl_);

  }
}

void SMAC::sentACK(Packet *p)
{
  struct smac_control_frame *cf = (struct smac_control_frame *)p->access(hdr_mac::offset_);
  
  updateNeighNav(cf->duration);
  Packet::free(p);
}

//void SMAC::sentSYNC(Packet *p)
//{}




void SMAC::sleep() 
{
  // go to sleep, turn off radio
  state_ = SLEEP;
  radioState_ = RADIO_SLP;

}

void SMAC::wakeup()
{
  //wakeup from sleep. turn on radio
  state_ = IDLE;
  
  // since radio can start to recv while in sleep
  // it might be in RX state
  // and eventually the pkt will not be recvd if in sleep state 
  // so careful not to change state of radio unless it is really sleeping
  if (radioState_ == RADIO_SLP)
    radioState_ = RADIO_IDLE;

}

void SMAC::updateNav(double d ) {
  double now = Scheduler::instance().clock();
  // already in sec
  // double d = duration * 1.0e-6; // convert to sec

  if ((now + d) > nav_) {
    nav_ = now + d;
    
    mhNav_.resched(d);
    
  }
}
	

void SMAC::updateNeighNav(double d ) {
  double now = Scheduler::instance().clock();
  //double d = duration * 1.0e-6; // convert to sec

  if ((now + d) > neighNav_) {
    neighNav_ = now + d;

    mhNeighNav_.resched(d);
  
  }
}

double SMAC::txtime(Packet *p)
{
  struct hdr_smac *sh = HDR_SMAC(p);

  switch(sh->type) {
  
  case DATA_PKT:
    return durDataPkt_;
    
  case RTS_PKT:
  case CTS_PKT:
  case ACK_PKT:
    return durCtrlPkt_;

  //case SYNCPKT:
  //return SYNCPKTTIME;
    
  default:
    fprintf(stderr, "invalid smac pkt type %d\n", sh->type);
    exit(1);
  }
  
}

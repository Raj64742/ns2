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
// and is re-written for ns by Padma Haldar (CONSER/ISI).

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
//  8) At bootup time each node listens for a fixed SYNCPERIOD and then
//     tries to send out a sync packet. It suppresses sending out of sync pkt 
//     if it happens to receive a sync pkt from a neighbor and follows the 
//     neighbor's schedule. 
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

// void SmacChkSendTimer::expire(Event *e) {
//   a_->handleChkSendTimer();
// }

void SmacCounterTimer::sched(double time) {
	// the cycle timer assumes that all time shall be scheduled with time "left to sleep" 
	// and not the absolute time for a given state (sleep, sync or data). Thus inorder 
	// to schedule for a sleep state, need to schedule with aggregate time CYCLETIME 
	// (sleeptime+synctime+dadatime).
	// Similarly for sync state, schedule with listenTime_ (synctime+datattime)
	// This is implemented to be in step with the counter used in actual smac.

	tts_ = time; // time before it goes to sleep again
	stime_ = Scheduler::instance().clock();
  
	if (time <= CLKTICK2SEC(cycleTime_) && time > CLKTICK2SEC(listenTime_)) { // in sleep state
		value_ = sleepTime_;
		if (status_ == TIMER_IDLE)
			TimerHandler::sched(time - CLKTICK2SEC(listenTime_)); 
		else
			TimerHandler::resched(time - CLKTICK2SEC(listenTime_)); 
    
	} else if ( time <= CLKTICK2SEC(listenTime_) && time > CLKTICK2SEC(dataTime_)) { // in sync state
		value_ = syncTime_;
		if (status_ == TIMER_IDLE)
			TimerHandler::sched(time - CLKTICK2SEC(dataTime_)); 
		else
			TimerHandler::resched(time - CLKTICK2SEC(dataTime_)); 
    
	} else { // in data state
		assert(time <= CLKTICK2SEC(dataTime_));
		value_ = dataTime_;
		if (status_ == TIMER_IDLE)
			TimerHandler::sched(time); 
		else
			TimerHandler::resched(time); 
    
	}

}

double SmacCounterTimer::timeToSleep() {
	return ((stime_ + tts_) - Scheduler::instance().clock()) ;
}

void SmacCounterTimer::expire(Event *e) {
	tts_ = stime_ = 0;
	a_->handleCounterTimer(index_);
}



SMAC::SMAC() : Mac(), mhNav_(this), mhNeighNav_(this), mhSend_(this), mhRecv_(this), mhGene_(this), mhCS_(this), syncFlag_(0) {

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

	/* setup internal mac and physical parameters
	   ----------------------------------------------
	   byte_tx_time_: time to transmit a byte, in ms. Derived from bandwidth
  
	   slotTime_: time of each slot in contention window. It should be large
	   enough to receive the whole start symbol but cannot be smaller than clock 
	   resolution. in msec
  
	   slotTime_sec_: slottime in sec
  
	   difs_: DCF interframe space (from 802.11), in ms. It is used at the beginning
	   of each contention window. It's the minmum time to wait to start a new 
	   transmission.
  
	   sifs_: short interframe space (f

	   /rom 802.11), in ms. It is used before sending
	   an CTS or ACK packet. It takes care of the processing delay of each pkt.
  
	   eifs_: Entended interfrane space (from 802.11) in ms. Used for backing off 
	   incase of a collision.

	   guardTime_: guard time at the end of each listen interval, in ms.

	*/

	byte_tx_time_ = 8.0 / BANDWIDTH;
	double start_symbol = byte_tx_time_ * 2.5;  // time to tx 20 bits
	slotTime_ = CLOCKRES >= start_symbol ? CLOCKRES : start_symbol;  // in msec
	slotTime_sec_ = slotTime_ / 1.0e3;   // in sec
	difs_ = 10.0 * slotTime_;
	sifs_ = 5.0 * slotTime_;
	eifs_ = 50.0 * slotTime_;
	guardTime_ = 4.0 * slotTime_;
  
	// calculate packet duration. Following equations assume 4b/6b coding.
	// All calculations yield in usec

	//durSyncPkt_ = ((SIZEOF_SMAC_SYNCPKT) * 12 + 18) / 1.0e4 ;

	durSyncPkt_ =  (PRE_PKT_BYTES + (SIZEOF_SMAC_SYNCPKT * ENCODE_RATIO)) * byte_tx_time_ + 1;
	durSyncPkt_ = CLKTICK2SEC(durSyncPkt_);

	//durDataPkt_ = ((SIZEOF_SMAC_DATAPKT) * 12 + 18) / 1.0e4 ;
	durDataPkt_ = (PRE_PKT_BYTES + (SIZEOF_SMAC_DATAPKT * ENCODE_RATIO)) * byte_tx_time_ + 1;
	durDataPkt_ = CLKTICK2SEC(durDataPkt_);

	//durCtrlPkt_ = ((SIZEOF_SMAC_CTRLPKT) * 12 + 18) / 1.0e4;
	durCtrlPkt_ = (PRE_PKT_BYTES + (SIZEOF_SMAC_CTRLPKT * ENCODE_RATIO)) * byte_tx_time_ + 1;
	durCtrlPkt_ = CLKTICK2SEC(durCtrlPkt_);
  
	// time to wait for CTS or ACK
	//timeWaitCtrl_ = durCtrlPkt_ + CLKTICK2SEC(4) ;    // timeout time
	double delay = 2 * PROC_DELAY + sifs_;
	timeWaitCtrl_ = CLKTICK2SEC(delay) + durCtrlPkt_;    // timeout time

  
	numSched_ = 0;
	numNeighb_ = 0;

	Tcl& tcl = Tcl::instance();
	tcl.evalf("Mac/SMAC set syncFlag_");
	if (strcmp(tcl.result(), "0") != 0)  
		syncFlag_ = 1;              // syncflag is set; use sleep-wakeup cycle
  
	if (!syncFlag_)
		txData_ = 0;
  
	else {
  
		// Calculate sync/data/sleeptime based on duty cycle
		// all time in ms
		syncTime_ = difs_ + slotTime_ * SYNC_CW + SEC2CLKTICK(durSyncPkt_) + guardTime_;
		dataTime_ = difs_ + slotTime_ * DATA_CW + SEC2CLKTICK(durCtrlPkt_) + guardTime_;
		listenTime_ = syncTime_ + dataTime_;
		cycleTime_ = listenTime_ * 100 / SMAC_DUTY_CYCLE + 1;
		sleepTime_ = cycleTime_ - listenTime_;
    
		//printf("cycletime=%d, sleeptime=%d, listentime=%d\n", cycleTime_, sleepTime_, listenTime_);


		for (int i=0; i< SMAC_MAX_NUM_SCHEDULES; i++) {
			mhCounter_[i] = new SmacCounterTimer(this, i);
			mhCounter_[i]->syncTime_ = syncTime_;
			mhCounter_[i]->dataTime_ = dataTime_;
			mhCounter_[i]->listenTime_ = listenTime_;
			mhCounter_[i]->sleepTime_ = sleepTime_;
			mhCounter_[i]->cycleTime_ = cycleTime_;
		}

		// printf("syncTime= %d, dataTime= %d, listentime = %d, sleepTime= %d, cycletime= %d\n", syncTime_, dataTime_, listenTime_, sleepTime_, cycleTime_);

		// listen for a whole period to choose a schedule first
		//double cw = (Random::random() % SYNC_CW) * slotTime_sec_ ;
  
		// The foll (higher) CW value allows neigh nodes to follow a single schedule
		// double w = (Random::random() % (SYNC_CW)) ;
		// double cw = w/10.0;
		double c = CLKTICK2SEC(listenTime_) + CLKTICK2SEC(sleepTime_);
		double s = SYNCPERIOD + 1;
		double t = c * s ;
		//mhGene_.sched(t + cw);
		mhGene_.sched(t);
	}
}

void SMAC::setMySched(Packet *pkt) 
{
	// set my schedule and put it into the first entry of schedule table
	state_ = IDLE;
	numSched_ = 1;
	schedTab_[0].numPeriods = 0;
	schedTab_[0].txData = 0;
	schedTab_[0].txSync = 1; // need to brdcast my schedule
  
	if (pkt == 0) { // freely choose my schedule

		mhCounter_[0]->sched(CLKTICK2SEC(listenTime_));
		mySyncNode_ = index_; // myself
    
		currSched_ = 0;
		//sendSYNC();  
    
	} else { // follow schedule in syncpkt
    
		struct smac_sync_frame *pf = (struct smac_sync_frame *)pkt->access(hdr_mac::offset_);

		mhCounter_[0]->sched(pf->sleepTime);

		mySyncNode_ = pf->srcAddr;
    
		//add node in my neighbor list
		neighbList_[0].nodeId = mySyncNode_;
		neighbList_[0].schedId = 0;
		numNeighb_ = 1;
	}
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
	case SYNC_PKT:
		sentSYNC(pktTx_);
		break;
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
		updateNav(CLKTICK2SEC(eifs_));
    
		if (state_ == CR_SENSE) 
			sleep(); // have to wait until next wakeup time
		else 
			radioState_ = RADIO_IDLE;
    
		goto done;
	}
  
	if (ch->error()) {
		Packet::free(pktRx_);
		updateNav(CLKTICK2SEC(eifs_));

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
	case SYNC_PKT:
		handleSYNC(pktRx_);
		break;
	default:
		fprintf(stderr, "Unknown smac pkt type, %d\n", sh->type);
		break;
	}
  
 done:
	pktRx_ = 0;
  
}

void SMAC::handleGeneTimer() 
{
  
	if (syncFlag_) {
		// still in choose-schedule state
		if (numSched_ == 0) {
			setMySched(0); // I'm the primary synchroniser
			return;
		} 
	}
	if (state_ == WAIT_CTS) {  // CTS timeout
		if (numRetry_ < SMAC_RETRY_LIMIT) {
			numRetry_++;
			// wait until receiver's next wakeup
			state_ = IDLE;
      
			if (!syncFlag_)
				checkToSend();
      
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
    
		if (numExtend_ < SMAC_EXTEND_LIMIT) { // extend time
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
  
	if (!syncFlag_) {
		if (state_ == SLEEP)
			wakeup();

		// try to send waiting data, if any
		checkToSend();
	} 
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
			double cw = (Random::random() % DATA_CW) * slotTime_sec_;
			mhCS_.sched(CLKTICK2SEC(difs_) + cw);
      
			return 1;
    
		} else {
			return 0;
		}
    
	done:
		return 0;
    
	} else {
		return 0;
	}
}


void SMAC::handleNeighNavTimer() {
  
	// Timer to track my neighbor's NAV
	neighNav_ = 0;         // probably don't need to use this variable
  
	if (state_ == WAIT_DATA) { // data timeout
		state_ = IDLE;
    
		// signal upper layer that rx msg is done
		// didnot get any/all data
		rxMsgDone(0); 
	} else {
		if (!syncFlag_)
			checkToSend();
	}
}


void SMAC::handleCsTimer() {
  
	// carrier sense successful
  
#ifdef MAC_DEBUG
	if (howToSend_ != BCASTSYNC && dataPkt_ == 0)
		numCSError++;
#endif // MAC_DEBUG
  
	switch(howToSend_) {
	case BCASTSYNC:
		if (sendSYNC())
			state_ = IDLE;
		break;
    
	case BCASTDATA:
		startBcast();
		break;
    
	case UNICAST:
		startUcast();
		break;
	}
}

void SMAC::handleCounterTimer(int id) {
  
	//printf("MAC:%d,id:%d - time:%.9f\n", index_,id,Scheduler::instance().clock());

	if (mhCounter_[id]->value_ == sleepTime_) { //woken up from sleep
		// listentime starts now

		if (radioState_ != RADIO_SLP && radioState_ != RADIO_IDLE)
			goto sched_1;  // cannot send if radio is sending or recving
    
		if (state_ != SLEEP && state_ != IDLE && state_ != WAIT_DATA )
			goto sched_1;; // cannot send if not in any of these states
    
		if (!(mhNav_.busy()) && !(mhNeighNav_.busy()) &&
		    (state_ == SLEEP || state_ == IDLE)) {
    
			if (state_ == SLEEP &&
			    (id == 0 || schedTab_[id].txSync == 1)) {
	
				wakeup();
			}
			if (schedTab_[id].txSync == 1) {
				// start carrier sense for sending sync
				howToSend_ = BCASTSYNC;
				currSched_ = id;
				state_ = CR_SENSE;
				double cw = (Random::random() % SYNC_CW) * slotTime_sec_;
				mhCS_.sched(CLKTICK2SEC(difs_) + cw);
			}
		}
		// start to listen now
	sched_1:
		mhCounter_[id]->sched(CLKTICK2SEC(listenTime_));
    
	} else if (mhCounter_[id]->value_ == syncTime_) { //synctime over
		// can start datatime now
    
		if (radioState_ != RADIO_SLP && radioState_ != RADIO_IDLE)
			goto sched_2;  // cannot send if radio is sending or recving
    
		if (state_ != SLEEP && state_ != IDLE && state_ != WAIT_DATA )
			goto sched_2; // cannot send if not in any of these states
    
		if (schedTab_[id].txData == 1 &&
		    (!(mhNav_.busy()) && !(mhNeighNav_.busy())) &&
		    (state_ == SLEEP || state_ == IDLE)) {
			// schedule sending data
      
			if (state_ == SLEEP)
				wakeup();
      
			struct hdr_smac *mh = (struct hdr_smac *)dataPkt_->access(hdr_mac::offset_);
			if ((u_int32_t)mh->dstAddr == MAC_BROADCAST)
				howToSend_ = BCASTDATA;
			else
				howToSend_ = UNICAST;
			currSched_ = id;
			state_ = CR_SENSE;
			// start cstimer
			double cw = (Random::random() % DATA_CW) * slotTime_sec_;
			mhCS_.sched(CLKTICK2SEC(difs_) + cw);
		}
	sched_2:
		mhCounter_[id]->sched(CLKTICK2SEC(dataTime_));
    
	} else if (mhCounter_[id]->value_ == dataTime_) { //datatime over

		// check if in the middle of recving a pkt
		if (radioState_ == RADIO_RX)
			goto sched_3;
    
		if (id == 0 && state_ == IDLE)
			sleep();

	sched_3:
		// now time to go to sleep
		mhCounter_[id]->sched(CLKTICK2SEC(cycleTime_));
    
		// check if ready to send out sync 
		if (schedTab_[id].numPeriods > 0) {
			schedTab_[id].numPeriods--;
			if (schedTab_[id].numPeriods == 0) {
				schedTab_[id].txSync = 1;
			}
		}
	}
}



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
		if (mhRecv_.busy()) { // and radiostate != RADIO_RX
			assert(radioState_ == RADIO_SLP);
			// The radio interface was recv'ing a pkt when it went to sleep
			// should it postpone sleep till it finishes recving the pkt???
			mhRecv_.resched(txtime(p));
		} else
			mhRecv_.sched(txtime(p));
    
		radioState_ = RADIO_RX;
		pktRx_ = p;
	}
}


void SMAC::capture(Packet *p) {
	// we update NAV for this pkt txtime
	updateNav(CLKTICK2SEC(eifs_) + txtime(p));
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

	case SYNC_PKT:
		if(drop_SYNC(p, why))
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
		drop(p, why);
		return 1;
	}
	return 0;
}

int SMAC::drop_SYNC(Packet *p, const char* why) 
{
	drop(p, why);
	return 1;
}


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

				if (!syncFlag_)
					txData_ = 0;
				else
					schedTab_[currSched_].txData = 0;
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
				if (!syncFlag_)
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
				if (!syncFlag_)
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


void SMAC::handleSYNC(Packet *p) 
{
	if(numSched_ == 0) { // in choose_sched state
		mhGene_.cancel();
		//double t = Scheduler::instance().clock();
		//struct smac_sync_frame *sf = (struct smac_sync_frame *)p->access(hdr_mac::offset_);
		//printf("Recvd SYNC (follow) at %d from %d.....at %.6f\n", index_, sf->srcAddr, t);
		setMySched(p);
		return;
	}
	if (numNeighb_ == 0) { // getting first sync pkt
		// follow this sched as have no other neighbor
	        //double t = Scheduler::instance().clock();
		//struct smac_sync_frame *sf = (struct smac_sync_frame *)p->access(hdr_mac::offset_);
		//printf("Recvd SYNC (follow) at %d from %d.....at %.6f\n", index_, sf->srcAddr, t);
	        setMySched(p);
		return;
	}
	state_ = IDLE;
	// check if sender is on my neighbor list
	struct smac_sync_frame *sf = (struct smac_sync_frame *)p->access(hdr_mac::offset_);
	int i, j;
	int foundNeighb = 0;
	int schedId = SMAC_MAX_NUM_SCHEDULES;
	//double t = Scheduler::instance().clock();
	//printf("Recvd SYNC (not/f) at %d from %d.....at %.6f\n", index_, sf->srcAddr, t);
  
	for(i = 0; i < numNeighb_; i++) {
		if (neighbList_[i].nodeId == sf->srcAddr) {
			foundNeighb = 1;
			schedId = neighbList_[i].schedId;  // a known neighbor
			mhCounter_[schedId]->sched(sf->sleepTime);
			break;
		}
		if (neighbList_[i].nodeId == sf->syncNode)
			// // found its synchronizer, remember it schedule id
			schedId = neighbList_[i].schedId;
	}
	if (!foundNeighb) { // unknown node, add it onto neighbor list
		neighbList_[numNeighb_].nodeId = sf->srcAddr;
		if (schedId < SMAC_MAX_NUM_SCHEDULES) {
			// found its synchronizer
			neighbList_[numNeighb_].schedId = schedId;
		} else if (sf->syncNode == index_) { // this node follows my schedule
			neighbList_[numNeighb_].schedId = 0;
		} else { // its synchronizer is unknown
			// check if its schedule equals to an existing one
			int foundSched = 0;
			for (j = 0; j < numSched_; j++) {
				double t = mhCounter_[j]->timeToSleep();
				double st = sf->sleepTime; 
				if (t == st || (t + CLKTICK2SEC(1)) == st || t == (st + CLKTICK2SEC(1))) {
					neighbList_[numNeighb_].schedId = j;
					foundSched = 1;
					break;
				}
			}
			if (!foundSched) { // this is unknown schedule
				schedTab_[numSched_].txSync = 1;
				schedTab_[numSched_].txData = 0;
				schedTab_[numSched_].numPeriods = 0;
				neighbList_[numNeighb_].schedId = numSched_;
				mhCounter_[numSched_]->sched(sf->sleepTime);
				numSched_++;
			}
		}
		numNeighb_++;  // increment number of neighbors
	}
}


void SMAC::rxMsgDone(Packet *p) {
	// no more fragments
	// defragment all pkts and send them up
	// fragmentation/de-frag to be implemented
  
	if (p)
		uptarget_->recv(p, (Handler*)0);
  
	if (!syncFlag_)
		// check if any pkt waiting to get tx'ed
		checkToSend();
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
	//succFrags_ = 0;
	numExtend_ = 0;
  
	if(sendRTS()) {
		state_ = WAIT_CTS;
		return 1;
	}
  
	return 0;
}


void  SMAC::txMsgDone() 
{
	if (!syncFlag_) {
		// check if any data is waiting to get tx'ed
		if(checkToSend())
			return;
		else if (callback_) { // signal upper layer
			Handler *h = callback_;
			callback_ = 0;
			h->handle((Event*) 0);
		}
	} else {
		if (callback_) { // signal upper layer
			Handler *h = callback_;
			callback_ = 0;
			h->handle((Event*) 0);
		}
	}
  
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

	//char * mh = (char *)p->access(hdr_mac::offset_);
	//int dst = hdr_dst(mh);
	//int src = hdr_src(mh);
  
	struct hdr_smac *sh = HDR_SMAC(p);
  
	sh->type = DATA_PKT;
	sh->length = SIZEOF_SMAC_DATAPKT;
	//sh->srcAddr = src;
	//sh->dstAddr = dst;
	dataPkt_ = p;

	for(int i=0; i < numSched_; i++) {
		schedTab_[i].txData = 1;
	}

	if (!syncFlag_) {
		txData_ = 1;
		// check if can send now
		if (checkToSend())
			return 1;
		else 
			return 0;
  
	} else {
		numBcast_ = numSched_;
		return 1;
	}
}

bool SMAC::unicastMsg(int numfrags, Packet *p) {
	//  if (dataPkt != 0 || p == 0)
	//return 0;

	assert(p);

	//if (state_ != IDLE && state_ != SLEEP && state_!= WAIT_DATA)
	//return 0;

  
	char * mh = (char *)p->access(hdr_mac::offset_);
	int dst = hdr_dst(mh);
	int src = hdr_src(mh);

	// search for schedule of dest node
	struct hdr_smac *sh = HDR_SMAC(p);
	//int dst = sh->dstAddr;

	if (syncFlag_) {
		int found = 0;
		for (int i=0; i < numNeighb_; i++) {
			if (neighbList_[i].nodeId == dst) {
				found = 1;
				schedTab_[neighbList_[i].schedId].txData = 1;
				break;
			}
		}
		if (found == 0) {
			printf("Neighbor unknown; cannot send pkt\n");
			return 0;  // unknown neighbor
		}
	}

	sh->type = DATA_PKT;
	sh->length = SIZEOF_SMAC_DATAPKT;
	sh->dstAddr = dst;
	sh->srcAddr = src;
	//numFrags_ = numfrags;
	dataPkt_ = p;

	if (!syncFlag_) {
		txData_ = 1;
    
		// check if can send now
		if (checkToSend())
			return 1;
		else
			return 0;
    
	} else 
		return 1;
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
  

	bzero(cf, MAC_HDR_LEN);
  
	cf->length = SIZEOF_SMAC_CTRLPKT;
	cf->type = RTS_PKT;
  
	cf->srcAddr = index_;  // mac_id
	cf->dstAddr = sendAddr_;

	// reserved time for CTS + all fragments + all acks
	//cf->duration = (numFrags_ + 1) * durCtrlPkt_ + numFrags_ * durDataPkt_;
	cf->duration = (2 * durCtrlPkt_ + durDataPkt_ + 0.001 ); 
	cf->crc = 0;
  
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

	bzero(cf, MAC_HDR_LEN);
  
	cf->length = SIZEOF_SMAC_CTRLPKT;
	cf->type = CTS_PKT;
  
	cf->srcAddr = index_;
	cf->dstAddr = recvAddr_;

	// input duration is the duration field from received RTS pkt  
	cf->duration = duration - durCtrlPkt_ ;  
	cf->crc = 0;
  
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
  
	bzero(cf, MAC_HDR_LEN);
  
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

bool SMAC::sendSYNC()
{
  // construct and send SYNC pkt
  Packet *p = Packet::alloc();
  struct smac_sync_frame *cf = (struct smac_sync_frame *)p->access(hdr_mac::offset_);
  struct hdr_cmn *ch = HDR_CMN(p);

  ch->uid() = 0;
  ch->ptype() = PT_MAC;
  ch->size() = SIZEOF_SMAC_SYNCPKT;
  ch->iface() = UNKN_IFACE.value();
  ch->direction() = hdr_cmn::DOWN;
  ch->error() = 0;	/* pkt not corrupt to start with */
  
  cf->length = SIZEOF_SMAC_SYNCPKT;
  cf->type = SYNC_PKT;
  
  cf->srcAddr = index_;
  cf->syncNode = mySyncNode_;
  // shld change SYNCPKTTIME to match with the configures durSyncPkt_
  cf->sleepTime = mhCounter_[0]->timeToSleep() - CLKTICK2SEC(SYNCPKTTIME);
  if (cf->sleepTime < 0)
    cf->sleepTime += CLKTICK2SEC(cycleTime_);
  
  // send SYNC
  if (chkRadio()) {
    transmit(p);
    //double t = Scheduler::instance().clock();
    //printf("Sent SYNC from %d.....at %.6f\n", cf->srcAddr, t);
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

    if (!syncFlag_) {
      txData_ = 0;
      dataPkt_ = 0;
      Packet::free(p);
    
      // signal upper layer
      txMsgDone(); 
    
    } else {
      schedTab_[currSched_].txData = 0;
      numBcast_--;
      if (numBcast_ == 0) {
	dataPkt_ = 0;
	Packet::free(p);
	
	// signal upper layer
	txMsgDone(); 
      }
    }
    
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

void SMAC::sentSYNC(Packet *p)
{
  schedTab_[currSched_].txSync = 0;
  schedTab_[currSched_].numPeriods = SYNCPERIOD;
  Packet::free(p);

}

void SMAC::sleep() 
{
  // go to sleep, turn off radio
  state_ = SLEEP;
  radioState_ = RADIO_SLP;
  //printf("SLEEP: ............node %d at %.6f\n", index_, Scheduler::instance().clock());
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
  //printf("WAKEUP: ............node %d at %.6f\n", index_, Scheduler::instance().clock());

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
  case SYNC_PKT:
    return CLKTICK2SEC(SYNCPKTTIME);
  default:
    fprintf(stderr, "invalid smac pkt type %d\n", sh->type);
    exit(1);
  }
  
}

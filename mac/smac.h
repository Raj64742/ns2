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
// and is ported into ns by Padma Haldar, June'02.

// This module implements Sensor-MAC
//  See http://www.isi.edu/scadds/papers/smac_report.pdf for details
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

#ifndef NS_SMAC
#define NS_SMAC

#include "mac.h"
#include "cmu-trace.h"
#include "random.h"
#include "timer-handler.h"


// define number of neighbors and schedules
#define MAX_NUM_NEIGHBORS 8
#define MAX_NUM_SCHEDULES 4

// XXX Note everything is in clockticks (5ms) for tinyOS
// so we need to convert that to sec for ns
#define CLKTICK2SEC(x)  (x * 0.005)

//length of a slottime is equal to a clock tick, i.e 5ms
#define SLOTTIME  0.005

// Length of one period = SLEEPTIME + SYNCTIME + DATATIME
// Only SLEEPTIME can be changed for different duty cycles.
#define SLEEPTIME 324   // duty cycle = (14+22)/(14+22+324) = 10%

// Do not change following parameters unless for tuning S-MAC performance
// LISTENTIME = SYNCTIME + DATATIME
//            = (DIFS + SYNC_CW + 4) + (DWAIT + DATA_CW + 5)
#define SYNCTIME 14
#define DATATIME 22
#define LISTENTIME (SYNCTIME + DATATIME)
#define SYNCPERIOD 10   // frequency (num of periods) to send a sync pkt
#define DIFS 3  // DCF interframe space: 3 CW slot here
#define SYNC_CW 7   // num of slots in sync contention window, 2^n - 1
#define DWAIT 2 // time to wait before start data contention
#define DATA_CW 15  // num of slots in data contention window, 2^n - 1
#define tick5ms 164,1  // initialize clock to 5ms per tick
#define CLOCKRES 5  // clock resolution is 5ms

#define EIFS 15     // about 5 times DIFS when there is a collision or corrupt pkt
#define CHK_INTERVAL (EIFS * 5)   // interval to check to see if can send data

// MAC states
#define SLEEP 0         // radio is turned off, can't Tx or Rx
#define IDLE 1          // radio in Rx mode, and can start Tx
//#define CHOOSE_SCHED 2  // node in boot-up phase, needs to choose a schedule
#define CR_SENSE 2      // medium is free, do it before initiate a Tx
//#define BACKOFF 3       // medium is busy, and cannot Tx
#define WAIT_CTS 3      // sent RTS, waiting for CTS
#define WAIT_DATA 4     // sent CTS, waiting for DATA
#define WAIT_ACK 5      // sent DATA, waiting for ACK
#define WAIT_NEXTFRAG 6 // send one fragment, waiting for next from upper layer


// how to send the pkt: broadcast or unicast
#define BCASTSYNC 0
#define BCASTDATA 1
#define UNICAST 2

#define DATA_PKT 0
#define RTS_PKT 1
#define CTS_PKT 2
#define ACK_PKT 3
#define SYNC_PKT 4

// retry limit: max number of RTS retries for a single message
// numRetry increments by 1 if sent RTS without receiving CTS
#define RETRY_LIMIT 5
// extension limit: max number of times that the sender can
// extend Tx time, when ACK timout happens
#define EXTEND_LIMIT 5

/* calculation of the time needed for transmit a packet
   tx rate: 10Kbps
1. coding: SEC_DED_BYTE_MAC (clock rate used: 10 ms per tick (tick100ps))
   Ttx = (18 + numByte * 18) / 10 (ms)
   where first 18 is for start symbol
   examples:
   numByte = 38, Ttx = (18 + 38 * 18) / 10 = 70.2 (ms)
   If timer is set by tick100ps, it needs at least 8 ticks
   numByte = 8, Ttx = (18 + 8 * 18) / 10 = 16.2 (ms) (2 ticks)
   For tx start symbol and the first byte, it only needs 3.6 ms (1 tick)
 2. coding: 4b/6b (clock rate used: 5 ms per tick (164, 1))
   Ttx = (18 + numByte * 12) / 10 (ms)
   where first 18 is for start symbol
   examples:
   numByte = 38, Ttx = (18 + 38 * 12) / 10 = 47.4 (ms)
   If timer is set as 5ms per tick, it needs at least 10 ticks
   numByte = 9, Ttx = (18 + 9 * 12) / 10 = 12.6 (ms) (3 ticks)
   For tx start symbol and the first byte, it only needs 3ms (1 tick)
*/


// radio states for performance measurement
#define RADIO_SLP 0  // radio off
#define RADIO_IDLE 1 // radio idle
#define RADIO_RX 2   // recv'ing mode
#define RADIO_TX 3   // transmitting mode



/*  XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/*  sizeof smac datapkt hdr and smac control and sync packets  */
/*  have been hardcoded here to mirror the values in TINY_OS implementation */
/*  The following is the pkt format definitions for tiny_os implementation */
/*  of smac : */

/*  typedef struct MAC_CTRLPKT_VALS{ */
/*  unsigned char length; */
/*  char type; */
/*  short addr; */
/*  unsigned char group; */
/*  short srcAddr; */
/*  unsigned char duration; */
/*  short crc; */
/*  }; */

/*  typedef struct MAC_SYNCPKT_VALS{ */
/*  unsigned char length; */
/*  char type; */
/*  short srcAddr; */
/*  short syncNode; */
/*  unsigned char sleepTime;  // my next sleep time from now */
/*  short crc; */
/*  };  */

/*  struct MSG_VALS{ */
/*  unsigned char length; */
/*  char type; */
/*  short addr; */
/*  unsigned char group; */
/*  short srcAddr; */
/*  unsigned char duration; */
/*  char data[DATA_LENGTH]; */
/*  short crc; */
/*  }; */

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#define SIZEOF_SMAC_DATAPKT 50  // hdr(10) + payload - fixed size pkts
#define SIZEOF_SMAC_CTRLPKT 10
//#define SIZEOF_SMAC_SYNCPKT 9  // not used for now


//SYNC PKT 
/* struct smac_sync_frame { */
/*   int type; */
/*   int length; */
/*   int srcAddr; */
/*   // syncNode; ??? */
/*   double sleepTime;  // my next sleep time from now */
/*   int crc; */
/* }; */

// RTS, CTS, ACK
struct smac_control_frame {
  int type;
  int length;
  int dstAddr;
  int srcAddr;
  double duration;
  int crc;
};

// DATA 
struct hdr_smac {
  int type;
  int length;
  int dstAddr;
  int srcAddr;
  double duration;
  //char data[DATA_LENGTH];
  int crc;
};

// Specifications for the SMAC radio (on motes)
// XXXX Need to define smac phyMIB and macMIB interfaces
/* #define SMAC_INTERFACE_CWMin 15 */
/* #define SMAC_INTERFACE_CWMax 15 */
/* #define SMAC_INTERFACE_SlotTime 0.005   // 5 ms */
/* #define SMAC_INTERFACE_MaxFragmentNum 25   //tentative,wei ye will confirm */
/* #define SMAC_INTERFACE_DataRate  1.0e4     // 10kbps */
/* #define SMAC_INTERFACE_PreambleLength  18  // 18 bits for start symbol */

/* // Specifications for SMAC */

/* // Length of one period = SLEEPTIME + SYNCTIME + DATATIME */
/* // Only SLEEPTIME can be changed for different duty cycles. */
/* #define SMAC_SLEEPTIME 324  // duty cycle = (14+22)/(14+22+324) = 10% */

/* #define SMAC_SYNCTIME  14 */
/* #define SMAC_DATATIME  22   // listentime = synctime + datatime */
/* #define SMAC_LISTENTIME (SYNCTIME + DATATIME) */
/* #define SMAC_SYNCPERIOD 10  // frequency (num of periods) to send a sync pkt */
/* #define SMAC_SYNC_CW 7      // num of slots in sync contention window, 2^n - 1 */
/* #define SMAC_DATA_CW 15     // num of slots in data contention window, 2^n - 1 */
/* #define SMAC_DIFS  3        // DCF interframe space: 3 CW slot here */
/* #define SMAC_DWAIT 2        // time to wait before start data contention */

/* // retry limit: max number of RTS retries for a single message */
/* // numRetry increments by 1 if sent RTS without receiving CTS */
/* #define SMAC_RETRY_LIMIT    5 */

/* // extension limit: max number of times that the sender can */
/* // extend Tx time, when ACK timout happens */
/* #define SMAC_EXTEND_LIMIT   5 */


/* struct SchedTable { */
/*   char txSync;  // flag indicating need to send sync */
/*   char txData;  // flag indicating need to send data */
/*   //unsigned short counter;  // tick counter */
/*   //unsigned char numPeriods; // count for number of periods */
/*   // these are implemented as part of timer for counter */
/* }; */

/* struct NeighbList { */
/* 	short nodeId; */
/* 	unsigned char schedId; */
/* }; */

class SMAC;

class SmacTimer : public TimerHandler {
 public:
  SmacTimer(SMAC *a) : TimerHandler() {a_ = a; }
  virtual void expire(Event *e) = 0 ;
  int busy() ;
 protected:
  SMAC *a_;
};

class SmacGeneTimer : public SmacTimer {
 public:
  SmacGeneTimer(SMAC *a) : SmacTimer(a) {}
  void expire(Event *e);
};

class SmacRecvTimer : public SmacTimer {
 public:
  SmacRecvTimer(SMAC *a) : SmacTimer(a) { stime_ = rtime_ = 0; }
  void sched(double duration);
  void expire(Event *e);
  double timeToExpire();
 protected:
  double stime_;
  double rtime_;
};

class SmacSendTimer : public SmacTimer {
 public:
  SmacSendTimer(SMAC *a) : SmacTimer(a) {}
  void expire(Event *e);
};

class SmacNavTimer : public SmacTimer {
 public:
  SmacNavTimer(SMAC *a) : SmacTimer(a) {}
  void expire(Event *e);
};

class SmacNeighNavTimer : public SmacTimer {
 public:
  SmacNeighNavTimer(SMAC *a) : SmacTimer(a) { stime_ = rtime_ = 0; }
  void sched(double duration);
  void expire(Event *e);
  double timeToExpire();
 protected:
  double stime_;
  double rtime_;
};

class SmacCsTimer : public SmacTimer {
 public:
  SmacCsTimer(SMAC *a) : SmacTimer(a) {}
  void expire(Event *e);
  void checkToCancel();
};

class SmacChkSendTimer : public SmacTimer {
 public:
  SmacChkSendTimer(SMAC *a) : SmacTimer(a) {}
  void expire(Event *e);
};


/* class SmacCounterTimer : public SmacTimer { */
/* public:  */
/*   SmacCounterTimer(SMAC *a) : SmacTimer(a) {} */
/*   void expire(Event *e); */
/* }; */

class SMAC : public Mac {
  
  friend class SmacGeneTimer;
  friend class SmacRecvTimer;
  friend class SmacSendTimer;
  friend class SmacNavTimer;
  friend class SmacNeighNavTimer;
  friend class SmacCsTimer; 
  friend class SmacChkSendTimer;
  // friend class SmacCounterTimer;

 public:
  SMAC(void);
  void recv(Packet *p, Handler *h);

 protected:
  
  // functions for handling timers
  void handleGeneTimer();
  void handleRecvTimer();
  void handleSendTimer();
  void handleNavTimer();
  void handleNeighNavTimer();
  void handleCsTimer();
  void handleChkSendTimer();
  // void handleCounterTimer();
  
 private:
  // XXXX functions for node schedule folowing sleep-wakeup cycles
  //void setMySched(Packet *syncpkt);
  void sleep();
  void wakeup();

  // XXXX functions for handling incoming packets
  
  void rxMsgDone(Packet* p);
  //void rxFragDone(Packet *p);  no frag for now

  void handleRTS(Packet *p);
  void handleCTS(Packet *p);
  void handleDATA(Packet *p);
  void handleACK(Packet *p);
  //void handleSYNC(Packet *p);

  // XXXX functions for handling outgoing packets
  
  int checkToSend();               // check if can send, start cs 
  bool chkRadio();         // checks radiostate
  void transmit(Packet *p);         // actually transmits packet

  bool sendMsg(Packet *p, Handler *h);
  bool bcastMsg(Packet *p);
  bool unicastMsg(int n, Packet *p);
  //int sendMoreFrag(Packet *p);
  
  void txMsgDone();
  // void txFragDone();

  int startBcast();
  int startUcast();
  
  bool sendRTS();
  bool sendCTS(double duration);
  bool sendDATA();
  bool sendACK(double duration);
  //bool sendSYNC();

  void sentRTS(Packet *p);
  void sentCTS(Packet *p);
  void sentDATA(Packet *p);
  void sentACK(Packet *p);
  //void sentSYNC(Packet *p);
  
  // Misc functions
  void collision(Packet *p);
  void capture(Packet *p);
  double txtime(Packet *p);
  
  void updateNav(double duration);
  void updateNeighNav(double duration);


  void mac_log(Packet *p) {
    logtarget_->recv(p, (Handler*) 0);
  }
  
  void discard(Packet *p, const char* why);
  int drop_RTS(Packet *p, const char* why);
  int drop_CTS(Packet *p, const char* why);
  int drop_DATA(Packet *p, const char* why);
  //int drop_SYNC(Packet *p, const char* why);
  
  // XXX SMAC variables

  NsObject*       logtarget_;
  
  // Internal states
  int  state_;                   // MAC state
  int  radioState_;              // state of radio, rx, tx or sleep
  int tx_active_;                
  int mac_collision_;            
  
  int sendAddr_;		// node to send data to
  int recvAddr_;		// node to receive data from
  
  double  nav_;	        // network allocation vector. nav>0 -> medium busy
  double  neighNav_;      // track neighbors' NAV while I'm sending/receiving
  
  // SMAC Timers
  SmacNavTimer	        mhNav_;		// NAV timer medium is free or not
  SmacNeighNavTimer     mhNeighNav_;    // neighbor NAV timer for data timeout
  SmacSendTimer		mhSend_;	// incoming packets
  SmacRecvTimer         mhRecv_;
  SmacGeneTimer         mhGene_;        // generic timer used sync/CTS/ACK timeout
  SmacCsTimer           mhCS_;          // carrier sense timer
  SmacChkSendTimer      mhChkSend_;     // periodically chks for data to send

  // array of schedtimer and synctimer, one for each schedule
  //SmacCounterTimer            mhCounter_[MAX_NUM_SCHEDULES];     // counter tracking node's sleep/awake cycle


  int numRetry_;	// number of tries for a data pkt
  int numExtend_;      // number of extensions on Tx time when frags are lost
  int numFrags_;       // number of fragments in this transmission
  int succFrags_;      // number of successfully transmitted fragments
  int lastRxFrag_;     // keep track of last data fragment recvd to prevent duplicate data

  int howToSend_;		// broadcast or unicast
  
  double durDataPkt_;     // duration of data packet XXX caveat fixed packet size
  double durCtrlPkt_;     // duration of control packet
  double timeWaitCtrl_;   // set timer to wait for a control packet
  
  //  struct SchedTable schedTab[MAX_NUM_SCHEDULES];   // schedule table
  // struct NeighbList neighbList[MAX_NUM_NEIGHBORS]; // neighbor list

  //Node *mySyncNode_;                                 // who's my synchronizer
  
  //int currSched_;      // current schedule I'm talking to
  //int numSched_;       // number of different schedules
  //int numNeighb_;      // number of known neighbors
  //int numBcast_;       // number of times needed to broadcast a packet
  
  //Packet *ctrlPkt_;	        // MAC control packet
  //Packet *syncPkt_;          // MAC sync packet
  Packet *dataPkt_;		// outgoing data packet

  Packet *pktRx_;               // buffer for incoming pkt
  Packet *pktTx_;               // buffer for outgoing pkt

#ifdef SMAC_NO_SYNC
  int txData_ ;
#endif //NO_SYNC
  
 protected:
  int command(int argc, const char*const* argv);
  virtual int initialized() { 
    return (netif_ && uptarget_ && downtarget_); 
  }
};


#endif //NS_SMAC

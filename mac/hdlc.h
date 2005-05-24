/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
 * Contributed by the Daedalus Research Group, http://daedalus.cs.berkeley.edu
 *
 */

/*
  This is an implementation of the ARQ feature used in HDLC working in ABM (Asynchronous Balanced Mode) which provides a reliable mode of link transfer using acknowledgements.

*/

#ifndef ns_hdlc_h
#define ns_hdlc_h

#include <ll.h>
#include <timer-handler.h>
#include <drop-tail.h>

class HDLC;

//#define HDLC_MWS  1024
#define HDLC_MWS  8
#define HDLC_MWM (HDLC_MWS-1)
#define HDLC_HDR_LEN      sizeof(struct hdr_hdlc);
#define DELAY_ACK_VAL     0.5     // arbitrarily chosen val for which acks are delayed

enum SS_t {RR=0, REJ=1, RNR=2, SREJ=3};
enum COMMAND_t {SNRM, SNRME, SARM, SARME, SABM, SABME, DISC, UA, DM};
enum HDLCFrameType { HDLC_I_frame, HDLC_S_frame, HDLC_U_frame };


class HdlcTimer : public TimerHandler {
public:
	HdlcTimer(HDLC *a, void (HDLC::*callback)() ) 
		: a_(a), callback_(callback) {};
	
protected:
	virtual void expire(Event *e);
	HDLC *a_;
	void (HDLC::*callback_)();
};

// class HdlcAckTimer : public TimerHandler {
// public:
// 	HdlcAckTimer(HDLC *a) : TimerHandler() { a_ = a; }
// protected:
// 	virtual void expire(Event *e);
// 	HDLC *a_;
// };

struct HDLCControlFrame {
	int recv_seq;
	int send_seq;
	char frame_command;
};

struct I_frame {
	int recv_seqno;
	bool poll_final_bit;
	int send_seqno;
};
                                                                                                                    
struct S_frame {
        int recv_seqno;
	bool poll_final_bit;
        SS_t stype;
};
                                                                                                                    
struct U_frame {
	bool poll_final_bit;
	COMMAND_t utype;
};



// The hdlc hdr share the same hdr space with hdr_ll
struct hdr_hdlc {
	int saddr_;                            // src mac id
	int daddr_;                           // destination mac id
	HDLCControlFrame hdlc_fc_;
	HDLCFrameType fc_type_;
	int padding_;
	inline int saddr() {return saddr_;}
	inline int daddr() {return daddr_;}
	
};

// XXXXXXXXX
// Currently, we assume the link layer shall have one p-2-p connection
// at any given time

// If this needs to change the ARQstate defined below should be used
// to support multiple p-2-p connections, each state for a src-dst pair.


struct ARQ_state {
	int saddr_;
	int daddr_;
	HdlcTimer rtx_timer_;
	PacketQueue sendBuf_;
	int SABME_req;
	int t_seqno_;
	int highest_ack_;
	int seqno_;
	int maxseq_;
	int closed_;
	int ntimeouts_;
};
	
	


// Brief description of HDLC model used :
// Type of station:
// combined station capable of issuing both commands and responses, 

// Link configurations:
// Consists of combined stations and supports both full-duplex and half-duplex transmission. HAlf-duplex for now; will be extended to full-duplex in the future.

// Data transfer mode: asynchronous balanced mode extended (ABME). Either station may initiate transmission without receiving permission from the other combined station. In this mode the frames are numbered and are acknowledged by the receiver. HDLC defines the extended mode to support 7 bits (128bit modulo numbering) for sequence numbers. For our simulation we use 31 bits.

// Initialization: consists of the sender sending a U frame issuing the SABME command (set asynchronous balanced mode/extended); the receiver sends back an UA (unnumbered ackowledged) or can send a DM (disconnected mode) to reject connection setup.

// Data sending: done using I frames by sender. receiver sends back RR S frames indicating the next I frame it expects. S frames are used for both error control as well as flow control. It sends a REJ for any missing frame asking receiver to go-back-N. It can send RNR asking recvr not to send any frames until it again sends RR. It also sends SREJ asking retx of a specific missing pkt.

// Disconnect:
//        Either HDLC entity can initiate a disconnect using a disconnect (DISC) frame. The remote entity must accept the disconnect by replying with a UA and informing its layer 3 user that the connection has been terminated. Any outstanding unacknowledged frames may be recovered by higher layers.


class HDLC : public LL {
	friend class HdlcTimer;
public:
	HDLC();
	virtual void recv(Packet* p, Handler* h);
	inline virtual void hdr_dst(Packet *p, int macDA) { HDR_HDLC(p)->daddr_ = macDA;}
	virtual void timeout();

protected:
	// main sending and recving routines
	void recvIncoming(Packet* p);
	void recvOutgoing(Packet* p);
	
	// misc routines
	void inSendBuffer(Packet *p);
	int resolveAddr(Packet *p);
	void reset();
	
	// queue 
	Packet *getPkt(PacketQueue buf, int seq);
	
	// rtx timer 
	void reset_rtx_timer(int backoff);
	void set_rtx_timer();
	void cancel_rtx_timer() { rtx_timer_.force_cancel(); }
	void rtt_backoff();
	double rtt_timeout();
	void delayTimeout();
	
	// sending routines
	void sendDown(Packet *p);
	void sendMuch();
	void output(Packet *p, int seqno);
	//Packet *dataToSend(Packet *p);
	void ack(Packet *p);
	Packet *dataToSend();
	//void ack();
	void sendUA(Packet *p, COMMAND_t cmd);
	void sendRR(Packet *p);
	void sendRNR(Packet *p);
	void sendREJ(Packet *p);
	void sendSREJ(Packet *p, int seq);
	void sendDISC(Packet *p);
	
	// receive routines
	void recvIframe(Packet *p);
	void recvSframe(Packet *p);
	void recvUframe(Packet *p);
	void handleSABMErequest(Packet *p);
	void handleUA(Packet *p);
	void handleDISC(Packet *p);
	void handleRR(Packet *p);    
	void handleRNR(Packet *p);
	void handleREJ(Packet *p);
	void handleSREJ(Packet *p);
	void handlePiggyAck(Packet *p);

	// go back N error recovery
	void goBackNMode(Packet *p);
	
	// selective repeat  mode
	void selectiveRepeatMode(Packet *p);
	
	// variables
	static int uidcnt_;
	int queueSize_;
	int timeout_;        // value of timeout
	int maxTimeouts_;    // max num of timeouts before connection closed

	int wnd_;            // size of window; set from tcl
	int highest_ack_;    // highest ack recvd so far
	int t_seqno_;        // tx'ing seq no
	int seqno_;          // counter of no for seq'ing incoming data pkts
	int maxseq_;         // highest seq no sent so far
	
	bool closed_;        // whether this connection is closed
	int nrexmit_;        // num of retransmits
	int ntimeouts_;      // num of retx timeouts
	int disconnect_;
	bool sentDISC_;
	
	int recv_seqno_;     // seq no of data pkts recvd by recvr
	bool SABME_req_;     // whether a SABME request has been sent or not
	bool sentREJ_;       // to prevent sending dup REJ

	Packet *save_;       // packet saved for delayed ack to allow piggybacking
	int delAckVal_;      // ack delayed 
	int delAck_;        // flag for delayed ack
	int selRepeat_;     // if this is set then selective repeat mode is used
	                     // otherwise Go Back N mode is used by default
	
	// Timers 
	HdlcTimer rtx_timer_;
	HdlcTimer delay_timer_;
	
	// buffer to hold outgoing the pkts until they are ack'ed 
	PacketQueue sendBuf_;
	
	// at receiving side hold pkts until they are all recvd in order
	// and can be fwded to layers above LL
	Packet** seen_;      // array of pkts seen
	int next_;       // next packet expected
	int maxseen_;    // max pkt number seen
	int wndmask_ ;       // window mask
	
	
};

#endif

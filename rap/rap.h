/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
//
// Copyright (c) 1997 by the University of Southern California
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
//
// rap.h 
//      Header File for the 'RAP Source' Agent Class
//
// Author: 
//   Mohit Talwar (mohit@catarina.usc.edu)
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/rap/rap.h,v 1.3 1999/06/09 21:54:09 haoboy Exp $

#ifndef RAP_H
#define RAP_H

#include "ns-process.h"
#include "app.h"
#include "utilities.h"

// RapTimeoutType
//      Possible timeout events

enum RapTimeoutType 
{
	RAP_IPG_TIMEOUT,		// IPG timer fires
	RAP_RTT_TIMEOUT		// RTT timer fires
};

// RapLossType
//      Possible loss types

enum RapLossType 
{
	RAP_TIMER_BASED,		// TIMER based loss detection
	RAP_ACK_BASED			// ACK based loss detection
};

// RapPacketStatus 
//      Possible packet types in the transmission history

enum RapPacketStatusType
{
	RAP_SENT,			// Packet has been sent
	RAP_PURGED,			// Packet detected lost or received
	RAP_INACTIVE			// Cluster loss ignored
};

class TransHistoryEntry
{
public:
	TransHistoryEntry(int seq, RapPacketStatusType stat = RAP_SENT)	{
		seqno = seq;
		status = stat;
		departureTime = Scheduler::instance().clock();
	};

	int seqno;
	RapPacketStatusType status;
	double departureTime;
};

// RAP packet header flags
const int RH_DATA = 0x01;
const int RH_ACK  = 0x02;

struct hdr_rap 
{
	int seqno_;		// Seq num of packet being sent/acked
	int size_;		// Size of user data inside this packet
	int rap_flags_;		// Flags to separate data/ack packets
	
	int lastRecv;		// Last hole at the RAP sink...
	int lastMiss;
	int prevRecv;

	static int offset_;	// offset for this header
	inline static int& offset() { return offset_; }
	inline static hdr_rap* access(Packet* p) {
		return (hdr_rap*) p->access(offset_);
	}

	int& seqno() { return seqno_; }
	int& size() { return size_; }
	int& flags() { return rap_flags_; }
};

class RapAgent;

class IpgTimer : public TimerHandler
{
public:
	IpgTimer(RapAgent *a) : TimerHandler() { a_ = a; }

protected:
	virtual void expire(Event *e);
	RapAgent *a_;
};

class RttTimer : public TimerHandler
{
public:
	RttTimer(RapAgent *a) : TimerHandler() { a_ = a; }

protected:
	virtual void expire(Event *e);
	RapAgent *a_;
};

// Rap flags
const int RF_ANYACK = 0x01;	// Received first ack
const int RF_STOP   = 0x02;	// This agent has stopped
const int RF_COUNTPKT = 0x04;    // This agent will send up to certain # of pkts

class RapAgent : public Agent	// RAP source
{
public:
	RapAgent();
	~RapAgent();

	void recv(Packet*, Handler*);
	void timeout(int type);

	int GetSeqno() { return seqno_; }
	double GetTimeout() { return timeout_; }
	int GetDebugFlag() { return debugEnable_; }
	FILE *GetLogfile() { return logfile_; }

	void IncrementLossCount() { sessionLossCount_++; }

	void start();
	void stop();
	void listen();
	void advanceby(int delta);
	void finish();

	// Data member access methods
	double srtt() { return srtt_; }
	double ipg() { return ipg_; }

	int anyack() { return flags_ & RF_ANYACK; }
	int is_stopped() { return flags_ & RF_STOP; }
	int counting_pkt() { return flags_ & RF_COUNTPKT; }
	void FixIpg(double fipg) { fixIpg_ = fipg; }

protected:
	virtual int command(int argc, const char*const* argv);

	void IpgTimeout();		// ipgTimer_ handler
	void RttTimeout();		// rttTimer_ handler

	void IncreaseIpg() { 
		fixIpg_ = 0; 
		ipg_ /= beta_; 
	}
	void DecreaseIpg() { 
		if (fixIpg_ != 0) 
			ipg_ = fixIpg_;
		else 
			ipg_ *= srtt_ / (ipg_ + srtt_);
	}
  
	// Adjust RTT estimate based on sample
	void UpdateTimeValues(double sampleRtt); 

	// Detect TIMER_BASED or ACK_BASED losses
	int LossDetection(RapLossType type, hdr_rap *ackHeader = NULL);
	// Increase ipg_ and mark history INACTIVE
	void LossHandler();		

	// Send a DATA packet
	void SendPacket(int nbytes, AppData *data = 0); 
	// Process an ACK
	void RecvAck(hdr_rap *ackHeader); 

	IpgTimer ipgTimer_;		// Send event
	RttTimer rttTimer_;		// IPG decrease event

	List transmissionHistory_;	// Of packets sent out
  
	TracedInt seqno_;		// Current sequence number
	TracedInt sessionLossCount_;	// ~ Packets lost in RAP session
	TracedInt curseq_;              // max # of pkts sent

	TracedDouble ipg_;		// Inter packet gap
	double beta_;			// Decrease factor

	TracedDouble srtt_;		// Smoothened round trip time
	double variance_;		// Variance in rtt samples
	double delta_;			// Jacobson/Karl 's constants...
	double mu_;
	double phi_;

	double overhead_;		// Max random delay added to IPG

	int useFineGrain_;		// Use fine grain rate adaptation?
	double frtt_;			// Fine grain feedback signals
	double xrtt_;
	double kxrtt_;
	double kfrtt_;

	TracedDouble timeout_;	// Timeout estimate

	double startTime_, stopTime_;	// Of this agent's life

	int debugEnable_;
	FILE *logfile_;

	// Data and methods as required by RAP receiver
	int lastRecv_;		// Last hole at the RAP sink...
	int lastMiss_;
	int prevRecv_;
	void UpdateLastHole(int seqNum);
	void SendAck(int seqNum);

	int off_rap_;
	int rap_base_hdr_size_;

	// Data packet counter: the number of data packets that we've sent 
	// in a RTT. If we have not sent much we should not increase 
	// rate because we don't have enough packets to probe for losses
	int dctr_;
	// The percentage threshold for increasing rate.
	int dpthresh_; 

	// Misc flags
	int flags_;
	double fixIpg_;
};

#endif // RAP_H

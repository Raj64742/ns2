/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
 *
 * Copyright (C) 2004 by USC/ISI
 *               2002 by Dina Katabi
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/xcp/xcp-end-sys.h,v 1.4 2004/10/13 16:23:19 haldar Exp $
 */

#ifndef ns_xcp_end_sys_h
#define ns_xcp_end_sys_h

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ip.h"
#include "tcp.h"
#include "flags.h"

#include "agent.h"
#include "packet.h"

#include "flags.h"
#include "tcp-sink.h"

//-----------  The Congestion Header ------------//
struct hdr_xcp {
	double	throughput_;
	double	rtt_;
	enum {
		XCP_DISABLED = 0,
		XCP_ENABLED,
		XCP_ACK
	} 	xcp_enabled_;		// to indicate that the flow is XCP enabled
	int	xcpId_;			// Sender's ID (debugging only)
	double	cwnd_;			// The current window (debugging only) 
	double	reverse_feedback_;

	// --- Initialized by source and Updated by Router 
	double delta_throughput_;
	unsigned int controlling_hop_;  // The AQM ID of the controlling router

	static int offset_;	    // offset for this header
	inline static int& offset() { return offset_; }
	inline static hdr_xcp* access(Packet* p) {
		return (hdr_xcp*) p->access(offset_);
	}

	/* per-field member functions */
	double& cwnd() { return (cwnd_); }
	double& rtt() { return (rtt_); }
};

/*--------------- Cwnd Shrinking Timer ---------------*
 *  If the cwnd becomes smaller than 1 then we keep it
 *  as one and reduce but delay sending the packet
 *  for s_rtt/cwnd
 * 
 * This code is to be written later!
 */

#define		MAX(a,b)	((a) > (b) ? (a) : (b))
#define		TP_TO_TICKS	MAX(1, (t_srtt_ >> T_SRTT_BITS))

#define TP_AVG_EXP		4	// used for xcp_metered_output_ == true

class XcpAgent;
  
class cwndShrinkingTimer : public TimerHandler {
public: 
	cwndShrinkingTimer(XcpAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	XcpAgent *a_;
};

class XcpAgent : public RenoTcpAgent {
public:
	XcpAgent();

protected:
	
	double time_now()  { return  Scheduler::instance().clock(); };
	void trace_var(char * var_name, double var);
	
	void init_rtt_vars(){
		flag_first_ack_received_ = 0.0;
		srtt_estimate_           = 0.0;
	}
	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char *varName, 
					const char *localName, 
					TclObject *tracer);
	
	virtual void output(int seqno, int reason = 0);
	virtual void recv_newack_helper(Packet *); 
	virtual void opencwnd(); 
	virtual void rtt_init(); // called in reset()
	virtual void rtt_update(double tao);
	
	/*--------- Variables --------------*/
	double current_positive_feedback_ ;
	int    tcpId_;
	double srtt_estimate_;
	/* more bits in delta for better precision, just for SRTT */
#define	XCP_DELTA_SHIFT		5
#define XCP_EXPO_SHIFT		3
#define	XCP_RTT_SHIFT		(XCP_DELTA_SHIFT + XCP_EXPO_SHIFT)	
	/* macros for SRTT calculations */
#define XCP_INIT_SRTT(rtt)					\
	((rtt) << XCP_RTT_SHIFT)
	   
#define XCP_UPDATE_SRTT(srtt, rtt)				\
	((srtt) + (((rtt) << XCP_DELTA_SHIFT)			\
		   - (((srtt) + (1 << (XCP_EXPO_SHIFT - 1)))	\
		      >> XCP_EXPO_SHIFT)))
	long   xcp_srtt_; // srtt estimate using the above macros
	double flag_first_ack_received_;

	// these are used if xcp_metered_output_ is true
	int    xcp_metered_output_;
	double xcp_feedback_;
	long   sent_bytes_;
	long   s_sent_bytes_;
	long   tp_to_start_ticks_;
	long   tp_to_ticks_;
	double estimated_throughput_;
	cwndShrinkingTimer shrink_cwnd_timer_;
};

class XcpSink : public Agent {
public:
	XcpSink(Acker*);
	void recv(Packet* pkt, Handler*);
	void reset();
	int command(int argc, const char*const* argv);
// 	TracedInt& maxsackblocks() { return max_sack_blocks_; }
protected:
	void ack(Packet*);
	virtual void add_to_ack(Packet* pkt);

	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char *varName, 
					const char *localName, 
					TclObject *tracer);
	Acker* acker_;
	int ts_echo_bugfix_;
	int ts_echo_rfc1323_;   // conforms to rfc1323 for timestamps echo
	// Added by Andrei Gurtov
	friend void Sacker::configure(TcpSink*);
// 	TracedInt max_sack_blocks_;	/* used only by sack sinks */
	Packet* save_;		/* place to stash saved packet while delaying */
				/* used by DelAckSink */
	int RFC2581_immediate_ack_;     // Used to generate ACKs immediately
	int bytes_;     // for JOBS
	// for RFC2581-compliant gap-filling.
	double lastreset_;      /* W.N. used for detecting packets
				 * from previous incarnations */
};

#endif /* ns_xcp_end_sys_h */

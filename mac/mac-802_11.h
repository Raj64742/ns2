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
 */
/* Ported from CMU/Monarch's code, nov'98 -Padma.
   wireless-mac-802_11.h

 */
#ifndef ns_mac_80211_h
#define ns_mac_80211_h

#include "mac-timers.h"
#include "marshall.h"

#define GET_ETHER_TYPE(x)		GET2BYTE((x))
#define SET_ETHER_TYPE(x,y)            {u_int16_t t = (y); STORE2BYTE(x,&t);}


/* ======================================================================
   Frame Formats
   ====================================================================== */

#define	MAC_ProtocolVersion	0x00

#define MAC_Type_Management	0x00
#define MAC_Type_Control	0x01
#define MAC_Type_Data		0x02
#define MAC_Type_Reserved	0x03

#define MAC_Subtype_RTS		0x0B
#define MAC_Subtype_CTS		0x0C
#define MAC_Subtype_ACK		0x0D
#define MAC_Subtype_Data	0x00

struct frame_control {
	u_char		fc_subtype		: 4;
	u_char		fc_type			: 2;
	u_char		fc_protocol_version	: 2;

	u_char		fc_order		: 1;
	u_char		fc_wep			: 1;
	u_char		fc_more_data		: 1;
	u_char		fc_pwr_mgt		: 1;
	u_char		fc_retry		: 1;
	u_char		fc_more_frag		: 1;
	u_char		fc_from_ds		: 1;
	u_char		fc_to_ds		: 1;
};

struct rts_frame {
	struct frame_control	rf_fc;
	u_int16_t		rf_duration;
	u_char			rf_ra[ETHER_ADDR_LEN];
	u_char			rf_ta[ETHER_ADDR_LEN];
	u_char			rf_fcs[ETHER_FCS_LEN];
};

struct cts_frame {
	struct frame_control	cf_fc;
	u_int16_t		cf_duration;
	u_char			cf_ra[ETHER_ADDR_LEN];
	u_char			cf_fcs[ETHER_FCS_LEN];
};

struct ack_frame {
	struct frame_control	af_fc;
	u_int16_t		af_duration;
	u_char			af_ra[ETHER_ADDR_LEN];
	u_char			af_fcs[ETHER_FCS_LEN];
};

struct hdr_mac802_11 {
	struct frame_control	dh_fc;
	u_int16_t		dh_duration;
	u_char			dh_da[ETHER_ADDR_LEN];
	u_char			dh_sa[ETHER_ADDR_LEN];
	u_char			dh_bssid[ETHER_ADDR_LEN];
	u_int16_t		dh_scontrol;
	u_char			dh_body[0]; // XXX Non-ANSI
};


/* ======================================================================
   Definitions
   ====================================================================== */
#define ETHER_HDR_LEN				\
	((phymib_->PreambleLength >> 3) +	\
	 (phymib_->PLCPHeaderLength >> 3) +	\
	 sizeof(struct hdr_mac802_11) +		\
	 ETHER_FCS_LEN)

#define ETHER_RTS_LEN				\
	((phymib_->PreambleLength >> 3) +        \
         (phymib_->PLCPHeaderLength >> 3) +      \
	 sizeof(struct rts_frame))

#define ETHER_CTS_LEN				\
        ((phymib_->PreambleLength >> 3) +        \
         (phymib_->PLCPHeaderLength >> 3) +      \
         sizeof(struct cts_frame))

#define ETHER_ACK_LEN				\
        ((phymib_->PreambleLength >> 3) +        \
         (phymib_->PLCPHeaderLength >> 3) +      \
	 sizeof(struct ack_frame))

#define	RTS_Time	(8 * ETHER_RTS_LEN / bandwidth_)
#define CTS_Time	(8 * ETHER_CTS_LEN / bandwidth_)
#define ACK_Time	(8 * ETHER_ACK_LEN / bandwidth_)
#define DATA_Time(len)	(8 * (len) / bandwidth_)

/*
 *  IEEE 802.11 Spec, section 9.2.5.7
 *	- After transmitting an RTS, a node waits CTSTimeout
 *	  seconds for a CTS.
 *
 *  IEEE 802.11 Spec, section 9.2.8
 *	- After transmitting DATA, a node waits ACKTimeout
 *	  seconds for an ACK.
 *
 *  IEEE 802.11 Spec, section 9.2.5.4
 *	- After hearing an RTS, a node waits NAVTimeout seconds
 *	  before resetting its NAV.  I've coined the variable
 *	  NAVTimeout.
 *
 */
#define CTSTimeout	((RTS_Time + CTS_Time) + 2 * sifs_)
#define ACKTimeout(len)	(DATA_Time(len) + ACK_Time + sifs_ + difs_)
#define NAVTimeout	(2 * phymib_->SIFSTime + CTS_Time + 2 * phymib->SlotTime)



#define RTS_DURATION(pkt)	\
	usec(sifs_ + CTS_Time + sifs_ + TX_Time(pkt) + sifs_ + ACK_Time)

#define CTS_DURATION(d)		\
	usec((d * 1e-6) - (CTS_Time + sifs_))

#define DATA_DURATION()		\
	usec(ACK_Time + sifs_)

#define ACK_DURATION()	0x00		// we're not doing fragments now

/*
 * IEEE 802.11 Spec, section 15.3.2
 *	- default values for the DSSS PHY MIB
 */
#define DSSS_CWMin			31
#define DSSS_CWMax			1023
#define DSSS_SlotTime			0.000020	// 20us
#define	DSSS_CCATime			0.000015	// 15us
#define DSSS_RxTxTurnaroundTime		0.000005	// 5us
#define DSSS_SIFSTime			0.000010	// 10us
#define DSSS_PreambleLength		144		// 144 bits
#define DSSS_PLCPHeaderLength		48		// 48 bits

class PHY_MIB {
public:
	u_int32_t	CWMin;
	u_int32_t	CWMax;
	double		SlotTime;
	double		CCATime;
	double		RxTxTurnaroundTime;
	double		SIFSTime;
	u_int32_t	PreambleLength;
	u_int32_t	PLCPHeaderLength;
};


/*
 * IEEE 802.11 Spec, section 11.4.4.2
 *      - default values for the MAC Attributes
 */
#define MAC_RTSThreshold		3000		// bytes
#define MAC_ShortRetryLimit		7		// retransmittions
#define MAC_LongRetryLimit		4		// retransmissions
#define MAC_FragmentationThreshold	2346		// bytes
#define MAC_MaxTransmitMSDULifetime	512		// time units
#define MAC_MaxReceiveLifetime		512		// time units

class MAC_MIB {
public:
	//		MACAddress;
	//		GroupAddresses;
	u_int32_t	RTSThreshold;
	u_int32_t	ShortRetryLimit;
	u_int32_t	LongRetryLimit;
	u_int32_t	FragmentationThreshold;
	u_int32_t	MaxTransmitMSDULifetime;
	u_int32_t	MaxReceiveLifetime;
	//		ManufacturerID;
	//		ProductID;

	u_int32_t	TransmittedFragmentCount;
	u_int32_t	MulticastTransmittedFrameCount;
	u_int32_t	FailedCount;	
	u_int32_t	RetryCount;
	u_int32_t	MultipleRetryCount;
	u_int32_t	FrameDuplicateCount;
	u_int32_t	RTSSuccessCount;
	u_int32_t	RTSFailureCount;
	u_int32_t	ACKFailureCount;
	u_int32_t	ReceivedFragmentCount;
	u_int32_t	MulticastReceivedFrameCount;
	u_int32_t	FCSErrorCount;
};


/* ======================================================================
   The following destination class is used for duplicate detection.
   ====================================================================== */
class Host {
public:
	LIST_ENTRY(Host) link;
	u_int32_t	index;
	u_int32_t	seqno;
};


/* ======================================================================
   The actual 802.11 MAC class.
   ====================================================================== */
class Mac802_11 : public Mac {
	friend class DeferTimer;
	friend class BackoffTimer;
	friend class IFTimer;
	friend class NavTimer;
	friend class RxTimer;
	friend class TxTimer;
public:
	Mac802_11(PHY_MIB* p, MAC_MIB *m);
	void		recv(Packet *p, Handler *h);
	inline int	hdr_dst(char* hdr, int dst = 0);
	inline int	hdr_src(char* hdr, int src = 0);
	inline int	hdr_type(char* hdr, u_int16_t type = 0);

protected:
	void	backoffHandler(void);
	void	deferHandler(void);
	void	navHandler(void);
	void	recvHandler(void);
	void	sendHandler(void);
	void	txHandler(void);

private:
	int		command(int argc, const char*const* argv);

	/*
	 * Called by the timers.
	 */
	void		recv_timer(void);
	void		send_timer(void);
	int		check_pktCTRL();
	int		check_pktRTS();
	int		check_pktTx();

	/*
	 * Packet Transmission Functions.
	 */
	void	send(Packet *p, Handler *h);
	void 	sendRTS(int dst);
	void	sendCTS(int dst, double duration);
	void	sendACK(int dst);
	void	sendDATA(Packet *p);
	void	RetransmitRTS();
	void	RetransmitDATA();

	/*
	 * Packet Reception Functions.
	 */
	void	recvRTS(Packet *p);
	void	recvCTS(Packet *p);
	void	recvACK(Packet *p);
	void	recvDATA(Packet *p);

	void		capture(Packet *p);
	void		collision(Packet *p);
	void		discard(Packet *p, const char* why);
	void		rx_resume(void);
	void		tx_resume(void);

	inline int	is_idle(void);

	/*
	 * Debugging Functions.
	 */
	void		trace_pkt(Packet *p);
	void		dump(char* fname);

	inline int initialized() {
		return (phymib_ && macmib_ && cache_ && logtarget_ && Mac::initialized());
	}

	void mac_log(Packet *p) {
		logtarget_->recv(p, (Handler*) 0);
	}

	inline double TX_Time(Packet *p) {
		double t = DATA_Time((HDR_CMN(p))->size());
		
		if(t < 0.0) {
			drop(p, "XXX");
			exit(1);
		}
		return t;
	}

	inline void inc_cw() {
		cw_ = (cw_ << 1) + 1;
		if(cw_ > phymib_->CWMax)
			cw_ = phymib_->CWMax;
	}

	inline void rst_cw() { cw_ = phymib_->CWMin; }

	inline void set_nav(u_int16_t us) {
		double now = Scheduler::instance().clock();
		double t = us * 1e-6;

		if((now + t) > nav_) {
			nav_ = now + t;
			if(mhNav_.busy())
				mhNav_.stop();
			mhNav_.start(t);
		}
	}

	inline u_int16_t usec(double t) {
		u_int16_t us = (u_int16_t) (t *= 1e6);
		if(us < t)
			us++;
		return us;
	}

protected:
	PHY_MIB		*phymib_;
	MAC_MIB		*macmib_;

private:
	/*
	 * Mac Timers
	 */
	IFTimer		mhIF_;		// interface timer
	NavTimer	mhNav_;		// NAV timer
	RxTimer		mhRecv_;		// incoming packets
	TxTimer		mhSend_;		// outgoing packets

	DeferTimer	mhDefer_;	// defer timer
	BackoffTimer	mhBackoff_;	// backoff timer

	/* ============================================================
	   Internal MAC State
	   ============================================================ */
	double		nav_;		// Network Allocation Vector

	MacState	rx_state_;	// incoming state (MAC_RECV or MAC_IDLE)
	MacState	tx_state_;	// outgoint state
#if 1
	int		tx_active_;	// transmitter is ACTIVE
#endif

	Packet		*pktRTS_;	// outgoing RTS packet
	Packet		*pktCTRL_;	// outgoing non-RTS packet

	u_int32_t	cw_;		// Contention Window
	u_int32_t	ssrc_;		// STA Short Retry Count
	u_int32_t	slrc_;		// STA Long Retry Count
	double		sifs_;		// Short Interface Space
	double		pifs_;		// PCF Interframe Space
	double		difs_;		// DCF Interframe Space
	double		eifs_;		// Extended Interframe Space
	double		tx_sifs_;
	double		tx_pifs_;
	double		tx_difs_;

	int		min_frame_len_;

	NsObject*	logtarget_;

	/* ============================================================
	   Duplicate Detection state
	   ============================================================ */
	u_int16_t	sta_seqno_;	// next seqno that I'll use
	int		cache_node_count_;
	Host		*cache_;
};


#endif /* __mac_80211_h__ */


/*
 * Copyright (c) 1991-1994 Regents of the University of California.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and the Network Research Group at
 *      Lawrence Berkeley Laboratory.
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/net.h,v 1.1 1997/05/14 00:42:12 mccanne Exp $ (LBL)
 */

#ifndef vic_net_h
#define vic_net_h

#include "inet.h"
#include "Tcl.h"
#include "iohandler.h"
#include "timer.h"

struct pktbuf;

class Crypt;

/* win95 #define's this...*/
#ifdef interface
#undef interface
#endif

class Network : public TclObject {
    public:
	Network();
	virtual ~Network();
	virtual int command(int argc, const char*const* argv);
	virtual void send(u_char* buf, int len);
	virtual void send(const pktbuf*);
	virtual int recv(u_char* buf, int len, u_int32_t& from);
	inline int rchannel() const { return (rsock_); }
	inline int schannel() const { return (ssock_); }
	inline u_int32_t addr() const { return (addr_); }
	inline u_int32_t interface() const { return (local_); }
	inline int port() const { return (port_); }
	inline int ttl() const { return (ttl_); }
	inline int noloopback_broken() const { return (noloopback_broken_); }
	virtual void reset();
	static void nonblock(int fd);
	inline Crypt* crypt() const { return (crypt_); }
    protected:
	virtual void dosend(u_char* buf, int len, int fd);
	virtual int dorecv(u_char* buf, int len, u_int32_t& from, int fd);

	u_int32_t addr_;
	u_int32_t local_;
	u_short lport_;
	u_short port_;
	int ttl_;
	int rsock_;
	int ssock_;

	int noloopback_broken_;
	
	Crypt* crypt_;

	static u_char* wrkbuf_;
	static int wrkbuflen_;
	static void expand_wrkbuf(int len);
	static int cpmsg(const pktbuf*);
};

class DataHandler;
class CtrlHandler;

class SessionHandler {
 public:
	virtual void recv(DataHandler*) = 0;
	virtual void recv(CtrlHandler*) = 0;
	virtual void announce(CtrlHandler*) = 0;
};

class DataHandler : public IOHandler {
    public:
	DataHandler* next;
	inline DataHandler() : next(0), sm_(0), net_(0) {}
	virtual void dispatch(int mask);
	inline Network* net() const { return (net_); }
	virtual void net(Network* net) {
		unlink();
		net_ = net;
		if (net != 0)
			link(net->rchannel(), TCL_READABLE);
	}
	inline int recv(u_char* bp, int len, u_int32_t& addr) {
		return (net_->recv(bp, len, addr));
	}
	inline void send(u_char* bp, int len) {
		net_->send(bp, len);
	}
	inline void manager(SessionHandler* sm) { sm_ = sm; }
    protected:
	SessionHandler* sm_;
	Network* net_;
};

/*
 * Parameters controling the RTCP report rate timer.
 */
#define CTRL_SESSION_BW_FRACTION (0.05)
#define CTRL_MIN_RPT_TIME (5.)
#define CTRL_SENDER_BW_FRACTION (0.25)
#define CTRL_RECEIVER_BW_FRACTION (1. - CTRL_SENDER_BW_FRACTION)
#define CTRL_SIZE_GAIN (1./8.)

class CtrlHandler : public DataHandler, public Timer {
    public:
	CtrlHandler();
	virtual void dispatch(int mask);
	virtual void net(Network* net);
	virtual void timeout();
	void adapt(int nsrc, int nrr, int we_sent);
	void sample_size(int cc);
	inline double rint() const { return (rint_); }
	inline void bandwidth(double bw) { ctrl_inv_bw_ = 1 / bw; }
 protected:
	void schedule_timer();
	double ctrl_inv_bw_;
	double ctrl_avg_size_;	/* (estimated) average size of ctrl packets */
	double rint_;		/* current session report rate (in ms) */
};

#endif

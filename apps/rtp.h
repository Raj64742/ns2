/*
 * Copyright (c) @ Regents of the University of California.
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
 * 	This product includes software developed by the MASH Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/apps/rtp.h,v 1.1 1996/12/19 03:22:45 mccanne Exp $
 */


#ifndef ns_rtp_h
#define ns_rtp_h

class RTPSource {
public:
	RTPSource* next;

	RTPSource(nsaddr_t addr);
	inline int addr() { return (addr_); }
	inline int np() { return (np_); }
	inline int snp() { return (snp_); }
	inline int ehsr() { return (ehsr_); }

	inline void np(int n) { np_ += n; }
	inline void snp(int n) { snp_ = n; }
	inline void ehsr(int n) { ehsr_ = n; }
protected:
	int addr_;
	int np_;
	int snp_;
	int ehsr_;
};

class RTPSession : public TclObject {
public:
	RTPSession();
	~RTPSession();
	virtual void recv(Packet* p);
	virtual void recv_ctrl(Packet* p);
	int command(int argc, const char*const* argv);
	int build_report(int bye);
	void localsrc_update(int);
protected:
	RTPSource* allsrcs_;
	RTPSource* localsrc_;
	int build_sdes();
	int build_bye();
	RTPSource* lookup(nsaddr_t);
	void enter(RTPSource*);
	int last_np_;
};

#endif

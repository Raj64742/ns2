/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996-1997 Regents of the University of California.
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/classifier/classifier.h,v 1.24 1999/09/17 22:47:50 heideman Exp $ (LBL)
 */

#ifndef ns_classifier_h
#define ns_classifier_h

#include "object.h"

class Packet;

class Classifier : public NsObject {
 public:
	Classifier();
	virtual ~Classifier();
	virtual void recv(Packet* p, Handler* h);
	int maxslot() const { return maxslot_; }
	inline NsObject* slot(int slot) {
		if ((slot >= 0) && (slot < nslot_))
			return slot_[slot];
		return 0;
	}
	int mshift(int val) { return ((val >> shift_) & mask_); }
	virtual NsObject* find(Packet*);
	virtual int classify(const Packet *);
	enum classify_ret {ONCE= -2, TWICE= -1};
 protected:
	void install(int slot, NsObject*);
	virtual void clear(int slot);
	virtual int getnxt(NsObject *);
	virtual int command(int argc, const char*const* argv);
	void alloc(int);
	NsObject** slot_;	/* table that maps slot number to a NsObject */
	int nslot_;
	int maxslot_;
	int offset_;		// offset for Packet::access()
	int shift_;
	int mask_;
	NsObject	*default_target_;
private:
	
	int off_ip_;            // XXX to be removed

};

#endif

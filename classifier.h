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
 */

#ifndef ns_classifier_h
#define ns_classifier_h

#include "object.h"

class Packet;
class Classifier : public NsObject {
 public:
	~Classifier();
	void recv(Packet*, Handler* h = 0);
	int maxslot() const { return maxslot_; }
	inline NsObject* slot(int slot) {
		if ((slot >= 0) || (slot < nslot_))
			return slot_[slot];
		return (NsObject*)NULL;
	}
	NsObject* find(Packet*);
	virtual int classify(Packet *const) = 0;
 protected:
	Classifier();
	void install(int slot, NsObject*);
	void clear(int slot);
	virtual int command(int argc, const char*const* argv);
	void alloc(int);
	NsObject** slot_;	/* table that maps slot number to a NsObject */
	int nslot_;
	int maxslot_;

	/*XXX eventually the classifier should be made completely
	 * indepedent of ip.  it should just take offsets and
	 * masks.  We can do this as soon as we work out the interface
	 * between the packet-header templates and the otcl packet
	 * manager.  then the packet manager can create the right
	 * kind of classifier for any necessary context.
	 */
	int off_ip_;
};

#endif

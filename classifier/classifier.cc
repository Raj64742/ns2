/*
 * Copyright (c) 1996 Regents of the University of California.
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/classifier/classifier.cc,v 1.13 1997/12/19 22:20:11 bajaj Exp $";
#endif

#include <stdlib.h>
#include "config.h"
#include "classifier.h"
#include "packet.h"

static class ClassifierClass : public TclClass {
public:
	ClassifierClass() : TclClass("Classifier") {}
	TclObject* create(int, const char*const*) {
		return (new Classifier());
	}
} class_classifier;


Classifier::Classifier() : slot_(0), nslot_(0), maxslot_(-1)
	, shift_(0), mask_(0xffffffff)
{
	bind("offset_", &offset_);
	bind("shift_", &shift_);
	bind("mask_", &mask_);
	bind("off_ip_", &off_ip_); // XXX to be removed
}

int Classifier::classify(Packet *const p)
{
	int value = *((int*) p->access(offset_));
	return (value >> shift_) & mask_;
}

Classifier::~Classifier()
{
	delete slot_;
}

void Classifier::alloc(int slot)
{
	NsObject** old = slot_;
	int n = nslot_;
	if (old == 0)
		nslot_ = 32;
	while (nslot_ <= slot)
		nslot_ <<= 1;
	slot_ = new NsObject*[nslot_];
	memset(slot_, 0, nslot_ * sizeof(NsObject*));
	for (int i = 0; i < n; ++i)
		slot_[i] = old[i];
	delete old;
}


void Classifier::install(int slot, NsObject* p)
{
	if (slot >= nslot_)
		alloc(slot);
	slot_[slot] = p;
	if (slot >= maxslot_)
		maxslot_ = slot;
}

void Classifier::clear(int slot)
{
	slot_[slot] = 0;
	if (slot == maxslot_) {
		while (--maxslot_ >= 0 && slot_[maxslot_] == 0)
			;
	}
}

int Classifier::getnxt()
{
  int i;
  for (i=0; i < nslot_; i++)
    if (slot_[i]==0)
      return i;
  i=nslot_;
  alloc(nslot_);
  return i;
}

/*
 * objects only ever see "packet" events, which come either
 * from an incoming link or a local agent (i.e., packet source).
 */
void Classifier::recv(Packet* p, Handler*)
{
	NsObject* node = find(p);
	if (node == NULL) {
		/*
		 * XXX this should be "dropped" somehow.  Right now,
		 * these events aren't traced.
		 */
		Packet::free(p);
		return;
	}
	node->recv(p);
}

/*
 * perform the mapping from packet to object
 * perform upcall if no mapping
 */

NsObject* Classifier::find(Packet* p)
{
        NsObject* node = NULL;
        int cl = classify(p);
        if (cl < 0 || cl >= nslot_ || (node = slot_[cl]) == 0) { 
                Tcl::instance().evalf("%s no-slot %d", name(), cl);
                /*      
                 * Try again.  Maybe callback patched up the table.
                 */      
                cl = classify(p);
                if (cl < 0 || cl >= nslot_ || (node = slot_[cl]) == 0)
			return (NULL);
	}
	return (node);
}

int Classifier::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
	  /*
	   * $classifier alloc-port
	   */
	  if (strcmp(argv[1],"alloc-port") == 0) {
	    int slot;
	    slot=getnxt();
	    tcl.resultf("%u",slot);
	    return(TCL_OK);
	  }
	} else 	if (argc == 3) {
		/*
		 * $classifier clear $slot
		 */
		if (strcmp(argv[1], "clear") == 0) {
			int slot = atoi(argv[2]);
			clear(slot);
			return (TCL_OK);
		}
		/*
		 * $classifier installNext $node
		 */
		if (strcmp(argv[1], "installNext") == 0) {
			int slot = maxslot_ + 1;
			NsObject* node = (NsObject*)TclObject::lookup(argv[2]);
			install(slot, node);
			tcl.resultf("%u", slot);
			return TCL_OK;
		}
		if (strcmp(argv[1], "slot") == 0) {
			int slot = atoi(argv[2]);
			if ((slot >= 0) || (slot < nslot_)) {
				tcl.resultf("%s", slot_[slot]->name());
				return TCL_OK;
			}
			tcl.resultf("Classifier: no object at slot %d", slot);
			return (TCL_ERROR);
		}
	} else if (argc == 4) {
		/*
		 * $classifier install $slot $node
		 */
		if (strcmp(argv[1], "install") == 0) {
			int slot = atoi(argv[2]);
			NsObject* node = (NsObject*)TclObject::lookup(argv[3]);
			install(slot, node);
			return (TCL_OK);
		}
	}
	return (NsObject::command(argc, argv));
}

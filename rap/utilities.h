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
// utilities.h 
//      Miscellaneous useful definitions, including debugging routines.
// 
// Author:
//   Mohit Talwar (mohit@catarina.usc.edu)
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/rap/utilities.h,v 1.6 2005/07/27 01:13:44 tomh Exp $

#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <memory.h>

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"
#include "random.h"
#include "raplist.h"

// Functions...
extern FILE * DebugEnable(unsigned int nodeid);

// Print debug message if flag is enabled
extern void Debug (int debugFlag, FILE *log, char* format, ...); 



// Data structures
class DoubleListElem {
public:
	DoubleListElem() : prev_(0), next_(0) {}

	virtual ~DoubleListElem () {}

	DoubleListElem* next() const { return next_; }
	DoubleListElem* prev() const { return prev_; }

	virtual void detach() {
		if (prev_ != 0) prev_->next_ = next_;
		if (next_ != 0) next_->prev_ = prev_;
		prev_ = next_ = 0;
	}
	// Add new element s before this one
	virtual void insert(DoubleListElem *s) {
		s->next_ = this;
		s->prev_ = prev_;
		if (prev_ != 0) prev_->next_ = s;
		prev_ = s;
	}
	// Add new element s after this one
	virtual void append(DoubleListElem *s) {
		s->next_ = next_;
		s->prev_ = this;
		if (next_ != 0) next_->prev_ = s;
		next_ = s;
	}

private:
	DoubleListElem *prev_, *next_;
};

class DoubleList {
public:
	DoubleList() : head_(0), tail_(0) {}
	virtual ~DoubleList() {}
	virtual void destroy();
	DoubleListElem* head() { return head_; }
	DoubleListElem* tail() { return tail_; }

	void detach(DoubleListElem *e) {
		if (head_ == e)
			head_ = e->next();
		if (tail_ == e)
			tail_ = e->prev();
		e->detach();
	}
	void insert(DoubleListElem *src, DoubleListElem *dst) {
		dst->insert(src);
		if (dst == head_)
			head_ = src;
	}
	void append(DoubleListElem *src, DoubleListElem *dst) {
		dst->append(src);
		if (dst == tail_)
			tail_ = src;
	}
protected:
	DoubleListElem *head_, *tail_;
};


#endif // UTILITIES_H

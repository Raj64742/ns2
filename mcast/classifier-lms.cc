/*
 * Copyright (c) 2001 University of Southern California.
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
 **
 * Light-Weight Multicast Services (LMS), Reliable Multicast
 *
 * classifier-lms.cc
 *
 * This is the classifier used to pick out LMS packets entering a node.
 *
 * Christos Papadopoulos. 
 * christos@isi.edu
 */

#include "config.h"
#include "packet.h"
#include "ip.h"
#include "classifier.h"

class LmsClassifier : public Classifier {
public:
	LmsClassifier();
protected:
	int classify(Packet *const p)
		{
		hdr_cmn* h = HDR_CMN(p);

		if (h->ptype_ == PT_LMS)
			return 1;
		else	return 0;
		}
};

static class LmsClassifierClass : public TclClass {
public:
	LmsClassifierClass() : TclClass("Classifier/Lms") {}
	TclObject* create(int, const char*const*) {
		return (new LmsClassifier());
	}
} class_lms_classifier;

LmsClassifier::LmsClassifier() {};

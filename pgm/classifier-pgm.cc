#ifdef PGM

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
 * Pragmatic General Multicast (PGM), Reliable Multicast
 *
 * classifier-pgm.cc
 *
 * This is the classifier used to pick out PGM packets entering a node.
 *
 * Ryan S. Barnett, 2001
 * rbarnett@catarina.usc.edu
 */

#include "config.h"
#include "packet.h"
#include "ip.h"
#include "classifier.h"

class PgmClassifier : public Classifier {
public:
  PgmClassifier();
protected:

  int classify(Packet *const p)
  {
    hdr_cmn* h = HDR_CMN(p);

    if (h->ptype_ == PT_PGM) {
      return 1;
    }
    else {
      return 0;
    }
  }

};

static class PgmClassifierClass : public TclClass {
public:
	PgmClassifierClass() : TclClass("Classifier/Pgm") {}
	TclObject* create(int, const char*const*) {
		return (new PgmClassifier());
	}
} class_pgm_classifier;

PgmClassifier::PgmClassifier() {};

#endif

/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * classifier-port.h
 * Copyright (C) 1999 by USC/ISI
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/classifier-port.h,v 1.7 2001/12/20 00:15:33 haldar Exp $ (USC/ISI)
 */

#ifndef ns_classifier_port_h
#define ns_classifier_port_h

#include "config.h"
#include "packet.h"
#include "ip.h"
#include "classifier.h"

class PortClassifier : public Classifier {
protected:
	int classify(Packet *p);
// 	void clear(int slot);
// 	int getnxt(NsObject *);
//	int command(int argc, const char*const* argv);
// 	int reserved_;
};

class ReservePortClassifier : public PortClassifier {
public:
	ReservePortClassifier() : PortClassifier(), reserved_(0) {}
protected:
	void clear(int slot);
	int getnxt(NsObject *);
	int command(int argc, const char*const* argv);
	int reserved_;
};

#endif /* ns_classifier_port_h */

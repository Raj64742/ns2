/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * classifier-port.cc
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/classifier-port.cc,v 1.7 2001/12/20 00:15:33 haldar Exp $ (USC/ISI)
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/classifier-port.cc,v 1.7 2001/12/20 00:15:33 haldar Exp $";
#endif

#include "classifier-port.h"

int PortClassifier::classify(Packet *p) 
{
	// Port classifier returns the destination port.  No shifting
	// or masking is required since in the 32-bit addressing,
	// ports are stored in a seperate variable.
	hdr_ip* iph = hdr_ip::access(p);
	return iph->dport();
};

static class PortClassifierClass : public TclClass {
public:
	PortClassifierClass() : TclClass("Classifier/Port") {}
	TclObject* create(int, const char*const*) {
		return (new PortClassifier());
	}
} class_address_classifier;

static class ReservePortClassifierClass : public TclClass {
public:
        ReservePortClassifierClass() : TclClass("Classifier/Port/Reserve") {}
        TclObject* create(int, const char*const*) {
                return (new ReservePortClassifier());
        }
} class_reserve_port_classifier;

int ReservePortClassifier::command(int argc, const char*const* argv)
{
        if (argc == 3 && strcmp(argv[1],"reserve-port") == 0) {
                reserved_ = atoi(argv[2]);
                alloc((maxslot_ = reserved_ - 1));
                return(TCL_OK);
        }
        return (Classifier::command(argc, argv));
}

void ReservePortClassifier::clear(int slot)
{
        slot_[slot] = 0;
        if (slot == maxslot_) {
                while (--maxslot_ >= reserved_ && slot_[maxslot_] == 0)
                        ;
        }
}
 
int ReservePortClassifier::getnxt(NsObject *nullagent)
{
        int i;
        for (i=reserved_; i < nslot_; i++)
                if (slot_[i]==0 || slot_[i]==nullagent)
                        return i;
        i=nslot_;
        alloc(nslot_); 
        return i;
}


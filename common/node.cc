/*
 * Copyright (c) 1994 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/node.cc,v 1.1.1.1 1996/12/19 03:22:45 mccanne Exp $ (LBL)";
#endif

#include "node.h"

/*
 * $node addr
 * $node agent $port
 */
int Node::command(int argc, const char*const* argv)
{
	/*XXX*/
	return (NsObject::command(argc, argv));
}

/*
 * Packets may be handed to Nodes at sheduled points
 * in time since a node is an event handler and a packet
 * is an event.  Packets should be the only type of event
 * scheduled on a node so we can carry out the cast below.
 */
void Node::handle(Event* e)
{
	recv((Packet*)e);
}

static class ConnectorClass : public TclClass {
public:
	ConnectorClass() : TclClass("connector") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new Connector);
	}
} class_connector;

Connector::Connector() : target_(0)
{
}


int Connector::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	/*XXX*/
	if (argc == 2) {
		if (strcmp(argv[1], "target") == 0) {
			if (target_ != 0)
				tcl.result(target_->name());
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "target") == 0) {
			target_ = (Node*)TclObject::lookup(argv[2]);
			if (target_ == 0) {
				tcl.resultf("no such node %s", argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
	}
	return (Node::command(argc, argv));
}

void Connector::recv(Packet* p, Handler* h)
{
	target_->recv(p, h);
}

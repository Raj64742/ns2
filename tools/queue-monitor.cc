/*
 * Copyright (c) 1997 Regents of the University of California.
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
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tools/queue-monitor.cc,v 1.4 1997/03/29 01:42:58 mccanne Exp $";
#endif

#include "integrator.h"
#include "connector.h"
#include "packet.h"
#include "ip.h"

class QueueMonitor : public Integrator {
 public:
	QueueMonitor() : size_(0) {
		bind("size_", &size_);
		bind("off_cmn_", &off_cmn_);
	}
	void in(Packet*);
	void out(Packet*);
	//	int command(int argc, const char*const* argv);
protected:
	int size_;
	int off_cmn_;
};

static class QueueMonitorClass : public TclClass {
public:
	QueueMonitorClass() : TclClass("QueueMonitor") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new QueueMonitor());
	}
} queue_monitor_class;

void QueueMonitor::in(Packet* p)
{
	size_ += ((hdr_cmn*)p->access(off_cmn_))->size();
	double now = Scheduler::instance().clock();
	newPoint(now, double(size_));
}

void QueueMonitor::out(Packet* p)
{
	size_ -= ((hdr_cmn*)p->access(off_cmn_))->size();
	double now = Scheduler::instance().clock();
	newPoint(now, double(size_));
}

class SnoopQueue : public Connector {
 public:
	SnoopQueue() : qm_(0) {}
	int command(int argc, const char*const* argv) {
		if (argc == 3) {
			if (strcmp(argv[1], "set-monitor") == 0) {
				qm_ = (QueueMonitor*)
					TclObject::lookup(argv[2]);
				return (TCL_OK);
			}
		}
		return (Connector::command(argc, argv));
	}
 protected:
	QueueMonitor* qm_;
};

class SnoopQueueIn : public SnoopQueue {
 public:
	void recv(Packet* p, Handler* h) {
		qm_->in(p);
		send(p, h);
	}
};

class SnoopQueueOut : public SnoopQueue {
 public:
	void recv(Packet* p, Handler* h) {
		qm_->out(p);
		send(p, h);
	}
};

static class SnoopQueueInClass : public TclClass {
public:
	SnoopQueueInClass() : TclClass("SnoopQueue/In") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new SnoopQueueIn());
	}
} snoopq_in_class;

static class SnoopQueueOutClass : public TclClass {
public:
	SnoopQueueOutClass() : TclClass("SnoopQueue/Out") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new SnoopQueueOut());
	}
} snoopq_out_class;


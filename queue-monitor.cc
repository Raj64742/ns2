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
 *      This product includes software developed by the MASH Research
 *      Group at the University of California Berkeley.
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
    "@(#) $Header: /home/barad-dur/b/grad/hari/src/ns/RCS/queue-monitor.cc,v 1.1 1997/03/24 22:
59:47 hari Exp hari $";
#endif

#include "queue-monitor.h"

int QueueMonitor::command(int argc, const char*const* argv) {
        Tcl& tcl = Tcl::instance();

        if (argc == 2) {
                if (strcmp(argv[1], "get-bytes-integrator") == 0) {
                        tcl.resultf("%s", bytesInt_->name());
                        return (TCL_OK);
                }
                if (strcmp(argv[1], "get-pkts-integrator") == 0) {
                        tcl.resultf("%s", pktsInt_->name());
                        return (TCL_OK);
                }
        }

        if (argc == 3) {
                if (strcmp(argv[1], "set-bytes-integrator") == 0) {
			bytesInt_ = (BytesIntegrator *)
				TclObject::lookup(argv[2]);
                        return (TCL_OK);
                }
                if (strcmp(argv[1], "set-pkts-integrator") == 0) {
			pktsInt_ = (PktsIntegrator *)
				TclObject::lookup(argv[2]);
                        return (TCL_OK);
                }
        }
}

static class QueueMonitorClass : public TclClass {
 public:
        QueueMonitorClass() : TclClass("QueueMonitor") {}
        TclObject* create(int argc, const char*const* argv) {
                return (new QueueMonitor());
        }
} queue_monitor_class;

static class BytesIntegratorClass : public TclClass {
 public:
        BytesIntegratorClass() : TclClass("BytesIntegrator") {}
        TclObject* create(int argc, const char*const* argv) {
                return (new BytesIntegrator());
        }
} bytes_integrator_class;

static class PktsIntegratorClass : public TclClass {
 public:
        PktsIntegratorClass() : TclClass("PktsIntegrator") {}
        TclObject* create(int argc, const char*const* argv) {
                return (new PktsIntegrator());
        }
} pkts_integrator_class;


void QueueMonitor::in(Packet* p)
{
	size_ += ((hdr_cmn*)p->access(off_cmn_))->size();
        pkts_++;
        double now = Scheduler::instance().clock();
        bytesInt_->newPoint(now, double(size_));
        pktsInt_->newPoint(now, double(pkts_));
}

void QueueMonitor::out(Packet* p)
{
        size_ -= ((hdr_cmn*)p->access(off_cmn_))->size();
        pkts_--;
        double now = Scheduler::instance().clock();
        bytesInt_->newPoint(now, double(size_));
        pktsInt_->newPoint(now, double(pkts_));
}

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

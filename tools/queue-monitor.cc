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
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tools/queue-monitor.cc,v 1.17 1997/08/10 07:49:47 mccanne Exp $";
#endif

#include "queue-monitor.h"

int QueueMonitor::command(int argc, const char*const* argv)
{
        Tcl& tcl = Tcl::instance();

        if (argc == 2) {
                if (strcmp(argv[1], "get-bytes-integrator") == 0) {
			if (bytesInt_)
				tcl.resultf("%s", bytesInt_->name());
			else
				tcl.resultf("");
                        return (TCL_OK);
                }
                if (strcmp(argv[1], "get-pkts-integrator") == 0) {
			if (pktsInt_)
				tcl.resultf("%s", pktsInt_->name());
			else
				tcl.resultf("");
                        return (TCL_OK);
                }
                if (strcmp(argv[1], "get-delay-samples") == 0) {
			if (delaySamp_)
				tcl.resultf("%s", delaySamp_->name());
			else
				tcl.resultf("");
                        return (TCL_OK);
                }
        }

        if (argc == 3) {
                if (strcmp(argv[1], "set-bytes-integrator") == 0) {
			bytesInt_ = (Integrator *)
				TclObject::lookup(argv[2]);
			if (bytesInt_ == NULL)
				return (TCL_ERROR);
                        return (TCL_OK);
                }
                if (strcmp(argv[1], "set-pkts-integrator") == 0) {
			pktsInt_ = (Integrator *)
				TclObject::lookup(argv[2]);
			if (pktsInt_ == NULL)
				return (TCL_ERROR);
                        return (TCL_OK);
                }
                if (strcmp(argv[1], "set-delay-samples") == 0) {
			delaySamp_ = (Samples*)
				TclObject::lookup(argv[2]);
			if (delaySamp_ == NULL)
				return (TCL_ERROR);
                        return (TCL_OK);
                }
		if (strcmp(argv[1], "trace") == 0) {
			int mode;
			const char* id = argv[2];
			channel_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
						if (channel_ == 0) {
				tcl.resultf("trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
        }
	if (argc == 4) {
		if (strcmp(argv[1], "set-src-dst") == 0) {
			srcId_ = atoi(argv[2]);
			dstId_ = atoi(argv[3]);
			return (TCL_OK);
		}
	}
	return TclObject::command(argc, argv);	// else control reaches end of
						// non-void function, see? :-)
}

static class QueueMonitorClass : public TclClass {
 public:
        QueueMonitorClass() : TclClass("QueueMonitor") {}
        TclObject* create(int, const char*const*) {
                return (new QueueMonitor());
        }
} queue_monitor_class;

void
QueueMonitor::printStats() {
	char wrk[500];
	int n;
	double now = Scheduler::instance().clock();
	sprintf(wrk, "%-6.3f %d %d %d %d", now, srcId_, dstId_, size_, pkts_);
	n = strlen(wrk);
	wrk[n] = '\n';
	wrk[n+1] = 0;
	(void)Tcl_Write(channel_, wrk, n+1);
	wrk[n] = 0;
}	

// packet arrival to a queue
void QueueMonitor::in(Packet* p)
{
	hdr_cmn* hdr = (hdr_cmn*)p->access(off_cmn_);
        double now = Scheduler::instance().clock();
	int pktsz = hdr->size();

	barrivals_ += pktsz;
	parrivals_++;
	size_ += pktsz;
	pkts_++;
	if (bytesInt_)
		bytesInt_->newPoint(now, double(size_));
	if (pktsInt_)
		pktsInt_->newPoint(now, double(pkts_));
	if (delaySamp_)
		hdr->timestamp() = now;
	if (channel_)
		printStats();
}

void QueueMonitor::out(Packet* p)
{
	hdr_cmn* hdr = (hdr_cmn*)p->access(off_cmn_);
        double now = Scheduler::instance().clock();
	int pktsz = hdr->size();

        size_ -= pktsz;
        pkts_--;
	bdepartures_ += pktsz;
	pdepartures_++;
	if (bytesInt_)
		bytesInt_->newPoint(now, double(size_));
	if (pktsInt_)
		pktsInt_->newPoint(now, double(pkts_));
	if (delaySamp_)
		delaySamp_->newPoint(now - hdr->timestamp());
	if (channel_)
		printStats();
}

void QueueMonitor::drop(Packet* p)
{
	hdr_cmn* hdr = (hdr_cmn*)p->access(off_cmn_);
        double now = Scheduler::instance().clock();
	int pktsz = hdr->size();

	size_ -= pktsz;
	pkts_--;
	bdrops_ += pktsz;
	pdrops_++;
	if (bytesInt_)
		bytesInt_->newPoint(now, double(size_));
	if (pktsInt_)
		pktsInt_->newPoint(now, double(pkts_));
	if (channel_)
		printStats();
}

static class SnoopQueueInClass : public TclClass {
public:
        SnoopQueueInClass() : TclClass("SnoopQueue/In") {}
        TclObject* create(int, const char*const*) {
                return (new SnoopQueueIn());
        }
} snoopq_in_class;

static class SnoopQueueOutClass : public TclClass {
public:
        SnoopQueueOutClass() : TclClass("SnoopQueue/Out") {}
        TclObject* create(int, const char*const*) {
                return (new SnoopQueueOut());
        }
} snoopq_out_class;

static class SnoopQueueDropClass : public TclClass {
public:
        SnoopQueueDropClass() : TclClass("SnoopQueue/Drop") {}
        TclObject* create(int, const char*const*) {
                return (new SnoopQueueDrop());
        }
} snoopq_drop_class;

static class SnoopQueueEDropClass : public TclClass {
public:
        SnoopQueueEDropClass() : TclClass("SnoopQueue/EDrop") {}
        TclObject* create(int, const char*const*) {
                return (new SnoopQueueEDrop);
        }
} snoopq_edrop_class;

static class QueueMonitorEDClass : public TclClass {
 public: 
        QueueMonitorEDClass() : TclClass("QueueMonitor/ED") {}
        TclObject* create(int, const char*const*) { 
                return (new EDQueueMonitor);
        }       
} queue_monitor_ed_class;

/*
 * a 'QueueMonitorCompat', which is used by the compat
 * code to produce the link statistics used available in ns-1
 *
 * in ns-1, the counters are the number of departures
 */

#include "ip.h"
QueueMonitorCompat::QueueMonitorCompat()
{
	bind("off_ip_", &off_ip_);
}

void QueueMonitorCompat::out(Packet* pkt)
{
	hdr_ip* iph = (hdr_ip*)pkt->access(off_ip_);
	int fid = iph->flowid();
	// printf("QueueMonitorCompat::out(), fid=%d\n", fid);
	bytes_[fid] += ((hdr_cmn*)pkt->access(off_cmn_))->size();
	pkts_[fid]++;
	QueueMonitor::out(pkt);
}

void QueueMonitorCompat::in(Packet* pkt)
{
	//
	// we're not keeping per-class running queue lengths, so
	// don't really have to do anything here
	//
	QueueMonitor::in(pkt);
}

void QueueMonitorCompat::drop(Packet* pkt)
{

	hdr_ip* iph = (hdr_ip*)pkt->access(off_ip_);
	int fid = iph->flowid();
	++drops_[fid];
	QueueMonitor::drop(pkt);
}

int QueueMonitorCompat::command(int argc, const char*const* argv)
{
        Tcl& tcl = Tcl::instance();
	int fid;
	if (argc == 3) {
		if (strcmp(argv[1], "bytes") == 0) {
			tcl.resultf("%d", bytes_[fid = atoi(argv[2])]);
			return TCL_OK;
		} else if (strcmp(argv[1], "pkts") == 0) {
			tcl.resultf("%d", pkts_[fid = atoi(argv[2])]);
			return TCL_OK;
		} else if (strcmp(argv[1], "drops") == 0) {
			tcl.resultf("%d", drops_[fid = atoi(argv[2])]);
			return TCL_OK;
		};
	}
	return (QueueMonitor::command(argc, argv));
}

static class QueueMonitorCompatClass : public TclClass {
 public: 
        QueueMonitorCompatClass() : TclClass("QueueMonitor/Compat") {}
        TclObject* create(int, const char*const*) { 
                return (new QueueMonitorCompat);
        }       
} queue_monitor_compat_class;

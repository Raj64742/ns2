/*
 * Copyright (c) 1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/flowmon.cc,v 1.4 1997/06/12 22:53:11 kfall Exp $ (LBL)";
#endif

//
// flow-monitor, basically a port from the ns-1 flow manager,
// but architected somewhat differently to better fit the ns-2
// object framework -KF
//

#include <stdlib.h>
#include "config.h"
#include "queue-monitor.h"
#include "classifier.h"
#include "ip.h"

// for convenience, we need most of the stuff in a QueueMonitor
// plus some flow info which looks like pkt header info

class Flow : public EDQueueMonitor {
public:
	Flow() : src_(-1), dst_(-1), fid_(-1), type_(-1) {
		bind("off_ip_" ,&off_ip_);
	}
	nsaddr_t src() const { return (src_); }
	nsaddr_t dst() const { return (dst_); }
	int flowid() const { return (fid_); }
	int ptype() const { return (type_); }
	void setfields(Packet *p) {
		hdr_ip* hdr = (hdr_ip*)p->access(off_ip_);
		hdr_cmn* chdr = (hdr_cmn*)p->access(off_cmn_);
		src_ = hdr->src();
		dst_ = hdr->dst();
		fid_ = hdr->flowid();
		type_ = chdr->ptype();
	}
protected:
	int		off_ip_;
	nsaddr_t	src_;
	nsaddr_t	dst_;
	int		fid_;
	int		type_;
};

/*
 * flow monitoring is performed like queue-monitoring with
 * a classifier to demux by flow
 */

class FlowMon : public EDQueueMonitor {
public:
	FlowMon();
	void in(Packet*);
	void out(Packet*);
	void drop(Packet*);
	void edrop(Packet*);
	int command(int argc, const char*const* argv);
protected:
	void	dumpflows();
	void	dumpflow(Tcl_Channel, Flow*);
	void	fformat(Flow*);
	char*	flow_list();

	Classifier*	classifier_;
	Tcl_Channel	channel_;
	char	wrk_[256];
};

FlowMon::FlowMon() : classifier_(NULL), channel_(NULL)
{
}

void
FlowMon::in(Packet *p)
{
	Flow* desc;
	EDQueueMonitor::in(p);
	if ((desc = ((Flow*)classifier_->find(p))) != NULL) {
		desc->setfields(p);
		desc->in(p);
	}
}
void
FlowMon::out(Packet *p)
{
	Flow* desc;
	EDQueueMonitor::out(p);
	if ((desc = ((Flow*)classifier_->find(p))) != NULL) {
		desc->setfields(p);
		desc->out(p);
	}
}

void
FlowMon::drop(Packet *p)
{
	Flow* desc;
	EDQueueMonitor::drop(p);
	if ((desc = ((Flow*)classifier_->find(p))) != NULL) {
		desc->setfields(p);
		desc->drop(p);
	}
}

void
FlowMon::edrop(Packet *p)
{
	Flow* desc;
	EDQueueMonitor::edrop(p);
	if ((desc = ((Flow*)classifier_->find(p))) != NULL) {
		desc->setfields(p);
		desc->edrop(p);
	}
}

void
FlowMon::dumpflows()
{
	register i, j = classifier_->maxslot();
	Flow* f;

	for (i = 0; i <= j; i++) {
		if ((f = (Flow*)classifier_->index(i)) != NULL)
			dumpflow(channel_, f);
	}
}

char*
FlowMon::flow_list()
{
	register i, j = classifier_->maxslot();
	Flow* f;
	register char* p = wrk_;
	register char* q;
	const register char* z;
	q = p + sizeof(wrk_) - 2;
	*p = '\0';
	for (i = 0; i <= j; i++) {
		if ((f = (Flow*)classifier_->index(i)) != NULL) {
			z = f->name();
			while (*z && p < q)
				*p++ = *z++;
			*p++ = ' ';
		}
	}
	if (p != wrk_)
		*--p = '\0';
	return (wrk_);
}

void
FlowMon::fformat(Flow* f)
{
	double now = Scheduler::instance().clock();
	sprintf(wrk_, "%8.3f %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
		now,
		f->flowid(),	// flowid
		0,		// category
		f->ptype(),	// type (from trace header)
		f->flowid(),	// flowid (formerly class)
		f->src(),
		f->dst(),
		f->parrivals(),	// arrivals this flow (pkts)
		f->barrivals(),	// arrivals this flow (bytes)
		f->epdrops(),	// early drops this flow (pkts)
		f->ebdrops(),	// early drops this flow (bytes)
		parrivals(),	// all arrivals (pkts)
		barrivals(),	// all arrivals (bytes)
		epdrops(),	// total early drops (pkts)
		ebdrops(),	// total early drops (bytes)
		pdrops(),	// total drops (pkts)
		bdrops(),	// total drops (bytes)
		f->pdrops(),	// drops this flow (pkts) [includes edrops]
		f->bdrops()	// drops this flow (bytes) [includes edrops]
	);
};

void
FlowMon::dumpflow(Tcl_Channel tc, Flow* f)
{
	fformat(f);
	if (tc != 0) {
		int n = strlen(wrk_);
		wrk_[n++] = '\n';
		wrk_[n] = '\0';
		(void)Tcl_Write(tc, wrk_, n);
		wrk_[n-1] = '\0';
	}
}

int
FlowMon::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "classifier") == 0) {
			if (classifier_)
				tcl.resultf("%s", classifier_->name());
			else
				tcl.resultf("");
			return (TCL_OK);
		}
		if (strcmp(argv[1], "dump") == 0) {
			dumpflows();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "flows") == 0) {
			tcl.resultf("%s", flow_list());
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "classifier") == 0) {
			classifier_ = (Classifier*)
				TclObject::lookup(argv[2]);
			if (classifier_ == NULL)
				return (TCL_ERROR);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			channel_ = Tcl_GetChannel(tcl.interp(),
				(char*) id, &mode);
			if (channel_ == NULL) {
				tcl.resultf("FlowMon (%s): can't attach %s for writing",
					name(), id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
	}
	return (EDQueueMonitor::command(argc, argv));
}

static class FlowMonitorClass : public TclClass {
 public:
	FlowMonitorClass() : TclClass("QueueMonitor/ED/Flowmon") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new FlowMon);
	}
} flow_monitor_class;

static class FlowClass : public TclClass {
 public:
	FlowClass() : TclClass("QueueMonitor/ED/Flow") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new Flow);
	}
} flow_class;

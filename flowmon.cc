/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
 *
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
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/flowmon.cc,v 1.21 2000/09/01 03:04:05 haoboy Exp $ (LBL)";
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
#include "flags.h"
#include "random.h"

// for convenience, we need most of the stuff in a QueueMonitor
// plus some flow info which looks like pkt header info

class Flow : public EDQueueMonitor {
public:
	Flow() : src_(-1), dst_(-1), fid_(-1), type_(PT_NTYPE) {
		bind("src_", &src_);
		bind("dst_", &dst_);
		bind("flowid_", &fid_);
	}
	nsaddr_t src() const { return (src_); }
	nsaddr_t dst() const { return (dst_); }
	int flowid() const { return (fid_); }
	packet_t ptype() const { return (type_); }
	void setfields(Packet *p) {
		hdr_ip* hdr = hdr_ip::access(p);
		hdr_cmn* chdr = hdr_cmn::access(p);
		src_ = hdr->saddr();
		dst_ = hdr->daddr();
		fid_ = hdr->flowid();
		type_ = chdr->ptype();
	}
	virtual void tagging(Packet *) {}
protected:
	nsaddr_t	src_;
	nsaddr_t	dst_;
	int		fid_;
	packet_t	type_;
};

/* TaggerTBFlow will use Token Bucket to check whehter the data flow stays in
 * the pre-set profile and mark it as In or Out accordingly.
 * By Yun Wang, based on Wenjia's algorithm.
 */
class TaggerTBFlow : public Flow {
public:
	TaggerTBFlow() : target_rate_(0.0), time_last_sent_(0.0),
			total_in(0.0), total_out(0.0)
	{
		bind_bw("target_rate_", &target_rate_);
		bind("bucket_depth_", &bucket_depth_);
		bind("tbucket_", &tbucket_);
	}
	void tagging(Packet *p) {
	    hdr_cmn* hdr = hdr_cmn::access(p);
            double now = Scheduler::instance().clock();
            double time_elapsed;

            time_elapsed      = now - time_last_sent_;
            tbucket_ += time_elapsed * target_rate_ / 8.0;
            if (tbucket_ > bucket_depth_)
                tbucket_ = bucket_depth_;         /* never overflow */

      	    if ((double)hdr->size_ < tbucket_ ) {
		hdr_flags::access(p)->pri_=1; //Tag the packet as In.
          	tbucket_ -= hdr->size_;
                total_in += 1;
            }
      	    else {
                total_out += 1;
      	    }
	    time_last_sent_ = now;
	}
protected:
	/* User defined parameters */
	double	target_rate_;		//predefined flow rate in bytes/sec
	double	bucket_depth_;		//depth of the token bucket

	double 	tbucket_;
	/* Dynamic state variables */
	double 	time_last_sent_;
	double	total_in;
	double	total_out;
};

/* TaggerTSWFlow will use Time Slide Window to check whehter the data flow 
 * stays in the pre-set profile and mark it as In or Out accordingly.
 * By Yun Wang, based on Wenjia's algorithm.
 */
class TaggerTSWFlow : public Flow {
public:
	TaggerTSWFlow() : target_rate_(0.0), avg_rate_(0.0), 
			t_front_(0.0), total_in(0.0), total_out(0.0) 
	{
		bind_bw("target_rate_", &target_rate_);
		bind("win_len_", &win_len_);
		bind_bool("wait_", &wait_);
	}
	void tagging(Packet *);
	void run_rate_estimator(Packet *p, double now){

	        hdr_cmn* hdr = hdr_cmn::access(p);
		double bytes_in_tsw = avg_rate_ * win_len_;
		double new_bytes    = bytes_in_tsw + hdr->size_;
		avg_rate_ = new_bytes / (now - t_front_ + win_len_);
		t_front_  = now;
	}
protected:
	/* User-defined parameters. */
	double	target_rate_;		//predefined flow rate in bytes/sec
	double	win_len_;		//length of the slide window
	double	avg_rate_;		//average rate
	double	t_front_;
	int	count;
	int	wait_;

	/* Counters for In/Out packets. */
	double 	total_in;
	double 	total_out;
};

void TaggerTSWFlow::tagging(Packet *pkt)
{
        double now = Scheduler::instance().clock();;
        double  p, prob, u;
        int count1;
        int retVal;

        run_rate_estimator(pkt, now);

        if (avg_rate_ <= target_rate_) {
                prob = 0;
                retVal = 0;
        }
        else {
               prob = (avg_rate_ - target_rate_) / avg_rate_;
               p    = prob;
               count1 = count;

               if ( p < 0.5) {
                 if (wait_) {
                   if (count1 * p < 1)
                     p = 0;
                   else if (count1 * p < 2)
                     p /= (2 - count1 *p);
                   else
                     p = 1;
                 }
                 else if (!wait_) {
                   if (count1 * p < 1)
                     p /= (1 - count1 * p);
                   else
                     p = 1;
                 }
               }

                u = Random::uniform();
                if (u < p) {
                    retVal = 1;
                }
                else
                  retVal = 0;

//                if (trace_) {
//                  sprintf(trace_->buffer(), "Tagged prob %g, p %g, count %d",
//                          prob, p, count1);
//                  trace_->dump();
//                }
        }

        if (retVal == 0) {
            hdr_flags::access(pkt)->pri_=1; //Tag the packet as In.
            total_in = total_in + 1;
            ++count;
        }
        else {
            total_out = total_out + 1;
            count = 0;
        }
};

/* Tagger performes like the queue monitor with a classifier
 * to demux by flow and mark the packets based on the flow profile
 */
class Tagger : public EDQueueMonitor {
public:
	Tagger() : classifier_(NULL), channel_(NULL) {}
	void in(Packet *);
	int command(int argc, const char*const* argv);
protected:
        void    dumpflows();
        void    dumpflow(Tcl_Channel, Flow*);
        void    fformat(Flow*);
        char*   flow_list();

        Classifier*     classifier_;
        Tcl_Channel     channel_;

        char    wrk_[2048];     // big enough to hold flow list
};

void
Tagger::in(Packet *p)
{
        Flow* desc;
        EDQueueMonitor::in(p);
        if ((desc = ((Flow*)classifier_->find(p))) != NULL) {
                desc->setfields(p);
		desc->tagging(p);
                desc->in(p);
        }
} 

void
Tagger::dumpflows()
{
        register int i, j = classifier_->maxslot();
        Flow* f;

        for (i = 0; i <= j; i++) {
                if ((f = (Flow*)classifier_->slot(i)) != NULL)
                        dumpflow(channel_, f);
        }
}

char*
Tagger::flow_list()
{
        register const char* z;
        register int i, j = classifier_->maxslot();
        Flow* f;
        register char* p = wrk_;
        register char* q;
        q = p + sizeof(wrk_) - 2;
        *p = '\0';
        for (i = 0; i <= j; i++) {
                if ((f = (Flow*)classifier_->slot(i)) != NULL) {
                        z = f->name();
                        while (*z && p < q)
                                *p++ = *z++;
                        *p++ = ' ';
                }
                if (p >= q) {
                        fprintf(stderr, "Tagger:: flow list exceeded working buffer\n");
                        fprintf(stderr, "\t  recompile ns with larger Tagger::wrk_[] array\n");
                        exit (1);
                }
        }
        if (p != wrk_)
                *--p = '\0';
        return (wrk_);
}

void
Tagger::fformat(Flow* f)
{
        double now = Scheduler::instance().clock();
	// insane but true: egcs 2.96 doesnt think "int eprops() const"
	// matches %d
        sprintf(wrk_, "%8.3f %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                now,            // 1: time
                f->flowid(),    // 2: flowid
                0,              // 3: category
                f->ptype(),     // 4: type (from common header)
                f->flowid(),    // 5: flowid (formerly class)
                f->src(),       // 6: sender
                f->dst(),       // 7: receiver
                f->parrivals(), // 8: arrivals this flow (pkts)
                f->barrivals(), // 9: arrivals this flow (bytes)
                int(f->epdrops()),   // 10: early drops this flow (pkts)
                int(f->ebdrops()),   // 11: early drops this flow (bytes)
                parrivals(),    // 12: all arrivals (pkts)
                barrivals(),    // 13: all arrivals (bytes)
                int(epdrops()),      // 14: total early drops (pkts)
                int(ebdrops()),      // 15: total early drops (bytes)
                pdrops(),       // 16: total drops (pkts)
                bdrops(),       // 17: total drops (bytes)
                f->pdrops(),    // 18: drops this flow (pkts) [includes edrops]
                f->bdrops()     // 19: drops this flow (bytes) [includes edrops]
        );
}

void
Tagger::dumpflow(Tcl_Channel tc, Flow* f)
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
Tagger::command(int argc, const char*const* argv)
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
                        tcl.result(flow_list());
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
                                tcl.resultf("Tagger (%s): can't attach %s for writing",
                                        name(), id);
                                return (TCL_ERROR);
                        }
                        return (TCL_OK);
                }
        }
        return (EDQueueMonitor::command(argc, argv));
}

/*
 * flow monitoring is performed like queue-monitoring with
 * a classifier to demux by flow
 */

class FlowMon : public EDQueueMonitor {
public:
	FlowMon();
	void in(Packet*);	// arrivals
	void out(Packet*);	// departures
	void drop(Packet*);	// all drops (incl 
	void edrop(Packet*);	// "early" drops
	int command(int argc, const char*const* argv);
protected:
	void	dumpflows();
	void	dumpflow(Tcl_Channel, Flow*);
	void	fformat(Flow*);
	char*	flow_list();

	Classifier*	classifier_;
	Tcl_Channel	channel_;

	int enable_in_;		// enable per-flow arrival state
	int enable_out_;	// enable per-flow depart state
	int enable_drop_;	// enable per-flow drop state
	int enable_edrop_;	// enable per-flow edrop state
	char	wrk_[2048];	// big enough to hold flow list
};

FlowMon::FlowMon() : classifier_(NULL), channel_(NULL),
	enable_in_(1), enable_out_(1), enable_drop_(1), enable_edrop_(1)
{
	bind_bool("enable_in_", &enable_in_);
	bind_bool("enable_out_", &enable_out_);
	bind_bool("enable_drop_", &enable_drop_);
	bind_bool("enable_edrop_", &enable_edrop_);
}

void
FlowMon::in(Packet *p)
{
	Flow* desc;
	EDQueueMonitor::in(p);
	if (!enable_in_)
		return;
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
	if (!enable_out_)
		return;
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
	if (!enable_drop_)
		return;
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
	if (!enable_edrop_)
		return;
	if ((desc = ((Flow*)classifier_->find(p))) != NULL) {
		desc->setfields(p);
		desc->edrop(p);
	}
}

void
FlowMon::dumpflows()
{
	register int i, j = classifier_->maxslot();
	Flow* f;

	for (i = 0; i <= j; i++) {
		if ((f = (Flow*)classifier_->slot(i)) != NULL)
			dumpflow(channel_, f);
	}
}

char*
FlowMon::flow_list()
{
	register const char* z;
	register int i, j = classifier_->maxslot();
	Flow* f;
	register char* p = wrk_;
	register char* q;
	q = p + sizeof(wrk_) - 2;
	*p = '\0';
	for (i = 0; i <= j; i++) {
		if ((f = (Flow*)classifier_->slot(i)) != NULL) {
			z = f->name();
			while (*z && p < q)
				*p++ = *z++;
			*p++ = ' ';
		}
		if (p >= q) {
			fprintf(stderr, "FlowMon:: flow list exceeded working buffer\n");
			fprintf(stderr, "\t  recompile ns with larger FlowMon::wrk_[] array\n");
			exit (1);
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
#if defined(HAVE_INT64)
	sprintf(wrk_, "%8.3f %d %d %d %d %d %d " STRTOI64_FMTSTR " " STRTOI64_FMTSTR " %d %d " STRTOI64_FMTSTR " " STRTOI64_FMTSTR " %d %d %d %d %d %d",
#else /* no 64-bit int */
	sprintf(wrk_, "%8.3f %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
#endif
		now,		// 1: time
		f->flowid(),	// 2: flowid
		0,		// 3: category
		f->ptype(),	// 4: type (from common header)
		f->flowid(),	// 5: flowid (formerly class)
		f->src(),	// 6: sender
		f->dst(),	// 7: receiver
		f->parrivals(),	// 8: arrivals this flow (pkts)
		f->barrivals(),	// 9: arrivals this flow (bytes)
		f->epdrops(),	// 10: early drops this flow (pkts)
		f->ebdrops(),	// 11: early drops this flow (bytes)
		parrivals(),	// 12: all arrivals (pkts)
		barrivals(),	// 13: all arrivals (bytes)
		epdrops(),	// 14: total early drops (pkts)
		ebdrops(),	// 15: total early drops (bytes)
		pdrops(),	// 16: total drops (pkts)
		bdrops(),	// 17: total drops (bytes)
		f->pdrops(),	// 18: drops this flow (pkts) [includes edrops]
		f->bdrops()	// 19: drops this flow (bytes) [includes edrops]
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
			tcl.result(flow_list());
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
	TclObject* create(int, const char*const*) {
		return (new FlowMon);
	}
} flow_monitor_class;

static class FlowClass : public TclClass {
 public:
	FlowClass() : TclClass("QueueMonitor/ED/Flow") {}
	TclObject* create(int, const char*const*) {
		return (new Flow);
	}
} flow_class;

/* Added by Yun Wang */
static class TaggerTBFlowClass : public TclClass {
 public:
	TaggerTBFlowClass() : TclClass("QueueMonitor/ED/Flow/TB") {}
	TclObject* create(int, const char*const*) {
		return (new TaggerTBFlow);
	}
} flow_tb_class;

/* Added by Yun Wang */
static class TaggerTSWFlowClass : public TclClass {
 public:
	TaggerTSWFlowClass() : TclClass("QueueMonitor/ED/Flow/TSW") {}
	TclObject* create(int, const char*const*) {
		return (new TaggerTSWFlow);
	}
} flow_tsw_class;

/* Added by Yun Wang */
static class TaggerClass : public TclClass {
 public:
	TaggerClass() : TclClass("QueueMonitor/ED/Tagger") {}
	TclObject* create(int, const char*const*) {
		return (new Tagger);
	}
} tagger_class;


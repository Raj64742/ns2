/* -*-	Mode:C++; c-basic-offset:8; tab-width:8 -*- */
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
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
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
 *
 * Contributed by the Daedalus Research Group, UC Berkeley 
 * (http://daedalus.cs.berkeley.edu)
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/queue/errmodel.cc,v 1.39 1998/04/08 20:09:40 gnguyen Exp $ (UCB)
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/queue/errmodel.cc,v 1.39 1998/04/08 20:09:40 gnguyen Exp $ (UCB)";
#endif

#include <stdio.h>
#include "packet.h"
#include "errmodel.h"
#include "srm-headers.h"		// to get the hdr_srm structure
#include "classifier.h"

static class ErrorModelClass : public TclClass {
public:
	ErrorModelClass() : TclClass("ErrorModel") {}
	TclObject* create(int, const char*const*) {
		return (new ErrorModel);
	}
} class_errormodel;

static class TwoStateErrorModelClass : public TclClass {
public:
	TwoStateErrorModelClass() : TclClass("ErrorModel/TwoState") {}
	TclObject* create(int, const char*const*) {
		return (new TwoStateErrorModel);
	}
} class_errormodel_twostate;

static class MultiStateErrorModelClass : public TclClass {
public:
	MultiStateErrorModelClass() : TclClass("ErrorModel/MultiState") {}
	TclObject* create(int, const char*const*) {
		return (new MultiStateErrorModel);
	}
} class_errormodel_multistate;

static char* eu_names[] = { EU_NAMES };


ErrorModel::ErrorModel() : unit_(EU_PKT), ranvar_(0), firstTime_(1)
{
	bind("enable_", &enable_);
	bind("rate_", &rate_);
	bind_bw("bandwidth_", &bandwidth_); // required for EU_TIME
}

int ErrorModel::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	ErrorModel *em;
	if (argc == 3) {
		if (strcmp(argv[1], "unit") == 0) {
			unit_ = STR2EU(argv[2]);
			return (TCL_OK);
		} 
		if (strcmp(argv[1], "ranvar") == 0) {
			ranvar_ = (RandomVariable*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		} 
	} else if (argc == 2) {
		if (strcmp(argv[1], "unit") == 0) {
			tcl.resultf("%s", eu_names[unit_]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ranvar") == 0) {
			tcl.resultf("%s", ranvar_->name());
			return (TCL_OK);
		} 
	}
	return Connector::command(argc, argv);
}

void ErrorModel::reset()
{
	firstTime_ = 1;
}


void ErrorModel::recv(Packet* p, Handler* h)
{
	// 1.  Determine the error by calling corrupt(p)
	// 2.  Set the packet's error flag if it is corrupted
	// 3.  If there is no error or drop_ target, let pkt continue
	//     else hand the corrupted packet to drop_

	hdr_cmn* ch = (hdr_cmn*)p->access(off_cmn_);
	int error = corrupt(p);
	ch->error() |= error;
	if (! (error && drop_)) {
		if (target_)
			target_->recv(p, h);
		return;
	}

	if (h != 0) {
		// if pkt just come from queue, then resume queue
		double delay = Random::uniform(ch->size() / bandwidth_);
		Scheduler::instance().schedule(h, &intr_, delay);
	}
	drop_->recv(p);
}


int ErrorModel::corrupt(Packet* p)
{
	if (enable_ == 0)
		return 0;
	switch (unit_) {
	case EU_TIME:
		return (CorruptTime(p) != 0);
	case EU_BYTE:
		return (CorruptByte(p) != 0);
	default:
		return (CorruptPkt(p) != 0);
	}
	return 0;
}

double ErrorModel::PktLength(Packet* p)
{
	double now;
	if (unit_ == EU_PKT)
		return 1;
	int byte = ((hdr_cmn*)p->access(off_cmn_))->size();
	if (unit_ == EU_BYTE)
		return byte;
	return 8.0 * byte / bandwidth_;
}

int ErrorModel::CorruptPkt(Packet* p) 
{
	// if no random var is specified, assume uniform random variable
	double u = ranvar_ ? ranvar_->value() : Random::uniform();
	return (u < rate_);
}

int ErrorModel::CorruptByte(Packet* p)
{
	// compute pkt error rate, assume uniformly distributed byte error
	double per = 1 - pow(1.0 - rate_, PktLength(p));
	double u = ranvar_ ? ranvar_->value() : Random::uniform();
	return (u < per);
}

int ErrorModel::CorruptTime(Packet *p)
{
	fprintf(stderr, "Warning:  uniform rate error cannot be time-based\n");
	return 0;
}

#if 0
/*
 * Decide whether or not to corrupt this packet, for a continuous 
 * time-based error model.  The main parameter used is errLength,
 * which is the time to the next error, from the last time an error 
 * occured on  the channel.  It is dependent on the random variable 
 * being used  internally.
 *	rate_ is the user-specified mean
 */
int ErrorModel::CorruptTime(Packet *p)
{
	/* 
	 * First get MAC header.  It has the transmission time (txtime)
	 * of the packet in one of it's fields.  Then, get the time
	 * interval [t-txtime, t], where t is the current time.  The
	 * goal is to figure out whether the channel would have
	 * corrupted the packet during that interval. 
	 */
	Scheduler &s = Scheduler::instance();
	hdr_mac *mh = (hdr_mac*) p->access(hdr_mac::offset_);
	double now = s.clock(), rv;
	int numerrs = 0;
	double start = now - mh->txtime();

	while (remainLen_ < start) {
		rv = ranvar_ ? ranvar_->value() : Random::uniform(rate_);
		remainLen_ += rv;
	}

	while (remainLen_ < now) { /* corrupt the packet */
		numerrs++;
		rv = ranvar_ ? ranvar_->value() : Random::uniform(rate_);
		remainLen_ += rv;
	}
	return numerrs;
}
#endif


/*
 * Two-State:  error-free and error
 */
TwoStateErrorModel::TwoStateErrorModel() : remainLen_(0)
{
	ranvar_[0] = ranvar_[1] = 0;
}

int TwoStateErrorModel::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (strcmp(argv[1], "ranvar") == 0) {
		int i = atoi(argv[2]);
		if (i < 0 || i > 1) {
			tcl.resultf("%s does not has ranvar_[%d]", name_, i);
			return (TCL_ERROR);
		}
		if (argc == 3) {
			tcl.resultf("%s", ranvar_[i]->name());
			return (TCL_OK);
		}
		// else if (argc == 4)
		ranvar_[i] = (RandomVariable*)TclObject::lookup(argv[3]);
		return (TCL_OK);
	}
	return ErrorModel::command(argc, argv);
}


int TwoStateErrorModel::corrupt(Packet* p)
{
#define ZERO_RANGE 0.000001
	int error;
	if (firstTime_) {
		firstTime_ = 0;
		state_ = 0;
		remainLen_ = ranvar_[state_]->value();
	}

	// if remainLen_ is outside the range of 0, then error = state_
	error = state_ && (remainLen_ > ZERO_RANGE);
	remainLen_ -= PktLength(p);

	// state transition until remainLen_ > 0 to covers the packet length
	while (remainLen_ < ZERO_RANGE) {
		state_ ^= 1;	// state transition: 0 <-> 1
		remainLen_ += ranvar_[state_]->value();
		error |= state_;
	}
	return error;
}


/*
// MultiState ErrorModel:
//   corrupt(pkt) invoke Tcl method "corrupt" to do state transition
//	Tcl corrupt either:
//	   - assign em_, the error-model to be use
//	   - return the status of the packet
//	If em_ is assigned, then invoke em_->corrupt(p)
*/

MultiStateErrorModel::MultiStateErrorModel() : em_(0)
{
}

int MultiStateErrorModel::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "error-model") == 0) {
			em_ = (ErrorModel*) TclObject::lookup(argv[2]);
			return TCL_OK;
		}
	}
        return ErrorModel::command(argc, argv);
}

int MultiStateErrorModel::corrupt(Packet* p)
{
	Tcl& tcl = Tcl::instance();
	tcl.evalf("%s corrupt", name());
	return (em_ ? em_->corrupt(p) : atoi(tcl.result()));
}


/*
 * Periodic ErrorModel
 */
static class PeriodicErrorModelClass : public TclClass {
public:
        PeriodicErrorModelClass() : TclClass("ErrorModel/Periodic") {}
        TclObject* create(int, const char*const*) {
                return (new PeriodicErrorModel);
        }
} class_periodic_error_model;

PeriodicErrorModel::PeriodicErrorModel() : cnt_(0), last_time_(0.0), first_time_(-1.0)
{      
	bind("period_", &period_);
	bind("offset_", &offset_);
}      

int PeriodicErrorModel::corrupt(Packet* p)
{
	hdr_cmn *ch = (hdr_cmn*) p->access(off_cmn_);
	double now = Scheduler::instance().clock();

	if (unit_ == EU_TIME) {
		if (first_time_ < 0.0) {
			if (now >= offset_) {
				first_time_ = last_time_ = now;
				return 1;
			}
			return 0;
		} else if ((now - last_time_) > period_) {
			last_time_ = now;
			return 1;
		}
	}
	cnt_ += (unit_ == EU_PKT) ? 1 : ch->size();
	if (int(first_time_) < 0) {
		if (cnt_ >= int(offset_)) {
			first_time_ = 1.0;
			cnt_ = 0;
			return 1;
		}
		return 0;
	} else if (cnt_ >= int(period_)) {
		cnt_ = 0;
		return 1;
	}
        return 0;
}


static class SelectErrorModelClass : public TclClass {
public:
	SelectErrorModelClass() : TclClass("SelectErrorModel") {}
	TclObject* create(int, const char*const*) {
		return (new SelectErrorModel);
	}
} class_selecterrormodel;

SelectErrorModel::SelectErrorModel()
{
	bind("pkt_type_", &pkt_type_);
	bind("drop_cycle_", &drop_cycle_);
	bind("drop_offset_", &drop_offset_);
}

int SelectErrorModel::command(int argc, const char*const* argv)
{
        if (strcmp(argv[1], "drop-packet") == 0) {
		pkt_type_ = atoi(argv[2]);
		drop_cycle_ = atoi(argv[3]);
		drop_offset_ = atoi(argv[4]);
		return TCL_OK;
        }
        return ErrorModel::command(argc, argv);
}

int SelectErrorModel::corrupt(Packet* p)
{
	if (unit_ == EU_PKT) {
		hdr_cmn *ch = (hdr_cmn*) p->access(off_cmn_);
		if (ch->ptype() == pkt_type_ && ch->uid() % drop_cycle_ 
		    == drop_offset_) {
			printf ("dropping packet type %d, uid %d\n", 
				ch->ptype(), ch->uid());
			return 1;
		}
	}
	return 0;
}

/* Error model for srm experiments */
class SRMErrorModel : public SelectErrorModel {
public:
	SRMErrorModel();
	virtual int corrupt(Packet*);
protected:
	int command(int argc, const char*const* argv);
        int off_srm_;
};

static class SRMErrorModelClass : public TclClass {
public:
	SRMErrorModelClass() : TclClass("SRMErrorModel") {}
	TclObject* create(int, const char*const*) {
		return (new SRMErrorModel);
	}
} class_srmerrormodel;

SRMErrorModel::SRMErrorModel()
{
        bind("off_srm_", &off_srm_);
}

int SRMErrorModel::command(int argc, const char*const* argv)
{
        int ac = 0;
        if (strcmp(argv[1], "drop-packet") == 0) {
		pkt_type_ = atoi(argv[2]);
		drop_cycle_ = atoi(argv[3]);
		drop_offset_ = atoi(argv[4]);
		return TCL_OK;
        }
        return ErrorModel::command(argc, argv);
}

int SRMErrorModel::corrupt(Packet* p)
{
	if (unit_ == EU_PKT) {
                hdr_srm *sh = (hdr_srm*) p->access(off_srm_);
		hdr_cmn *ch = (hdr_cmn*) p->access(off_cmn_);
                if ((ch->ptype() == pkt_type_) && (sh->type() == SRM_DATA) && 
		    (sh->seqnum() % drop_cycle_ == drop_offset_)) {
		  //printf ("dropping packet type SRM-DATA, seqno %d\n", 
		  //sh->seqnum());
		  return 1;
		}
	}
	return 0;
}


static class ErrorModuleClass : public TclClass {
public:
	ErrorModuleClass() : TclClass("ErrorModule") {}
	TclObject* create(int, const char*const*) {
		return (new ErrorModule);
	}
} class_errormodule;

void ErrorModule::recv(Packet *p, Handler *h)
{
	classifier_->recv(p, h);
}

int ErrorModule::command(int argc, const char*const* argv)
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
	} else if (argc == 3) {
                if (strcmp(argv[1], "classifier") == 0) {
                        classifier_ = (Classifier*)
                                TclObject::lookup(argv[2]);
                        if (classifier_ == NULL) {
				tcl.resultf("Couldn't look up classifier %s", argv[2]);
                                return (TCL_ERROR);
			}
                        return (TCL_OK);
                }
	}
	return (Connector::command(argc, argv));
}

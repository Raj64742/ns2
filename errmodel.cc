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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/errmodel.cc,v 1.31 1998/03/06 21:19:32 haoboy Exp $ (UCB)";
#endif

#include "delay.h"
#include "packet.h"
#include "mac.h"
#include "errmodel.h"
#include "srm-headers.h"		// to get the hdr_srm structure
#include "connector.h"
#include "classifier.h"

static class ErrorModelClass : public TclClass {
public:
	ErrorModelClass() : TclClass("ErrorModel") {}
	TclObject* create(int, const char*const*) {
		return (new ErrorModel);
	}
} class_errormodel;

static class EmpErrorModelClass : public TclClass {
public:
	EmpErrorModelClass() : TclClass("ErrorModel/Empirical") {}
	TclObject* create(int, const char*const*) {
		return (new EmpiricalErrorModel);
	}
} class_emp_errormodel;

static char* eu_names[] = { EU_NAMES };


ErrorModel::ErrorModel() : Connector(), unit_(EU_PKT), ranvar_(0), 
	onlink_(0), enable_(1), firstTime_(1)
{
	bind("rate_", &rate_);
	bind("errPkt_", &errPkt_);
	bind("errByte_", &errByte_);
	bind("errTime_", &errTime_);
	bind("onlink_", &onlink_);
	bind("enable_", &enable_);
	bind("off_mac_", &off_mac_);
}


void ErrorModel::copy(ErrorModel* orig)
{
	// Only need to copy variables that are not Tcl-bound, OR
	// 	*this = *orig;
	unit_ = orig->unit_;
	ranvar_ = orig->ranvar_;
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
		if (strcmp(argv[1], "corrupt") == 0) {
			em = (ErrorModel*)TclObject::lookup(argv[2]);
			tcl.resultf("%d", corrupt(em->pkt_));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "copy") == 0) {
			em = (ErrorModel*)TclObject::lookup(argv[2]);
			copy(em);
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
		if (strcmp(argv[1], "onlink") == 0) {
			/* this model is between a queue and a link */
			onlink_ = 1;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "enable") == 0) {
			enable_ = 1;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "disable") == 0) {
			enable_ = 0;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "enabled?") == 0) {
			tcl.resultf("%d", enable_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "corrupt") == 0) {
			tcl.resultf("%d", corrupt(pkt_));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "reset") == 0) {
			em_reset();
			return (TCL_OK);
		}
	}
	return Connector::command(argc, argv);
}

void ErrorModel::recv(Packet* p, Handler* h)
{
	Tcl& tcl = Tcl::instance();

	pkt_ = p;
	tcl.evalf("%s corrupt", name());
	if (strcmp(tcl.result(), "0")) { /* corrupt if return value != 0 */
		((hdr_cmn*)p->access(off_cmn_))->error() |= 1;
		if (onlink_) {
			/* Callback to queue: assume next target is link(XXX)*/
			LinkDelay *link = (LinkDelay *)target_;
			double txt = link->txtime(p);
			Scheduler &s = Scheduler::instance();
			/* XXX drop time is a random time during xmission */
			s.schedule(h, &intr_, Random::uniform(0, txt));
		}
		/*
		 * If drop_ target exists, drop the corrupted packet,
		 * else mark the error() flag of the packet and pass it on.
		 */
		if (drop_) {
			drop_->recv(p);
			return;
		}
	}
	if (target_)
		target_->recv(p, h);
	/* XXX if no target, assume packet is still used by other object */
}

int ErrorModel::corrupt(Packet* p)
{
	/* If no random var is specified, assume uniform random variable */
	if (!enable_) 
		return 0;
	switch (unit_) {
	case EU_PKT: 
		return CorruptPkt(p);
	case EU_TIME:
		return CorruptTime(p);
	case EU_BYTE:
		return CorruptByte(p);
	default:
		break;
	}
	return 0;
}

/* Decide whether or not to corrupt this packet, based on a packet-based error
 * model.  The main parameter used in errPkt_, which is the number of
 * packets to next error, from the last time an error occured on the channel.
 * It is dependent on the random variable being used internally.
 * rate_ is the user-specified mean number of packets between errors.
 */
int ErrorModel::CorruptPkt(Packet *p) 
{
	double rv;
	int numerrs = 0;

	if (errPkt_ == 0) {
		if (firstTime_)
			firstTime_ = 0;
		else		/* corrupt the packet */
			numerrs++;
		/* rv is a random variable with mean = rate_ (set up in tcl) */
		rv = ranvar_ ? ranvar_->value() : Random::uniform(rate_);
		errPkt_ += (int) rv; /* # pkts to next error */
	} else
		errPkt_--;	/* count down to next error */
	return numerrs;
}


/*
 * Decide whether or not to corrupt this packet, for a continuous 
 * time-based error model.  The main parameter used is errTime_,
 * which is the time to the next error, from the last time an error 
 * occured on  the channel.  It is dependent on the random variable 
 * being used  internally.  rate_ is the user-specified mean amount of 
 * time, in seconds, between errors.
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
	hdr_mac *mh = (hdr_mac*) p->access(off_mac_);
	double now = s.clock(), rv;
	int numerrs = 0;
	double start = now - mh->txtime();

	while (errTime_ < start) {
		rv = ranvar_ ? ranvar_->value() : Random::uniform(rate_);
		errTime_ += rv;
	}

	while (errTime_ < now) { /* corrupt the packet */
		numerrs++;
		rv = ranvar_ ? ranvar_->value() : Random::uniform(rate_);
		errTime_ += rv;
	}
	return numerrs;
}

/*
 * Decide whether or not to corrupt this packet, based on a byte-based error
 * model.  The main parameter used in errByte_, which is the number of
 * bytes to next error, from the last time an error occured on the channel.
 * It is dependent on the random variable being used internally.  
 * rate_ is the user-specified mean number of bytes between errors.
 */
int ErrorModel::CorruptByte(Packet *p)
{
	int size = ((hdr_cmn*)p->access(off_cmn_))->size();
	int numerrs = 0;
	double rv;

	/* The same packet might have multiple errors, so catch them all! */
	while (errByte_ < size) {
		if (firstTime_)
			firstTime_ = 0;
		else		/* corrupt the packet */
			numerrs++;
		/* rv is a random variable with mean = rate_ (set up in tcl) */
		rv = ranvar_ ? ranvar_->value() : Random::uniform(rate_);
		errByte_ += (int) rv;

	}
	errByte_ -= size;
	if (errByte_ < 0)	/* XXX this should never happen, actually */
		errByte_ = 0;
	return numerrs;
}

void ErrorModel::em_reset()
{
	errByte_ = 0;
	errTime_ = 0;
	firstTime_ = 1;
	return;
}

int EmpiricalErrorModel::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (argc == 3) {
		if (strcmp(argv[1], "granvar") == 0) {
			grv_ = (RandomVariable*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "branvar") == 0) {
			brv_ = (RandomVariable*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	if (argc == 2) {
		if (strcmp(argv[1], "granvar") == 0) {
			tcl.resultf("%s", grv_->name());
			return (TCL_OK);
		} 
		if (strcmp(argv[1], "branvar") == 0) {
			tcl.resultf("%s", brv_->name());
			return (TCL_OK);
		}
	}
	return ErrorModel::command(argc, argv);
}

/*
 * Empirical error model based on CDFs of errored and error-free lengths.  
 */
int
EmpiricalErrorModel::CorruptTime(Packet *p)
{
	/* 
	 * First get MAC header.  It has the transmission time (txtime)
	 * of the packet in one of it's fields.  Then, get the time
	 * interval [t-txtime, t], where t is the current time.  The
	 * goal is to figure out whether the channel would have
	 * corrupted the packet during that interval. 
	 */
	Scheduler &s = Scheduler::instance();
	double now = s.clock();
	int numerrs = 0;
	double start = now - ((hdr_mac*) p->access(off_mac_))->txtime();

	if (firstTime_) {
		state_ = EM_GOOD;
		errTime_ = grv_->value();
		firstTime_ = 0;
	}
		
	while (errTime_ < start) {
		/* toggle states */
		if (state_ == EM_GOOD) {
			errTime_ += brv_->value();
			state_ = EM_BAD;
		} else {
			errTime_ += grv_->value();
			state_ = EM_GOOD;
		}
	}

	if (errTime_ >= now && state_ == EM_GOOD)
		return 0;
	return 1;
}



static class SelectErrorModelClass : public TclClass {
public:
	SelectErrorModelClass() : TclClass("SelectErrorModel") {}
	TclObject* create(int, const char*const*) {
		return (new SelectErrorModel);
	}
} class_selecterrormodel;

SelectErrorModel::SelectErrorModel() : ErrorModel()
{
	bind("pkt_type_", &pkt_type_);
	bind("drop_cycle_", &drop_cycle_);
	bind("drop_offset_", &drop_offset_);
}

int SelectErrorModel::command(int argc, const char*const* argv)
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

/*
 * periodic packet drops (drop every nth packet we see)
 * this can be conveniently combined with a flow-based classifier
 * to achieve drops in particular flows
 */
class PeriodicErrorModel : public ErrorModel {
  public:
        PeriodicErrorModel();
        int corrupt(Packet*);
  protected:
        int command(int argc, const char*const* argv);
	int cnt_;
        double period_;
	double offset_;
	double last_time_;
	double first_time_;
};


static class PeriodicErrorModelClass : public TclClass {
public:
        PeriodicErrorModelClass() : TclClass("ErrorModel/Periodic") {}
        TclObject* create(int, const char*const*) {
                return (new PeriodicErrorModel);
        }
} class_periodic_error_model;

PeriodicErrorModel::PeriodicErrorModel() : ErrorModel(), cnt_(0),
	last_time_(0.0), first_time_(-1.0)
{      
	bind("period_", &period_);
	bind("offset_", &offset_);
}      

int PeriodicErrorModel::command(int argc, const char*const* argv)
{   
        return ErrorModel::command(argc, argv);
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

static class SRMErrorModelClass : public TclClass {
public:
	SRMErrorModelClass() : TclClass("SRMErrorModel") {}
	TclObject* create(int, const char*const*) {
		return (new SRMErrorModel);
	}
} class_srmerrormodel;


SRMErrorModel::SRMErrorModel() : ErrorModel()
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
		  printf ("dropping packet type SRM-DATA, seqno %d\n", 
			  sh->seqnum());
		  return 1;
		}
	}
	return 0;
}


class ErrorModule : public Connector {
public:
	ErrorModule() : classifier_(0) {}
	int command(int, const char*const*);
protected:
	void recv(Packet*, Handler*);
	Classifier* classifier_;
};

static class ErrorModuleClass : public TclClass {
public:
	ErrorModuleClass() : TclClass("ErrorModule") {}
	TclObject* create(int, const char*const*) {
		return (new ErrorModule);
	}
} class_errormodule;

void
ErrorModule::recv(Packet *p, Handler *h)
{
	classifier_->recv(p, h);
}

int
ErrorModule::command(int argc, const char*const* argv)
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

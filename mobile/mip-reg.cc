/*
 * Copyright (c) Sun Microsystems, Inc. 1998 All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Sun Microsystems, Inc.
 *
 * 4. The name of the Sun Microsystems, Inc nor may not be used to endorse or
 *      promote products derived from this software without specific prior
 *      written permission.
 *
 * SUN MICROSYSTEMS MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS
 * SOFTWARE FOR ANY PARTICULAR PURPOSE.  The software is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this software.
 */

// #ident "@(#)mip-reg.cc  1.4     98/08/30 SMI"

#include "template.h"
#include "mip.h"
#include "random.h"

#define AGENT_ADS_SIZE		48
#define REG_REQUEST_SIZE	52

static class MIPHeaderClass : public PacketHeaderClass {
public:
	MIPHeaderClass() : PacketHeaderClass("PacketHeader/MIP",
					     sizeof(hdr_mip)) {
	}
} class_miphdr;

static class MIPBSAgentClass : public TclClass {
public:
	MIPBSAgentClass() : TclClass("Agent/MIPBS") {}
	TclObject* create(int, const char*const*) {
		return (new MIPBSAgent());
	}
} class_mipbsagent;

MIPBSAgent::MIPBSAgent() : Agent(PT_UDP), bcast_target_(0), timer_(this),
	beacon_(1.0), shift_(8), mask_(0xffffffff), adlftm_(~0)
{
	bind("adSize_", &size_);
	bind("shift_", &shift_);
	bind("mask_", &mask_);
	bind("ad_lifetime_", &adlftm_);
	bind("off_mip_", &off_mip_);
	size_ = AGENT_ADS_SIZE;
	seqno_ = -1;
}

void MIPBSAgent::recv(Packet* p, Handler *h)
{
	Tcl& tcl = Tcl::instance();
	char *objname;
	NsObject *obj;
	hdr_mip *miph = (hdr_mip *)p->access(off_mip_);
	hdr_ip *iph = (hdr_ip *)p->access(off_ip_);

	switch (miph->type_) {
	case MIPT_REG_REQUEST:
		if (miph->ha_ == (addr_ >> shift_ & mask_)) {
			if (miph->ha_ == miph->coa_) { // back home
				tcl.evalf("%s clear-reg %d", name_,
					  miph->haddr_);
			}
			else {
				tcl.evalf("%s encap-route %d %d %lf", name_,
					  miph->haddr_, miph->coa_,
					  miph->lifetime_);
			}
			iph->dst() = iph->src();
			miph->type_ = MIPT_REG_REPLY;
		}
		else {
			iph->dst() = iph->dst() & ~(~(nsaddr_t)0 << shift_) |
				(miph->ha_ & mask_) << shift_;
		}
		iph->src() = addr_;
		// by now should be back to normal route
		// if dst is the mobile
		send(p, 0);
		break;
	case MIPT_REG_REPLY:
		assert(miph->coa_ == (addr_ >> shift_ & mask_));
		tcl.evalf("%s get-link %d %d", name_, addr_ >> shift_ & mask_,
			  miph->haddr_);
		obj = (NsObject*)tcl.lookup(objname = tcl.result());
		tcl.evalf("%s decap-route %d %s %lf", name_, miph->haddr_,
			  objname, miph->lifetime_);
		iph->src() = iph->dst();
		iph->dst() = iph->dst() & ~(~(nsaddr_t)0 << shift_) |
			(miph->haddr_ & mask_) << shift_;
		obj->recv(p, (Handler*)0);
		break;
	case MIPT_SOL:
		tcl.evalf("%s get-link %d %d", name_, addr_ >> shift_ & mask_,
			  miph->haddr_);
		send_ads(miph->haddr_, (NsObject*)tcl.lookup(tcl.result()));
		Packet::free(p);
		break;
	default:
		Packet::free(p);
		break;
	}
}

void MIPBSAgent::timeout(int tno)
{
	send_ads();
	timer_.resched(beacon_);
}

int MIPBSAgent::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "beacon-period") == 0) {
			beacon_ = atof(argv[2]);
			timer_.resched(Random::uniform(0, beacon_));
			return TCL_OK;
		}
		if (strcmp(argv[1], "bcast-target") == 0) {
			bcast_target_ = (NsObject *)TclObject::lookup(argv[2]);
			return TCL_OK;
		}
	}
	return (Agent::command(argc, argv));
}

void MIPBSAgent::send_ads(int dst, NsObject *target)
{
	Packet *p = allocpkt();
	hdr_mip *h = (hdr_mip *)p->access(off_mip_);
	h->haddr_ = h->ha_ = -1;
	h->coa_ = addr_ >> shift_ & mask_;
	h->type_ = MIPT_ADS;
	h->lifetime_ = adlftm_;
	h->seqno_ = ++seqno_;
	if (dst != -1) {
		hdr_ip *iph = (hdr_ip *)p->access(off_ip_);
		iph->dst() = iph->dst() & ~(~(nsaddr_t)0 << shift_) |
			(dst & mask_) << shift_;
	}
	if (target == NULL) {
		if (bcast_target_) bcast_target_->recv(p, (Handler*) 0);
		else Packet::free(p); // drop; may log in future code
	}
	else target->recv(p, (Handler*)0);
}

void AgtListTimer::expire(Event *e) {
	a_->timeout(MIP_TIMER_AGTLIST);
}

static class MIPMHAgentClass : public TclClass {
public:
	MIPMHAgentClass() : TclClass("Agent/MIPMH") {}
	TclObject* create(int, const char*const*) {
		return (new MIPMHAgent());
	}
} class_mipmhagent;

MIPMHAgent::MIPMHAgent() : Agent(PT_UDP), ha_(-1), coa_(-1), agts_(0),
	rtx_timer_(this), agtlist_timer_(this), shift_(8), mask_(0xffffffff),
	reglftm_(~0), adlftm_(0.0), beacon_(1.0), bcast_target_(0)
{
	bind("home_agent_", &ha_);
	bind("rreqSize_", &size_);
	bind("reg_rtx_", &reg_rtx_);
	bind("shift_", &shift_);
	bind("mask_", &mask_);
	bind("reg_lifetime_", &reglftm_);
	bind("off_mip_", &off_mip_);
	size_ = REG_REQUEST_SIZE;
	seqno_ = -1;
}

void MIPMHAgent::recv(Packet* p, Handler *h)
{
	Tcl& tcl = Tcl::instance();
	hdr_mip *miph = (hdr_mip *)p->access(off_mip_);
	switch (miph->type_) {
	case MIPT_REG_REPLY:
		if (miph->coa_ != coa_) break; // not pending
		tcl.evalf("%s update-reg %d", name_, coa_);
		if (rtx_timer_.status() == TIMER_PENDING)
			rtx_timer_.cancel();
		break;
	case MIPT_ADS:
		{
			AgentList **ppagts = &agts_, *ptr;
			while (*ppagts) {
				if ((*ppagts)->node_ == miph->coa_) break;
				ppagts = &(*ppagts)->next_;
			}
			if (*ppagts) {
				ptr = *ppagts;
				*ppagts = ptr->next_;
				ptr->expire_time_ = beacon_ +
					Scheduler::instance().clock();
				ptr->lifetime_ = miph->lifetime_;
				ptr->next_ = agts_;
				agts_ = ptr;
				if (coa_ == miph->coa_) {
					seqno_++;
					reg();
				}
			}
			else { // new ads
				ptr = new AgentList;
				ptr->node_ = miph->coa_;
				ptr->expire_time_ = beacon_ +
					Scheduler::instance().clock();
				ptr->lifetime_ = miph->lifetime_;
				ptr->next_ = agts_;
				agts_ = ptr;
				coa_ = miph->coa_;
				adlftm_ = miph->lifetime_;
				seqno_++;
				reg();
			}
		}
		break;
	default:
		break;
	}
	Packet::free(p);
}

void MIPMHAgent::timeout(int tno)
{
	switch (tno) {
	case MIP_TIMER_SIMPLE:
		reg();
		break;
	case MIP_TIMER_AGTLIST:
		{
			double now = Scheduler::instance().clock();
			AgentList **ppagts = &agts_, *ptr;
			int coalost = 0;
			while (*ppagts) {
				if ((*ppagts)->expire_time_ < now) {
					ptr = *ppagts;
					*ppagts = ptr->next_;
					if (ptr->node_ == coa_) {
						coa_ = -1;
						coalost = 1;
					}
					delete ptr;
				}
				else ppagts = &(*ppagts)->next_;
			}
			agtlist_timer_.resched(beacon_);
			if (coalost) {
				seqno_++;
				reg();
			}
		}
		break;
	default:
		break;
	}
}

int MIPMHAgent::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "beacon-period") == 0) {
			beacon_ = atof(argv[2]);
			timeout(MIP_TIMER_AGTLIST);
			agtlist_timer_.resched(beacon_);
			rtx_timer_.resched(Random::uniform(0, beacon_));
			return TCL_OK;
		}
		if (strcmp(argv[1], "bcast-target") == 0) {
			bcast_target_ = (NsObject *)TclObject::lookup(argv[2]);
			return TCL_OK;
		}
	}
	// later: agent solicitation (now done!), start of simulation, ...
	return (Agent::command(argc, argv));
}

void MIPMHAgent::reg()
{
	rtx_timer_.resched(reg_rtx_);
	if (agts_ == 0) {
		send_sols();
		return;
	}
	if (coa_ < 0) {
		coa_ = agts_->node_;
		adlftm_ = agts_->lifetime_;
	}
	Tcl& tcl = Tcl::instance();
	Packet *p = allocpkt();
	hdr_ip *iph = (hdr_ip *)p->access(off_ip_);
	iph->dst() = iph->dst() & ~(~(nsaddr_t)0 << shift_) |
		(coa_ & mask_) << shift_;
	hdr_mip *h = (hdr_mip *)p->access(off_mip_);
	h->haddr_ = addr_ >> shift_ & mask_;
	h->ha_ = ha_;
	h->coa_ = coa_;
	h->type_ = MIPT_REG_REQUEST;
	h->lifetime_ = min(reglftm_, adlftm_);
	h->seqno_ = seqno_;
	tcl.evalf("%s get-link %d %d", name_, h->haddr_, coa_);
	((NsObject *)tcl.lookup(tcl.result()))->recv(p, (Handler*) 0);
}

void MIPMHAgent::send_sols()
{
	Packet *p = allocpkt();
	hdr_mip *h = (hdr_mip *)p->access(off_mip_);
	h->coa_ = -1;
	h->haddr_ = addr_ >> shift_ & mask_;
	h->ha_ = ha_;
	h->type_ = MIPT_SOL;
	h->lifetime_ = reglftm_;
	h->seqno_ = seqno_;
	if (bcast_target_) bcast_target_->recv(p, (Handler*) 0);
	else Packet::free(p); // drop; may log in future code
}

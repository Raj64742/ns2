/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or
 * derivative work. Xerox grants no other licenses expressed or
 * implied. The Xerox trade name should not be used in any advertising
 * without its written permission. 
 *
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * software. 
 */

#include "packet.h"
#include "ip.h"
#include "resv.h"
#include "connector.h"
#include "adc.h"
#include "salink.h"

static class SALinkClass : public TclClass {
public:
	SALinkClass() : TclClass("SALink") {}
	TclObject* create(int, const char*const*) {
		return (new SALink());
	}
}class_salink;



SALink::SALink() : adc_(0)
{
	int i;
	for (i=0;i<NFLOWS;i++) {
		pending_[i].flowid=-1;
		pending_[i].status=0;
	}
	bind("off_resv_",&off_resv_);
	bind("off_ip_",&off_ip_);
}


void SALink::recv(Packet *p, Handler *h)
{
	int decide;
	int j;
	
	hdr_cmn *ch=(hdr_cmn*)p->access(off_cmn_);
	hdr_ip *iph=(hdr_ip*)p->access(off_ip_);
	hdr_resv *rv=(hdr_resv*)p->access(off_resv_);
	
	//CLEAN THIS UP
	int cl=(iph->flowid())?1:0;
	
	switch(ch->ptype()) {
	case PT_REQUEST:
		{
			decide=adc_->admit_flow(cl,rv->rate(),rv->bucket());
			//put decide in the packet
			rv->decision() &= decide;
			if (decide) {
				j=get_nxt();
				pending_[j].flowid=iph->flowid();
				//pending_[j].status=decide;
			}
			break;
		}
	case PT_REPLY:
		break;
	case PT_CONFIRM:
		{
			j=lookup(iph->flowid());
			if (j!=-1) {
				if (!rv->decision()) 
					//decrease the avload for this class 
					adc_->rej_action(cl,rv->rate(),rv->bucket());
				pending_[j].flowid=-1;
			}
			break;
		}
	case PT_TEARDOWN:
		{
			adc_->teardown_action(cl,rv->rate(),rv->bucket());
			break;
		}
	default:
#ifdef notdef
		error("unknown signalling message type : %d",ch->ptype());
		abort();
#endif
		break;
	}
	send(p,h);
}

int SALink::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	
	if (argc ==3) {
		if (strcmp(argv[1],"attach-adc") == 0 ) {
			adc_=(ADC *)TclObject::lookup(argv[2]);
			if (adc_ ==0 ) {
				tcl.resultf("no such node %s", argv[2]);
				return(TCL_ERROR);
			}
			return(TCL_OK);
		}
	}
	return Connector::command(argc,argv);
}

int SALink::lookup(int flowid)
{
	int i;
	for (i=0;i<NFLOWS;i++)
		if (pending_[i].flowid==flowid)
			return i;
	return(-1);
}

int SALink::get_nxt()
{
	int i;
	for (i=0;i<NFLOWS;i++)
		{
			if (pending_[i].flowid==-1)
				return i;
		}
	printf("Ran out of pending space \n");
	exit(1);
	return i;
}


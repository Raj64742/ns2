/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
/* Ported from CMU/Monarch's code, nov'98 -Padma.*/

#include <packet.h>
#include <ip.h>
#include <tcp.h>
#include <rtp.h>
#include <arp.h>
#include <dsr/hdr_sr.h>	// DSR
#include <mac.h>
#include <mac-802_11.h>
#include <address.h>
#include <tora/tora_packet.h> //TORA
#include <imep/imep_spec.h>         // IMEP
#include <aodv/aodv_packet.h> //AODV
#include <cmu-trace.h>
#include <mobilenode.h>

//#define LOG_POSITION

//extern char* pt_names[];

static class CMUTraceClass : public TclClass {
public:
	CMUTraceClass() : TclClass("CMUTrace") { }
	TclObject* create(int, const char*const* argv) {
		return (new CMUTrace(argv[4], *argv[5]));
	}
} cmutrace_class;


CMUTrace::CMUTrace(const char *s, char t) : Trace(t)
{
	bzero(tracename, sizeof(tracename));
	strncpy(tracename, s, MAX_ID_LEN);

        if(strcmp(tracename, "RTR") == 0) {
                tracetype = TR_ROUTER;
        }
	else if(strcmp(tracename, "TRP") == 0) {
                tracetype = TR_ROUTER;
        }
        else if(strcmp(tracename, "MAC") == 0) {
                tracetype = TR_MAC;
        }
        else if(strcmp(tracename, "IFQ") == 0) {
                tracetype = TR_IFQ;
        }
        else if(strcmp(tracename, "AGT") == 0) {
                tracetype = TR_AGENT;
        }
        else {
                fprintf(stderr, "CMU Trace Initialized with invalid type\n");
                exit(1);
        }

	assert(type_ == DROP || type_ == SEND || type_ == RECV);

	newtrace_ = 0;

        node_ = 0;
	off_mac_ = hdr_mac::offset_;
	//bind("off_mac_", &off_mac_);
	bind("off_arp_", &off_arp_);
	bind("off_SR_", &off_sr_);
	bind("off_TORA_", &off_TORA_);
        bind("off_IMEP_", &off_IMEP_);
	bind("off_AODV_", &off_AODV_);

}

void
CMUTrace::format_mac(Packet *p, const char *why, int offset)
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_mac802_11 *mh = HDR_MAC802_11(p);
	double x = 0.0, y = 0.0, z = 0.0;
       
	char op = (char) type_;


	Node* thisnode = Node::get_node_by_address(src_);
	

	double energy = -1;
	if (thisnode) {
	    if (thisnode->energy_model()) {
	       energy = thisnode->energy();
	    }
	}

	// hack the IP address to convert pkt format to hostid format
	// for now until port ids are removed from IP address. -Padma.

	int src = Address::instance().get_nodeaddr(ih->saddr());

	if(tracetype == TR_ROUTER && type_ == SEND) {
		if(src_ != src)
			op = FWRD;
	}

	// Use new ns trace format to replace the old cmu trace format)
	if (newtrace_) {
	    
	    node_->getLoc(&x, &y, &z);
	    // consistence
	    if ( op == DROP ) { op = 'd';}

	        // basic trace infomation + basic exenstion

	    sprintf(wrk_ + offset,
		    "%c -t %.9f -Hs %d -Hd %d -Ni %d -Nx %.2f -Ny %.2f -Nz %.2f -Ne %f -Nl %3s -Nw %s ",
		    op,                       // event type
		    Scheduler::instance().clock(),  // time
		    src_,                           // this node
                    ch->next_hop_,                  // next hop
		    src_,                           // this node
		    x,                              // x coordinate
		    y,                              // y coordinate
		    z,                              // z coordinate
		    energy,                         // energy, -1 = not existing
		    tracename,                      // trace level
                    why);                            // reason

	    // mac layer extension

	    offset = strlen(wrk_);

	    sprintf(wrk_ + offset, 
		    "-m [ %x %x %x %x ] ",
		    mh->dh_duration,
		    ETHER_ADDR(mh->dh_da),
		    ETHER_ADDR(mh->dh_sa),
		    GET_ETHER_TYPE(mh->dh_body));
		    
	     return;
	}


#ifdef LOG_POSITION
        double x = 0.0, y = 0.0, z = 0.0;
        node_->getLoc(&x, &y, &z);
#endif

	sprintf(wrk_ + offset,
#ifdef LOG_POSITION
		"%c %.9f %d (%6.2f %6.2f) %3s %4s %d %s %d [%x %x %x %x] ",
#else
		"%c %.9f _%d_ %3s %4s %d %s %d [%x %x %x %x] ",
#endif
		op,
		Scheduler::instance().clock(),
                src_,                           // this node
#ifdef LOG_POSITION
                x,
                y,
#endif
		tracename,
		why,

                ch->uid(),                      // identifier for this event
		packet_info.name(ch->ptype()),
		ch->size(),

		//*((u_int16_t*) &mh->dh_fc),
		mh->dh_duration,
		ETHER_ADDR(mh->dh_da),
		ETHER_ADDR(mh->dh_sa),
		GET_ETHER_TYPE(mh->dh_body));

	offset = strlen(wrk_);

	if (thisnode) {
		if (thisnode->energy_model()) {
			sprintf(wrk_ + offset,
				"[energy %f] ",
				thisnode->energy());
		}
        }
}

void
CMUTrace::format_ip(Packet *p, int offset)
{
        struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	
	// hack the IP address to convert pkt format to hostid format
	// for now until port ids are removed from IP address. -Padma.
	int src = Address::instance().get_nodeaddr(ih->saddr());
	int dst = Address::instance().get_nodeaddr(ih->daddr());

	if (newtrace_) {
	    sprintf(wrk_ + offset,
		    "-Is %d.%d -Id %d.%d -It %s -Il %d -If %d -Ii %d -Iv %d ",
		    src,                           // packet src
		    ih->sport(),                   // src port
		    dst,                           // packet dest
		    ih->dport(),                   // dst port
		    packet_info.name(ch->ptype()),  // packet type
		    ch->size(),                     // packet size
		    ih->flowid(),                   // flow id
		    ch->uid(),                      // unique id
		    ih->ttl_);                      // ttl
	} else {
	    sprintf(wrk_ + offset, "------- [%d:%d %d:%d %d %d] ",
		src, ih->sport(),
		dst, ih->dport(),
		ih->ttl_, (ch->next_hop_ < 0) ? 0 : ch->next_hop_);
	}
}

void
CMUTrace::format_arp(Packet *p, int offset)
{
	struct hdr_arp *ah = HDR_ARP(p);

	if (newtrace_) {
	    sprintf(wrk_ + offset,
		    "-P arp -Po %s -Pm %d -Ps %d -Pa %d -Pd %d ",
		    ah->arp_op == ARPOP_REQUEST ?  "REQUEST" : "REPLY",
		    ah->arp_sha,                        // src mac addr
		    ah->arp_spa,                        // src
		    ah->arp_tha,                        // dst mac - what you looking for 
		    ah->arp_tpa);                       // dst
	} else {

	    sprintf(wrk_ + offset,
		"------- [%s %d/%d %d/%d]",
		ah->arp_op == ARPOP_REQUEST ?  "REQUEST" : "REPLY",
		ah->arp_sha,
		ah->arp_spa,
		ah->arp_tha,
		ah->arp_tpa);
	}
}

void
CMUTrace::format_dsr(Packet *p, int offset)
{
	hdr_sr *srh = (hdr_sr*)p->access(off_sr_);

	if (newtrace_) {
	    sprintf(wrk_ + offset, 
		"-P dsr -Pn %d -Pq %d -Pi %d -Pp %d -Pl %d -Pe %d->%d -Pw %d -Pm %d -Pc%d -Pb %d->%d ",
		    srh->num_addrs(),                   // how many nodes travered

		    srh->route_request(),               //is this a route request?
		    srh->rtreq_seq(),                   //route request sequence number
		    srh->route_reply(),                 // is the reply below valid?
		    //    srh->rtreq_seq(),                   
		    srh->route_reply_len(),             //reply len
		// the dest of the src route
		    srh->reply_addrs()[0].addr,         //src of the src route
		    srh->reply_addrs()[srh->route_reply_len()-1].addr,  // dest of the src route

		    srh->route_error(),                //are we carrying a route error reply?
		    srh->num_route_errors(),           // 
		    srh->down_links()[srh->num_route_errors() - 1].tell_addr, // tell whom
		    srh->down_links()[srh->num_route_errors() - 1].from_addr,     //link down b/w from
		    srh->down_links()[srh->num_route_errors() - 1].to_addr);      //and to

	   return;
	}
	sprintf(wrk_ + offset, 
		"%d [%d %d] [%d %d %d %d->%d] [%d %d %d %d->%d]",
		srh->num_addrs(),

		srh->route_request(),
		srh->rtreq_seq(),

		srh->route_reply(),
		srh->rtreq_seq(),
		srh->route_reply_len(),
		// the dest of the src route
		srh->reply_addrs()[0].addr,
		srh->reply_addrs()[srh->route_reply_len()-1].addr,

		srh->route_error(),
		srh->num_route_errors(),
		srh->down_links()[srh->num_route_errors() - 1].tell_addr,
		srh->down_links()[srh->num_route_errors() - 1].from_addr,
		srh->down_links()[srh->num_route_errors() - 1].to_addr);
}

void
CMUTrace::format_msg(Packet *, int)
{

}


void
CMUTrace::format_tcp(Packet *p, int offset)
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_tcp *th = HDR_TCP(p);
	
	if( newtrace_ ) {
	    sprintf(wrk_ + offset,
		"-P tcp -Ps %d -Pa %d -Pf %d -Po %d ",
		th->seqno_,
		th->ackno_,
		ch->num_forwards(),
		ch->opt_num_forwards());

	} else {
	    sprintf(wrk_ + offset,
		"[%d %d] %d %d",
		th->seqno_,
		th->ackno_,
		ch->num_forwards(),
		ch->opt_num_forwards());
	}
}

void
CMUTrace::format_rtp(Packet *p, int offset)
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_rtp *rh = HDR_RTP(p);

	if (newtrace_) {
	    sprintf(wrk_ + offset,
		"-P cbr -Pi %d -Pf %d -Po %d ",
		rh->seqno_,
		ch->num_forwards(),
		ch->opt_num_forwards());
	      

	} else {

	    sprintf(wrk_ + offset,
		"[%d] %d %d",
		rh->seqno_,
		ch->num_forwards(),
		ch->opt_num_forwards());
	}
}

void
CMUTrace::format_imep(Packet *p, int offset)
{
        struct hdr_imep *im = HDR_IMEP(p);

#define U_INT16_T(x)    *((u_int16_t*) &(x))

	if (newtrace_) {
	    sprintf(wrk_ + offset,
                "-P imep %c %c %c 0x%04x ",
                (im->imep_block_flags & BLOCK_FLAG_ACK) ? 'A' : '-',
                (im->imep_block_flags & BLOCK_FLAG_HELLO) ? 'H' : '-',
                (im->imep_block_flags & BLOCK_FLAG_OBJECT) ? 'O' : '-',
                U_INT16_T(im->imep_length));
	} else {
            sprintf(wrk_ + offset,
                "[%c %c %c 0x%04x] ",
                (im->imep_block_flags & BLOCK_FLAG_ACK) ? 'A' : '-',
                (im->imep_block_flags & BLOCK_FLAG_HELLO) ? 'H' : '-',
                (im->imep_block_flags & BLOCK_FLAG_OBJECT) ? 'O' : '-',
                U_INT16_T(im->imep_length));
	}
#undef U_INT16_T
}


void
CMUTrace::format_tora(Packet *p, int offset)
{
        struct hdr_tora *th = HDR_TORA(p);
        struct hdr_tora_qry *qh = HDR_TORA_QRY(p);
        struct hdr_tora_upd *uh = HDR_TORA_UPD(p);
        struct hdr_tora_clr *ch = HDR_TORA_CLR(p);

        switch(th->th_type) {

        case TORATYPE_QRY:

		if (newtrace_) {
		    sprintf(wrk_ + offset,
			"-p [ tora 0x%x %d QUERY ] ",
                        qh->tq_type, qh->tq_dst);
			
                } else {

                    sprintf(wrk_ + offset, "[0x%x %d] (QUERY)",
                        qh->tq_type, qh->tq_dst);
		}
                break;

        case TORATYPE_UPD:

		if (newtrace_) {
		    sprintf(wrk_ + offset,
                        "-p [ tora 0x%x %d (%f %d %d %d %d) UPDATE ] ",
                        uh->tu_type,
                        uh->tu_dst,
                        uh->tu_tau,
                        uh->tu_oid,
                        uh->tu_r,
                        uh->tu_delta,
                        uh->tu_id);
		   

		} else {

                    sprintf(wrk_ + offset,
                        "[0x%x %d (%f %d %d %d %d)] (UPDATE)",
                        uh->tu_type,
                        uh->tu_dst,
                        uh->tu_tau,
                        uh->tu_oid,
                        uh->tu_r,
                        uh->tu_delta,
                        uh->tu_id);
		}
                break;

        case TORATYPE_CLR:
		if (newtrace_) {
		    sprintf(wrk_ + offset, 
			"-p [ tora 0x%x %d %f %d] CLEAR ] ",
                        ch->tc_type,
                        ch->tc_dst,
                        ch->tc_tau,
                        ch->tc_oid);


		} else {

                    sprintf(wrk_ + offset, "[0x%x %d %f %d] (CLEAR)",
                        ch->tc_type,
                        ch->tc_dst,
                        ch->tc_tau,
                        ch->tc_oid);
		}
                break;
        }
}

void
CMUTrace::format_aodv(Packet *p, int offset)
{
        struct hdr_aodv *ah = HDR_AODV(p);
        struct hdr_aodv_request *rq = HDR_AODV_REQUEST(p);
        struct hdr_aodv_reply *rp = HDR_AODV_REPLY(p);

        switch(ah->ah_type) {
        case AODVTYPE_RREQ:

		if (newtrace_) {

		    sprintf(wrk_ + offset,
			"-p [ aodv 0x%x %d %d [%d %d] [%d %d]] REQUEST ] ",
			rq->rq_type,
                        rq->rq_hop_count,
                        rq->rq_bcast_id,
                        rq->rq_dst,
                        rq->rq_dst_seqno,
                        rq->rq_src,
                        rq->rq_src_seqno);


		} else {

		    sprintf(wrk_ + offset,
			"[0x%x %d %d [%d %d] [%d %d]] (REQUEST)",
			rq->rq_type,
                        rq->rq_hop_count,
                        rq->rq_bcast_id,
                        rq->rq_dst,
                        rq->rq_dst_seqno,
                        rq->rq_src,
                        rq->rq_src_seqno);
		}
                break;

        case AODVTYPE_RREP:
        case AODVTYPE_UREP:
        case AODVTYPE_HELLO:

		if (newtrace_) {

		    sprintf(wrk_ + offset,
			"-p [aodv 0x%x %d [%d %d] %d] %s ] ",
			rp->rp_type,
                        rp->rp_hop_count,
                        rp->rp_dst,
                        rp->rp_dst_seqno,
                        rp->rp_lifetime,
                        rp->rp_type == AODVTYPE_RREP ? "REPLY" :
                        (rp->rp_type == AODVTYPE_UREP ? "UNSOLICITED REPLY" :
                         "HELLO"));

	        } else {

		    sprintf(wrk_ + offset,
			"[0x%x %d [%d %d] %d] (%s)",
			rp->rp_type,
                        rp->rp_hop_count,
                        rp->rp_dst,
                        rp->rp_dst_seqno,
                        rp->rp_lifetime,
                        rp->rp_type == AODVTYPE_RREP ? "REPLY" :
                        (rp->rp_type == AODVTYPE_UREP ? "UNSOLICITED REPLY" :
                         "HELLO"));
		}
                break;

        default:
                fprintf(stderr,
                        "%s: invalid AODV packet type\n", __FUNCTION__);
                abort();
        }
}

void
CMUTrace::nam_format(Packet *p, const char *why, int offset)
{
        struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	char op = (char) type_;
	Node* thisnode = Node::get_node_by_address(src_);

	int src = Address::instance().get_nodeaddr(ih->saddr());
	int dst = Address::instance().get_nodeaddr(ih->daddr());

	if (op == 's') op = 'h' ;

	sprintf(nwrk_ + offset,
		"%c -t %.9f -s %d -d %d -p %s -e %d -c 2 -i %d ",
		op,
		Scheduler::instance().clock(),
		src,                           // this node
		dst,
		packet_info.name(ch->ptype()),
		ch->size(),
		ch->uid());

	offset = strlen(nwrk_);

}

void CMUTrace::format(Packet* p, const char *why)
{
	hdr_cmn *ch = HDR_CMN(p);
	int offset = 0;

	/*
	 * Log the MAC Header
	 */
	format_mac(p, why, offset);
	nam_format(p, why, offset);
	offset = strlen(wrk_);

	switch(ch->ptype()) {

	case PT_MAC:
		break;

	case PT_ARP:
		format_arp(p, offset);
		break;

	default:
		format_ip(p, offset);
		offset = strlen(wrk_);

		switch(ch->ptype()) {
		
		 case PT_AODV:
			 format_aodv(p, offset);
			 break;

		 case PT_TORA:
                        format_tora(p, offset);
                        break;

                case PT_IMEP:
                        format_imep(p, offset);
                        break;

		case PT_DSR:
			format_dsr(p, offset);
			break;

		case PT_MESSAGE:
		case PT_UDP:
			format_msg(p, offset);
			break;
			
		case PT_TCP:
		case PT_ACK:
			format_tcp(p, offset);
			break;
			
		case PT_CBR:
			format_rtp(p, offset);
			break;

		default:
			fprintf(stderr, "%s - invalid packet type (%s).\n",
				__PRETTY_FUNCTION__, packet_info.name(ch->ptype()));
			exit(1);
		}
	}
}

int
CMUTrace::command(int argc, const char*const* argv)
{
	
        if(argc == 3) {
                if(strcmp(argv[1], "node") == 0) {
                        node_ = (MobileNode*) TclObject::lookup(argv[2]);
                        if(node_ == 0)
                                return TCL_ERROR;
                        return TCL_OK;
                }
		if (strcmp(argv[1], "newtrace") == 0) {
			newtrace_ = atoi(argv[2]);
		        return TCL_OK;
		}
        }
	return Trace::command(argc, argv);
}

/*ARGSUSED*/
void
CMUTrace::recv(Packet *p, Handler *h)
{
	if (!node_energy()) {
	    Packet::free(p);
	    return;
	}
	//struct hdr_ip *ih = HDR_IP(p);
	
	// hack the IP address to convert pkt format to hostid format
	// for now until port ids are removed from IP address. -Padma.

// 	int src = Address::instance().get_nodeaddr(ih->saddr());
        
        assert(initialized());

        /*
         * Agent Trace "stamp" the packet with the optimal route on
         * sending.
         */
        if(tracetype == TR_AGENT && type_ == SEND) {
                //assert(src_ == src);
                God::instance()->stampPacket(p);
        }
#if 0
        /*
         * When the originator of a packet drops the packet, it may or may
         * not have been stamped by GOD.  Stamp it before logging the
         * information.
         */
        if(src_ == src && type_ == DROP) {
                God::instance()->stampPacket(p);
        }
#endif
	format(p, "---");
	dump();
	namdump();
	if(target_ == 0)
		Packet::free(p);
	else
		send(p, h);
}

void
CMUTrace::recv(Packet *p, const char* why)
{

        assert(initialized() && type_ == DROP);

	if (!node_energy()) {
	    Packet::free(p);
	    return;
	}


#if 0
        /*
         * When the originator of a packet drops the packet, it may or may
         * not have been stamped by GOD.  Stamp it before logging the
         * information.
         */
        if(src_ == ih->saddr()) {
                God::instance()->stampPacket(p);
        }
#endif
	format(p, why);
	dump();
	Packet::free(p);
}

int
CMUTrace::node_energy()
{

	Node* thisnode = Node::get_node_by_address(src_);

	double energy = 1;

	if (thisnode) {
	    if (thisnode->energy_model()) {
	       energy = thisnode->energy();
	    }
	} 

	if (energy > 0) return 1;
	printf("DEBUG: node %d dropping pkts due to energy = 0\n", src_);
	return 0;

}

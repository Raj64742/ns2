#include <packet.h>
#include <ip.h>
#include <tcp.h>
#include <rtp.h>
#include <arp.h>
#include <dsr/hdr_sr.h>	// DSR
#include <new-mac.h>
#include <new-mac-802_11.h>

#include <cmu-trace.h>

// #define LOG_POSITION

extern char* pt_names[];

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

        node_ = 0;

	bind("off_mac_", &off_mac_);
	bind("off_arp_", &off_arp_);
	bind("off_SR_", &off_sr_);
}

void
CMUTrace::format_mac(Packet *p, const char *why, int offset)
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_mac802_11 *mh = HDR_MAC(p);
	char op = (char) type_;

	if(tracetype == TR_ROUTER && type_ == SEND) {
		if(src_ != ih->src_)
			op = FWRD;
	}

#ifdef LOG_POSITION
        double x = 0.0, y = 0.0, z = 0.0;
        node_->getLoc(&x, &y, &z);
#endif

	sprintf(wrk_ + offset,
#ifdef LOG_POSITION
		"%c %.9f %d (%6.2f %6.2f) %3s %4s %d %s %d [%x %x %x %x %x] ",
#else
		"%c %.9f _%d_ %3s %4s %d %s %d [%x %x %x %x %x] ",
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
		pt_names[ch->ptype()],
		ch->size(),

		*((u_int16_t*) &mh->dh_fc),
		mh->dh_duration,
		ETHER_ADDR(mh->dh_da),
		ETHER_ADDR(mh->dh_sa),
		GET_ETHER_TYPE(mh->dh_body));
}

void
CMUTrace::format_ip(Packet *p, int offset)
{
        struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);

	sprintf(wrk_ + offset, "------- [%d:%d %d:%d %d %d] ",
		ih->src_, ih->sport_,
		ih->dst_, ih->dport_,
		ih->ttl_, (ch->next_hop_ < 0) ? 0 : ch->next_hop_);
}

void
CMUTrace::format_arp(Packet *p, int offset)
{
	struct hdr_arp *ah = HDR_ARP(p);

	sprintf(wrk_ + offset,
		"------- [%s %d/%d %d/%d]",
		ah->arp_op == ARPOP_REQUEST ?  "REQUEST" : "REPLY",
		ah->arp_sha,
		ah->arp_spa,
		ah->arp_tha,
		ah->arp_tpa);
}

void
CMUTrace::format_dsr(Packet *p, int offset)
{
	hdr_sr *srh = (hdr_sr*)p->access(off_sr_);
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
	
	sprintf(wrk_ + offset,
		"[%d %d] %d %d",
		th->seqno_,
		th->ackno_,
		ch->num_forwards(),
		ch->opt_num_forwards());
}

void
CMUTrace::format_rtp(Packet *p, int offset)
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_rtp *rh = HDR_RTP(p);

	sprintf(wrk_ + offset,
		"[%d] %d %d",
		rh->seqno_,
		ch->num_forwards(),
		ch->opt_num_forwards());
}


void CMUTrace::format(Packet* p, const char *why)
{
	hdr_cmn *ch = HDR_CMN(p);
	int offset = 0;

	/*
	 * Log the MAC Header
	 */
	format_mac(p, why, offset);
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

		case PT_DSR:
			format_dsr(p, offset);
			break;

		case PT_MESSAGE:
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
				__PRETTY_FUNCTION__, pt_names[ch->ptype()]);
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
        }
	return Trace::command(argc, argv);
}

void
CMUTrace::recv(Packet *p, Handler *h)
{
        struct hdr_ip *ih = HDR_IP(p);

        assert(initialized());

        /*
         * Agent Trace "stamp" the packet with the optimal route on
         * sending.
         */
        if(tracetype == TR_AGENT && type_ == SEND) {
                assert(src_ == ih->src_);
                God::instance()->stampPacket(p);
        }
#if 0
        /*
         * When the originator of a packet drops the packet, it may or may
         * not have been stamped by GOD.  Stamp it before logging the
         * information.
         */
        if(src_ == ih->src_ && type_ == DROP) {
                God::instance()->stampPacket(p);
        }
#endif
	format(p, "---");
	dump();
	if(target_ == 0)
		Packet::free(p);
	else
		send(p, h);
}

void
CMUTrace::recv(Packet *p, const char* why)
{
        assert(initialized() && type_ == DROP);
#if 0
        /*
         * When the originator of a packet drops the packet, it may or may
         * not have been stamped by GOD.  Stamp it before logging the
         * information.
         */
        if(src_ == ih->src_) {
                God::instance()->stampPacket(p);
        }
#endif
	format(p, why);
	dump();
	Packet::free(p);
}


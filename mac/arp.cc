/* arp.cc
   basic arp cache and MAC addr resolution
   $Id: arp.cc,v 1.1 1998/12/08 19:14:14 haldar Exp $

   Note: code in this file violates the convention that addresses of
   type Af_INET stored in nsaddr_t variables are stored in 24/8 format.
   Many variables in nsaddr_t's in this file store ip addrs as simple ints.
   */

#include <errno.h>

#include <delay.h>

#include <debug.h>
#include <new-mac.h>
#include <arp.h>
#include <topography.h>
#include <cmu-trace.h>
#include <mobilenode.h>
#include <new-ll.h>
#include <packet.h>


// #define DEBUG

static class ARPTableClass : public TclClass {
public:
        ARPTableClass() : TclClass("ARPTable") {}
        TclObject* create(int, const char*const* argv) {
                return (new ARPTable(argv[4], argv[5]));
        }
} class_arptable;

static class ARPHeaderClass : public PacketHeaderClass {
public:
        ARPHeaderClass() : PacketHeaderClass("PacketHeader/ARP",
                                             sizeof(hdr_arp)) { }
} class_arphdr;


/* ======================================================================
   Address Resolution (ARP) Table
   ====================================================================== */

ARPTable_List ARPTable::athead = { 0 };

void
ARPTable::Terminate()
{
	ARPEntry *ll;
	for(ll = arphead.lh_first; ll; ll = ll->arp_link.le_next) {
		if(ll->hold) {
			drop(ll->hold, DROP_END_OF_SIMULATION);
			ll->hold = 0;
		}
	}
}


ARPTable::ARPTable(const char *tclnode, const char *tclmac) : LinkDelay() {
	LIST_INIT(&arphead);

        node = (MobileNode*) TclObject::lookup(tclnode);
	assert(node);

	mac = (newMac*) TclObject::lookup(tclmac);
	assert(mac);

	bind("off_ll_", &off_ll_);
	bind("off_mac_", &off_mac_);
	bind("off_arp_", &off_arp_);

	LIST_INSERT_HEAD(&athead, this, link);
}

int
ARPTable::command(int argc, const char*const* argv)
{

  if (argc == 2 && strcasecmp(argv[1], "reset") == 0)
    {
      Terminate();
      //FALL-THROUGH to give parents a chance to reset
    }

  return LinkDelay::command(argc, argv);
}


int
ARPTable::arpresolve(Packet *p, newLL *ll)
{
        ARPEntry *llinfo ;
	hdr_cmn *ch = HDR_CMN(p);
	hdr_ip *ih = HDR_IP(p);
	nsaddr_t dst = ih->dst();
	
	assert(initialized());

	switch(ch->addr_type()) {

	case AF_LINK:
	  mac->hdr_dst((char*) HDR_MAC(p), ch->next_hop());
	  return 0;

	case AF_INET:
	  dst = ch->next_hop();
	  /* FALL THROUGH */
	  
	case AF_NONE:
	  if (IP_BROADCAST == (u_int32_t) dst)
	    {
	      mac->hdr_dst((char*) HDR_MAC(p), MAC_BROADCAST);
	      return 0;
	    }
	   llinfo = arplookup(dst);
	  break;
	  
	default:
	  fprintf(stderr,"%s: unknown address type %d in arpresolve\n",
		  __FILE__, ch->addr_type());
	  exit(-1);
	}

#ifdef DEBUG
        fprintf(stderr, "%d - %s\n", node->index(), __FUNCTION__);
#endif
	
	if(llinfo && llinfo->up) {
		mac->hdr_dst((char*) HDR_MAC(p), llinfo->macaddr);
		return 0;
	}

	if(llinfo == 0) {
		/*
		 *  Create a new ARP entry
		 */
		llinfo = new ARPEntry(&arphead, dst);
	}

        if(llinfo->count >= ARP_MAX_REQUEST_COUNT) {
                /*
                 * Because there is not necessarily a scheduled event between
                 * this callback and the point where the callback can return
                 * to this point in the code, the order of operations is very
                 * important here so that we don't get into an infinite loop.
                 *                                      - josh
                 */
                Packet *t = llinfo->hold;

                llinfo->count = 0;
                llinfo->hold = 0;

                if(t) {
                        ch = HDR_CMN(t);

                        if (ch->xmit_failure_) {
                                ch->xmit_reason_ = 0;
                                ch->xmit_failure_(t, ch->xmit_failure_data_);
                        }
                        else {
                                drop(t, DROP_IFQ_ARP_FULL);
                        }
                }

                ch = HDR_CMN(p);

		if (ch->xmit_failure_) {
                        ch->xmit_reason_ = 0;
                        ch->xmit_failure_(p, ch->xmit_failure_data_);
                }
                else {
                        drop(p, DROP_IFQ_ARP_FULL);
                }

                return EADDRNOTAVAIL;
        }

	llinfo->count++;
	if(llinfo->hold)
		drop(llinfo->hold, DROP_IFQ_ARP_FULL);
	llinfo->hold = p;

	/*
	 *  We don't have a MAC address for this node.  Send an ARP Request.
	 *
	 *  XXX: Do I need to worry about the case where I keep ARPing
	 *	 for the SAME destination.
	 */
	int src = node->index(); // this host's IP addr
	arprequest(src, dst, ll);
	return EADDRNOTAVAIL;
}


ARPEntry*
ARPTable::arplookup(nsaddr_t dst)
{
	ARPEntry *a;

	for(a = arphead.lh_first; a; a = a->nextarp()) {
		if(a->ipaddr == dst)
			return a;
	}
	return 0;
}


void
ARPTable::arprequest(nsaddr_t src, nsaddr_t dst, newLL *ll)
{
	Scheduler& s = Scheduler::instance();
	Packet *p = Packet::alloc();

	hdr_cmn *ch = HDR_CMN(p);
	char	*mh = (char*) HDR_MAC(p);
	newhdr_ll	*lh = HDR_LL(p);
	hdr_arp	*ah = HDR_ARP(p);

	ch->uid() = 0;
	ch->ptype() = PT_ARP;
	ch->size() = ARP_HDR_LEN;
	ch->iface() = -2;
	ch->error() = 0;

	mac->hdr_dst(mh, MAC_BROADCAST);
	mac->hdr_src(mh, ll->mac_->addr());
	mac->hdr_type(mh, ETHERTYPE_ARP);

	lh->seqno() = 0;
	lh->lltype() = LL_DATA;

	ah->arp_hrd = ARPHRD_ETHER;
	ah->arp_pro = ETHERTYPE_IP;
	ah->arp_hln = ETHER_ADDR_LEN;
	ah->arp_pln = sizeof(nsaddr_t);
	ah->arp_op  = ARPOP_REQUEST;
	ah->arp_sha = ll->mac_->addr();
	ah->arp_spa = src;
	ah->arp_tha = 0;		// what were're looking for
	ah->arp_tpa = dst;

	s.schedule(ll->downtarget_, p, delay_);
}

void
ARPTable::arpinput(Packet *p, newLL *ll)
{
	Scheduler& s = Scheduler::instance();
	hdr_arp *ah = HDR_ARP(p);
	ARPEntry *llinfo;

	assert(initialized());

#ifdef DEBUG
	fprintf(stderr,
                "%d - %s\n\top: %x, sha: %x, tha: %x, spa: %x, tpa: %x\n",
		node->index(), __FUNCTION__, ah->arp_op,
                ah->arp_sha, ah->arp_tha, ah->arp_spa, ah->arp_tpa);
#endif

	if((llinfo = arplookup(ah->arp_spa)) == 0) {

		/*
		 *  Create a new ARP entry
		 */
		llinfo = new ARPEntry(&arphead, ah->arp_spa);
	}
        assert(llinfo);

	llinfo->macaddr = ah->arp_sha;
	llinfo->up = 1;

	/*
	 *  Can we send whatever's being held?
	 */
	if(llinfo->hold) {
		hdr_cmn *ch = HDR_CMN(llinfo->hold);
		char *mh = (char*) HDR_MAC(llinfo->hold);
                hdr_ip *ih = HDR_IP(llinfo->hold);
                
		if((ch->addr_type() == AF_NONE &&
                    ih->dst() == ah->arp_spa) ||
                   (AF_INET == ch->addr_type() &&
                    ch->next_hop() == ah->arp_spa)) {
#ifdef DEBUG
			fprintf(stderr, "\tsending HELD packet.\n");
#endif
			mac->hdr_dst(mh, ah->arp_sha);
			s.schedule(ll->downtarget_, llinfo->hold, delay_);
			llinfo->hold = 0;
		}
                else {
                        fprintf(stderr, "\tfatal ARP error...\n");
                        exit(1);
                }
	}

	if(ah->arp_op == ARPOP_REQUEST &&
		ah->arp_tpa == node->index()) {
		
		hdr_cmn *ch = HDR_CMN(p);
		char	*mh = (char*)HDR_MAC(p);
		newhdr_ll  *lh = HDR_LL(p);

		ch->size() = ARP_HDR_LEN;
		ch->error() = 0;

		mac->hdr_dst(mh, ah->arp_sha);
		mac->hdr_src(mh, ll->mac_->addr());
		mac->hdr_type(mh, ETHERTYPE_ARP);

		lh->seqno() = 0;
		lh->lltype() = LL_DATA;

		// ah->arp_hrd = 
		// ah->arp_pro =
		// ah->arp_hln =
		// ah->arp_pln =

		ah->arp_op  = ARPOP_REPLY;
		ah->arp_tha = ah->arp_sha;
		ah->arp_sha = ll->mac_->addr();

		nsaddr_t t = ah->arp_spa;
		ah->arp_spa = ah->arp_tpa;
		ah->arp_tpa = t;

		s.schedule(ll->downtarget_, p, delay_);
		return;
	}
	Packet::free(p);
}


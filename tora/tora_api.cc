/* 
   tora_api.cc
   $Id: tora_api.cc,v 1.1 1999/08/03 04:07:05 yaxu Exp $
   
   Implement the API used by IMEP
   */


#include <tora/tora.h>

#define	CURRENT_TIME	Scheduler::instance().clock()

void
toraAgent::rtNotifyLinkUP(nsaddr_t index)
{
	TORADest *td = dstlist.lh_first;

        /*
         * Update the destination lists...
         */
	for( ; td; td = td->link.le_next) {
		if(td->nb_find(index) == 0) {
                        (void) td->nb_add(index);
		}
		if (td->rt_req) 
		  { // must send a new query for this dest so the new
		    // neighbor can hear it
		    // IMEP will take care of aggregating all these into
		    // one physical pkt

		    trace("T %.9f _%d_ QRY %d for %d (rtreq set)",
			  Scheduler::instance().clock(), ipaddr(), 
			  td->index, index);

		    sendQRY(td->index);
		    td->time_tx_qry = CURRENT_TIME;
		}
	}
}

void
toraAgent::rtNotifyLinkDN(nsaddr_t index)
{
	TORADest *td = dstlist.lh_first;

        /*
         *  Purge each Destination's Neighbor List
         */
        for( ; td; td = td->link.le_next) {
                if(td->nb_del(index)) {
                        /*
                         * Send an UPD packet if you've lost the last
                         * downstream link.
                         */
                        sendUPD(td->index);
                }
        }

	/*
	 *  Now purge the Network Interface queues that
	 *  may have packets destined for this broken
	 *  neighbor.
	 */
        {
                Packet *head = 0;
                Packet *p;
                Packet *np = 0;

                while((p = ifqueue->filter(index))) {
                        p->next_ = head;
                        head = p;
                }

                for(p = head; p; p = np) {
                        np = p->next_;
                        /*
                         * This make a lot of sense for TORA since we
                         * will almost always have multiple routes to
                         * a destination.
                         */
                        log_link_layer_recycle(p);
                        rt_resolve(p);
                }
        }
}


void
toraAgent::rtNotifyLinkStatus(nsaddr_t /* index */, u_int32_t /* status */)
{
	abort();
}

void
toraAgent::rtRoutePacket(Packet *p)
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ip = HDR_IP(p);

	// Non-data packets and BROADCAST packets can be dropped.
	if(DATA_PACKET(ch->ptype()) == 0 || 
	   ip->dst() == (nsaddr_t) IP_BROADCAST) {
		drop(p, DROP_RTR_MAC_CALLBACK);
		return;
	}

	rt_resolve(p);
}

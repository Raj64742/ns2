/*
  tora.cc
  $Id: tora.cc,v 1.4 1999/08/11 22:43:24 yaxu Exp $
  */

#include <agent.h>
#include <random.h>
#include <trace.h>

#include <ll.h>
#include <priqueue.h>
#include <tora/tora_packet.h>
#include <tora/tora.h>

#define LOG(s)							\
	fprintf(stdout, "%s --- %s (index: %d, time: %f)\n",	\
		__PRETTY_FUNCTION__, (s), index, Scheduler::instance().clock())

// #define DEBUG
#define	CURRENT_TIME	Scheduler::instance().clock()

/* sec of spacing inserted between pkts when a bunch of packets are
   dumped into the link layer all at once.  Allows arp time to resolve
   dst, preventing the dumping of all but the last pkt on the floor */
#define ARP_SEPARATION_DELAY 0.030
	  

/* ======================================================================
   TCL Hooks
   ====================================================================== */
static class TORAHeaderClass : public PacketHeaderClass {
public:
        TORAHeaderClass() : PacketHeaderClass("PacketHeader/TORA",
					      TORA_HDR_LEN) { } 
} class_toraAgent_hdr;

static class toraAgentclass : public TclClass {
public:
	toraAgentclass() : TclClass("Agent/rtProto/TORA") {}
	TclObject* create(int argc, const char*const* argv) {
		assert(argc == 5);
		return (new toraAgent((nsaddr_t) atoi(argv[4])));
	}
} class_toraAgent;


/* ======================================================================
   toraAgent Class Functions
   ====================================================================== */
toraAgent::toraAgent(nsaddr_t id) :
	rtAgent(id, PT_TORA),
	rqueue()
{
	bind("off_TORA_", &off_TORA_);
	LIST_INIT(&dstlist);
	imepagent = 0;
	logtarget = 0;
	ifqueue = 0;
}

void
toraAgent::reset()
{
  Packet *p;
  while((p = rqueue.deque())) 
    {
      drop(p,DROP_END_OF_SIMULATION);
    }

}

int
toraAgent::command(int argc, const char*const* argv)
{
	if(argc == 2) {
		Tcl& tcl = Tcl::instance();

		if(strncasecmp(argv[1], "id", 2) == 0) {
			tcl.resultf("%d", index);
			return TCL_OK;
		}
	}
	else if(argc == 3) {

		if(strcmp(argv[1], "log-target") == 0) {
			logtarget = (Trace*) TclObject::lookup(argv[2]);
			if(logtarget == 0)
				return TCL_ERROR;
			return TCL_OK;
		}
		else if(strcmp(argv[1], "drop-target") == 0) {
		        int stat = rqueue.command(argc,argv);
			if (stat != TCL_OK) return stat;
                	return Agent::command(argc, argv);
		}
		else if(strcmp(argv[1], "if-queue") == 0) {
			ifqueue = (PriQueue*) TclObject::lookup(argv[2]);
			if(ifqueue == 0)
				return TCL_ERROR;
			return TCL_OK;
		}
		else if(strcmp(argv[1], "imep-agent") == 0) {
			imepagent = (imepAgent*) TclObject::lookup(argv[2]);
			if(imepagent == 0)
				return TCL_ERROR;
			imepagent->imepRegister((rtAgent*) this);
			return TCL_OK;
		}
	}
	return Agent::command(argc, argv);
}


/* ======================================================================
   Destination Management Functions
   ====================================================================== */
TORADest*
toraAgent::dst_find(nsaddr_t id)
{
	TORADest* td = dstlist.lh_first;
	for( ; td; td = td->link.le_next) {
		if(td->index == id)
			return td;
	}
	return 0;
}

TORADest*
toraAgent::dst_add(nsaddr_t id)
{
	TORADest *td = new TORADest(id, this);
	assert(td);

	LIST_INSERT_HEAD(&dstlist, td, link);

	int *nblist = 0, nbcnt = 0;
	imepagent->imepGetBiLinks(nblist, nbcnt);

	for(int i = 0; i < nbcnt; i++)
		(void) td->nb_add(nblist[i]);

	if(nblist) delete[] nblist;

	return td;
}

void
toraAgent::dst_dump()
{
	TORADest *td = dstlist.lh_first;

	for( ; td; td = td = td->link.le_next)
		td->dump();
}


/* ======================================================================
   Route Resolution
   ====================================================================== */
void
toraAgent::forward(Packet *p, nsaddr_t nexthop, Time delay)
{
        struct hdr_cmn *ch = HDR_CMN(p);

#ifdef TORA_DISALLOW_ROUTE_LOOP
        if(nexthop == ch->prev_hop_) {
		log_route_loop(ch->prev_hop_, nexthop);
                drop(p, DROP_RTR_ROUTE_LOOP);
                return;
        }
#endif
        ch->next_hop() = nexthop;
        ch->prev_hop_ = ipaddr();
        ch->addr_type() = AF_INET;

	if (0.0 == delay) {
	  tora_output(p);
	} else {
	  Scheduler::instance().schedule(target_, p, delay);
	}
}


void
toraAgent::rt_resolve(Packet *p)
{
	struct hdr_ip *ih = HDR_IP(p);
	TORADest *td;
	TORANeighbor *tn;

	td = dst_find(ih->dst_);
	if(td == 0) {
		td = dst_add(ih->dst_);
	}

	tn = td->nb_find_next_hop();
	if(tn == 0) {
		rqueue.enque(p);

		trace("T %.9f _%d_ tora enq %d->%d",
		      Scheduler::instance().clock(), ipaddr(), 
		      ih->src(), ih->dst());

		if(!td->rt_req)
		  { // if no QRY pending, then send one
		    sendQRY(ih->dst_);
		    td->time_tx_qry = CURRENT_TIME;
		    td->rt_req = 1;
		  }
	}
	else {
                forward(p, tn->index);
	}
}


/* ======================================================================
   Incoming Packets
   ====================================================================== */
void
toraAgent::recv(Packet *p, Handler *)
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);

	assert(initialized());

        //assert(p->incoming == 0);

	if(ch->ptype() == PT_TORA) {
		recvTORA(p);
		return;
	}

        /*
         *  Must be a packet I'm originating...
         */
	if(ih->src_ == ipaddr() && ch->num_forwards() == 0) {
                /*
                 *  Add the IP Header.
                 */
                ch->size() += IP_HDR_LEN;
                
                ih->ttl_ = IP_DEF_TTL;
	}

#ifdef TORA_DISALLOW_ROUTE_LOOP
        /*
         *  I received a packet that I sent.  Probably
         *  a routing loop.
         */
        else if(ih->src_ == ipaddr()) {
                drop(p, DROP_RTR_ROUTE_LOOP);
                return;
        }
#endif
	/*
	 *  Packet I'm forwarding...
	 */
	else {
		/*
		 *  Check the TTL.  If it is zero, then discard.
		 */
		if(--ih->ttl_ == 0) {
			drop(p, DROP_RTR_TTL);
			return;
		}
	}

	rt_resolve(p);
}

void
toraAgent::recvTORA(Packet *p)
{
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_tora *th = HDR_TORA(p);
	TORADest *td;
	TORANeighbor *tn;

	/*
	 * Fix the source IP address.
	 */
	assert(ih->sport_ == RT_PORT);
	assert(ih->dport_ == RT_PORT);

	/*
	 * Incoming Packets.
	 */
	switch(th->th_type) {
		case TORATYPE_QRY:
			recvQRY(p);
			Packet::free(p);
			return;		// don't add/change routing state

		case TORATYPE_UPD:
			log_recv_upd(p);
			recvUPD(p);
			break;

		case TORATYPE_CLR:
			log_recv_clr(p);
			recvCLR(p);
			break;

		default:
			fprintf(stderr,
				"%s: Invalid TORA type (%x)\n",
				__PRETTY_FUNCTION__, th->th_type);
			exit(1);
	}

	if((td = dst_find(th->th_dst)) == 0) {
		Packet::free(p);
		return;
	}

	logNextHopChange(td);

	if((tn = td->nb_find_next_hop())) {
		Packet *p0;
		Time delay = 0.0;

		while((p0 = rqueue.deque(td->index))) {
                        forward(p0, tn->index, delay);
			delay += ARP_SEPARATION_DELAY;
		}
	}
	Packet::free(p);
}



/*
 *  IETF Draft - TORA Specification, section 3.7.6
 */
void
toraAgent::recvQRY(Packet *p)
{
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_tora_qry *qh = HDR_TORA_QRY(p);
	TORADest *td;
	TORANeighbor *tn;

	if(qh->tq_dst == ipaddr()) {
#ifdef DEBUG
		fprintf(stderr, "node %d received `QRY` for itself.\n", index);
#endif
		return;
	}

	td = dst_find(qh->tq_dst);
	if(td == 0)
		td = dst_add(qh->tq_dst);

	if(td->rt_req) {
		return;
	}

	if(td->height.r == 0) {					// II, A
		tn = td->nb_find(ih->src_);

		if(tn && tn->time_act > td->time_upd) {		// II, A, 1
			td->time_upd = Scheduler::instance().clock();
			sendUPD(td->index);
		}
		else {						// II, A, 2
		}
	}
	else {
		tn = td->nb_find_min_height(0);

		if(tn) {					// II, B, 1
			td->update_height(tn->height.tau,
					  tn->height.oid,
					  tn->height.r,
					  tn->height.delta + 1,
					  ipaddr());

			td->time_upd = Scheduler::instance().clock();

			sendUPD(td->index);
		}
		else {
			td->rt_req = 1;
			td->time_rt_req = CURRENT_TIME;

			if(td->num_active > 1) {		// II, B, 1, a
				sendQRY(td->index);
			}
			else {					// II, B, 1, b

			}
		}
	}
}


/*
 *  IETF Draft - TORA Specification, section 3.7.7
 */
void
toraAgent::recvUPD(Packet *p)
{
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_tora_upd *uh = HDR_TORA_UPD(p);
	TORADest *td;
	TORANeighbor *tn;

	if(uh->tu_dst == ipaddr()) {
		return;
	}

	td = dst_find(uh->tu_dst);
	if(td == 0)
		td = dst_add(uh->tu_dst);

	tn = td->nb_find(ih->src_);
	if(tn == 0) {
		/*
		 * update link status? -josh
		 */

	         // No, don't update linkstatus: it may be an update
	         // that was delayed in the IMEP layer for sequencing -dam
	         // no way at the TORA level to tell if we're connected...
	         trace("T %.9f _%d_ received `UPD` from non-neighbor %d",
		       CURRENT_TIME, ipaddr(), ih->src_);		 
#ifdef DEBUG
		fprintf(stderr,
                        "node %d received `UPD` from non-neighbor %d\n",
			index, ih->src_);
#endif
		return;
	}

	/*
	 *  Update height and link status for neighbor [j][k].
	 */
	td->update_height_nb(tn, uh);

	if(td->rt_req && tn->height.r == 0) {								// I

		td->update_height(tn->height.tau,
				  tn->height.oid,
				  tn->height.r,
				  tn->height.delta + 1,
				  ipaddr());

		td->rt_req = 0;

		td->time_upd = Scheduler::instance().clock();

		sendUPD(td->index);
	}
	else if(td->num_down == 0) {									// II
		if(td->num_up == 0) {									// II, A
			if(td->height.isNull())								// II, A, 1
				return;									// II, A, 1, a
			else {
				td->height.Null();							// II, A, 1, b 

				td->time_upd = Scheduler::instance().clock();

				sendUPD(td->index);
			}
		}
		else {
			if(td->nb_check_same_ref()) {							// II, A, 2
				TORANeighbor *tn;

				if( (tn = td->nb_find_min_height(0)) ) {			// II, A, 2, a
					td->update_height(tn->height.tau,				// II, A, 2, a, i 
							  tn->height.oid,
							  1,
							  0,
							  ipaddr());
					td->time_upd = Scheduler::instance().clock();

					sendUPD(td->index);
				}
				else {
					if(td->height.oid == ipaddr()) {					// II, A, 2, a, ii
						double temp_tau = td->height.tau;			// II, A, 2, a, ii, x 
						nsaddr_t temp_oid = td->height.oid;

						td->height.Null();
						td->num_down = 0;
						td->num_up = 0;

						/*
						 *  For every active link n, if the neighbor connected
						 *  via link n is the destination j, set HT_NEIGH[j][n]=ZERO
						 *  and LNK_STAT[j][n] = DN.
						 *  Otherwise, set HT_NEIGH[j][n] = NULL and LNK_STAT[j][n] = UN.
						 */
						for(tn = td->nblist.lh_first; tn; tn = tn->link.le_next) {
							if(tn->index == td->index) {
								tn->height.Zero();
								tn->lnk_stat = LINK_DN;

							}
							else {
								tn->height.Null();
								tn->lnk_stat = LINK_UN;
							}
						}

						sendCLR(td->index, temp_tau, temp_oid);
					}
					else {
						td->update_height(Scheduler::instance().clock(),	// II, A, 2, a, ii, y
								  ipaddr(),
								  0,
								  0,
								  ipaddr());
						td->rt_req = 0;
						td->time_upd = Scheduler::instance().clock();

#ifdef DEBUG
						// under what circumstances does this rule fire?
						// seems like it will prevent the detection of 
						// partitions...??? -dam 8/24/98

						if (logtarget) 
						  {
						    sprintf(logtarget->buffer(), "T %.9f _%d_ rule IIA2a(ii)x fires %d",
							    Scheduler::instance().clock(), ipaddr(), td->index);
						    logtarget->dump();
						  }
#endif
						sendUPD(td->index);
					}
				}

			}
			else {
				TORANeighbor *n = td->nb_find_max_height();				// II, A, 2, b
				assert(n);
				TORANeighbor *m = td->nb_find_min_nonnull_height(&n->height);
				assert(m);

				td->update_height(m->height.tau,
						  m->height.oid,
						  m->height.r,
						  m->height.delta - 1,
						  ipaddr());

				td->time_upd = Scheduler::instance().clock();

				sendUPD(td->index);
			}
		}
	}
	else {												// II, B

	}
}


/*
 *  IETF Draft - TORA Specification, section 3.7.8
 */
void
toraAgent::recvCLR(Packet *p)
{
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_tora_clr *th = HDR_TORA_CLR(p);
	TORADest *td;
	TORANeighbor *tn;

	if(th->tc_dst == ipaddr()) {
		return;
	}

	td = dst_find(th->tc_dst);
	if(td == 0)
		td = dst_add(th->tc_dst);
	assert(td);

	if(td->height.tau == th->tc_tau &&
	   td->height.oid == th->tc_oid &&
	   td->height.r == 1) {					// I
		double temp_tau = td->height.tau;
		nsaddr_t temp_oid = td->height.oid;

		td->height.Null();
		td->num_up = 0;
		td->num_down = 0;

		for(tn = td->nblist.lh_first; tn; tn = tn->link.le_next) {
			if(tn->index == td->index) {
				tn->height.Zero();
				tn->lnk_stat = LINK_DN;
			}
			else {
				tn->height.Null();
				tn->lnk_stat = LINK_UN;
			}
		}
		if(td->num_active > 1) {			// I, A
			sendCLR(td->index, temp_tau, temp_oid);
		}
		else {						// I, B
		}
	}
	else {
		tn = td->nb_find(ih->src_);			// II
		if(tn == 0) {
			/*
			 *  XXX - update link status?
			 */
		        trace("T %.9f _%d_ received `CLR` from non-neighbor %d",
		               CURRENT_TIME, index, ih->src_);		
#ifdef DEBUG
			fprintf(stderr,
				"node %d received `CLR` from non-neighbor %d\n",
				index, ih->src_);
#endif
			return;
		}

		tn->height.Null();
		tn->lnk_stat = LINK_UN;

		for(tn = td->nblist.lh_first; tn; tn = tn->link.le_next) {
			if(tn->height.tau == th->tc_tau &&
			   tn->height.oid == th->tc_oid &&
			   tn->height.r == 1) {
				tn->height.Null();
				tn->lnk_stat = LINK_UN;
                        }
		}
		if(td->num_down == 0) {				// II, A
			if(td->num_up == 0) {			// II, A, 1
				if(td->height.isNull()) {	// II, A, 1, a
				}
				else {
					td->height.Null();
					td->time_upd = Scheduler::instance().clock();
					sendUPD(td->index);
				}
			}
			else {
				td->update_height(Scheduler::instance().clock(),
						  ipaddr(),
						  0,
						  0,
						  ipaddr());
				td->rt_req = 0;
				td->time_upd = Scheduler::instance().clock();
				sendUPD(td->index);
			}
		}
		else {
								// II, B
		}
	}
}

void
toraAgent::trace(char* fmt, ...)
{
  va_list ap;
  
  if (!logtarget) return;

  va_start(ap, fmt);
  vsprintf(logtarget->buffer(), fmt, ap);
  logtarget->dump();
  va_end(ap);
}



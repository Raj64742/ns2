/*
  $Id: tora_neighbor.cc,v 1.1 1999/08/03 04:07:13 yaxu Exp $
  */

#include <agent.h>
#include <random.h>
#include <trace.h>

#include <ll.h>
#include <priqueue.h>
#include <tora/tora_packet.h>
#include <tora/tora.h>

#define	CURRENT_TIME	Scheduler::instance().clock()

/* ======================================================================
   TORANeighbor Class Functions
   ====================================================================== */
TORANeighbor::TORANeighbor(nsaddr_t id, Agent *a) : height(id)
{
	index = id;  
	lnk_stat = LINK_UN;
	time_act = Scheduler::instance().clock();

        agent = (toraAgent*) a;
}       


void
TORANeighbor::update_link_status(Height *h)
{
#ifdef LOGGING
        int t = lnk_stat;
#endif
	if(height.isNull())
		lnk_stat = LINK_UN;
	else if(h->isNull())
		lnk_stat = LINK_DN;
	else if(height.compare(h) > 0)
		lnk_stat = LINK_DN;
	else if(height.compare(h) < 0)
		lnk_stat = LINK_UP;
#ifdef LOGGING
        if(t != lnk_stat) {
                agent->log_nb_state_change(this);
        }
#endif
}

void
TORANeighbor::dump()
{
	fprintf(stdout, "\t\tNEIG: %d, Link Status: %d, Time Active: %f\n",
		index, lnk_stat, time_act);
	fprintf(stdout, "\t\t\ttau: %f, oid: %d, r: %d, delta: %d, id: %d\n",
		height.tau, height.oid, height.r, height.delta, height.id);
}


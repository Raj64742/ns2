/*
  $Id: tora_io.cc,v 1.1 1999/08/03 04:07:10 yaxu Exp $
  
  marshall TORA packets 
  */

#include <tora/tora_packet.h>
#include <tora/tora.h>

/* ======================================================================
   Outgoing Packets
   ====================================================================== */
void
toraAgent::sendQRY(nsaddr_t id)
{
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_tora_qry *th = HDR_TORA_QRY(p);

	ch->uid() = uidcnt_++;
	ch->ptype() = PT_TORA;
	ch->size() = QRY_HDR_LEN;
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = AF_NONE;
        ch->prev_hop_ = ipaddr();

	ih->src_ = ipaddr();
	ih->dst_ = IP_BROADCAST;
	ih->sport_ = RT_PORT;
	ih->dport_ = RT_PORT;
	ih->ttl_ = 1;

	th->tq_type = TORATYPE_QRY;
	th->tq_dst = id;

	trace("T %.9f _%d_ tora sendQRY %d",
	      Scheduler::instance().clock(), ipaddr(), id);

	tora_output(p);
}


void
toraAgent::sendUPD(nsaddr_t id)
{
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_tora_upd *th = HDR_TORA_UPD(p);
	TORADest *td;

	td = dst_find(id);
	assert(td);

	ch->uid() = uidcnt_++;
	ch->ptype() = PT_TORA;
	ch->size() = UPD_HDR_LEN;
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = AF_NONE;
        ch->prev_hop_ = ipaddr();

	ih->src_ = ipaddr();
	ih->dst_ = IP_BROADCAST;
	ih->sport_ = RT_PORT;
	ih->dport_ = RT_PORT;
	ih->ttl_ = 1;

	th->tu_type = TORATYPE_UPD;
	th->tu_dst = id;
	th->tu_tau = td->height.tau;
	th->tu_oid = td->height.oid;
	th->tu_r = td->height.r;
	th->tu_delta = td->height.delta;
	th->tu_id = td->height.id;

	tora_output(p);
}


void
toraAgent::sendCLR(nsaddr_t id, double tau, nsaddr_t oid)
{
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_tora_clr *th = HDR_TORA_CLR(p);

	ch->uid() = uidcnt_++;
	ch->ptype() = PT_TORA;
	ch->size() = CLR_HDR_LEN;
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = AF_NONE;
        ch->prev_hop_ = ipaddr();

	ih->src_ = ipaddr();
	ih->dst_ = IP_BROADCAST;
	ih->sport_ = RT_PORT;
	ih->dport_ = RT_PORT;
	ih->ttl_ = 1;

	th->tc_type = TORATYPE_CLR;
	th->tc_dst = id;
	th->tc_tau = tau;
	th->tc_oid = oid;

	tora_output(p);
}

void
toraAgent::tora_output(Packet *p)
{
	target_->recv(p, (Handler*) 0);
}


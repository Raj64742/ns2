/*
  imep_io.cc
  $Id: imep_io.cc,v 1.1 1999/08/03 04:12:36 yaxu Exp $

  marshall IMEP packets 
*/

#include <random.h>
#include <packet.h>
#include <imep/imep.h>

// ======================================================================
// ======================================================================
// Outgoing Packets

void
imepAgent::sendBeacon()
{
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_imep *im = HDR_IMEP(p);

	ch->ptype() = PT_IMEP;
	ch->size() = BEACON_HDR_LEN;
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = AF_NONE;
        ch->prev_hop_ = ipaddr;
	ch->uid() = uidcnt_++;

	ih->src_ = ipaddr;
	ih->dst_ = IP_BROADCAST;
	ih->sport_ = RT_PORT;
	ih->dport_ = RT_PORT;
	ih->ttl_ = 1;

	im->imep_version = IMEP_VERSION;
	im->imep_block_flags = 0x00;
	U_INT16_T(im->imep_length) = sizeof(struct hdr_imep);

	imep_output(p);
}

void
imepAgent::sendHello(nsaddr_t index)
{
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_imep *im = HDR_IMEP(p);
	struct imep_hello_block *hb = (struct imep_hello_block*) (im + 1);
	struct imep_hello *hello = (struct imep_hello*) (hb + 1);

	ch->ptype() = PT_IMEP;
	ch->size() = HELLO_HDR_LEN;
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = AF_NONE;
        ch->prev_hop_ = ipaddr;
	ch->uid() = uidcnt_++;

	ih->src_ = ipaddr;
	ih->dst_ = IP_BROADCAST;
	ih->sport_ = RT_PORT;
	ih->dport_ = RT_PORT;
	ih->ttl_ = 1;

	im->imep_version = IMEP_VERSION;
	im->imep_block_flags = BLOCK_FLAG_HELLO;
	U_INT16_T(im->imep_length) = sizeof(struct hdr_imep) +
		sizeof(struct imep_hello_block) + sizeof(struct imep_hello);

	hb->hb_num_hellos = 1;

	INT32_T(hello->hello_ipaddr) = index;

	helloQueue.enque(p);
	// aggregate as many control messages as possible before sending

	if(controlTimer.busy() == 0) {
		controlTimer.start(MIN_TRANSMIT_WAIT_TIME_LOWP
			 + ((MAX_TRANSMIT_WAIT_TIME_LOWP - MIN_TRANSMIT_WAIT_TIME_LOWP)
			 * Random::uniform()));
	}

}

void
imepAgent::sendAck(nsaddr_t index, u_int32_t seqno)
{
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_imep *im = HDR_IMEP(p);
	struct imep_ack_block *ab = (struct imep_ack_block*) (im + 1);
	struct imep_ack *ack = (struct imep_ack*) (ab + 1);

	ch->ptype() = PT_IMEP;
	ch->size() = ACK_HDR_LEN;
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = AF_NONE;
        ch->prev_hop_ = ipaddr;
	ch->uid() = uidcnt_++;

	ih->src_ = ipaddr;
	ih->dst_ = IP_BROADCAST;
	ih->sport_ = RT_PORT;
	ih->dport_ = RT_PORT;
	ih->ttl_ = 1;

	im->imep_version = IMEP_VERSION;
	im->imep_block_flags = BLOCK_FLAG_ACK;
	U_INT16_T(im->imep_length) = sizeof(struct hdr_imep) +
		sizeof(struct imep_ack_block) + sizeof(struct imep_ack)
;
	ab->ab_num_acks = 1;
	ack->ack_seqno = seqno;
	INT32_T(ack->ack_ipaddr) = index;

	// aggregate as many control messages as possible before sending
	ackQueue.enque(p);
	if(controlTimer.busy() == 0) {
		controlTimer.start(MIN_TRANSMIT_WAIT_TIME_LOWP
			 + ((MAX_TRANSMIT_WAIT_TIME_LOWP - MIN_TRANSMIT_WAIT_TIME_LOWP)
			 * Random::uniform()));
	}
}

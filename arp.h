/* -*- c++ -*-
 $Id: arp.h,v 1.1 1998/12/08 19:14:14 haldar Exp $
 */

#ifndef __arp_h__
#define __arp_h__

#include <scheduler.h>
#include <delay.h>
#include <list.h>

class LinkDelay;
class ARPEntry;
class ARPTable;
class newLL;
class newMac;

LIST_HEAD(ARPTable_List, ARPTable);
LIST_HEAD(ARPEntry_List, ARPEntry);


/* ======================================================================
   Address Resolution (ARP) Header
   ====================================================================== */
#define ARPHRD_ETHER		1	/* ethernet hardware format */

#define ARPOP_REQUEST		1	/* request to resolve address */
#define ARPOP_REPLY		2	/* response to previous request */
#define ARPOP_REVREQUEST	3	/* request protocol address */
#define ARPOP_REVREPLY		4	/* response giving protocol address */
#define ARPOP_INVREQUEST	8	/* request to identify peer */
#define ARPOP_INVREPLY		9	/* response identifying peer */

#define ARP_HDR_LEN		28

struct hdr_arp {
	u_int16_t	arp_hrd;
	u_int16_t	arp_pro;
	u_int8_t	arp_hln;
	u_int8_t	arp_pln;
	u_int16_t	arp_op;
	int		arp_sha;
	u_int16_t	pad1;		// so offsets are correct
	nsaddr_t	arp_spa;
	int		arp_tha;
	u_int16_t	pad2;		// so offsets are correct
	nsaddr_t	arp_tpa;
};

class ARPEntry {
	friend class ARPTable;
public:
	ARPEntry(ARPEntry_List* head, nsaddr_t dst) {
		up = macaddr = count = 0;
		ipaddr = dst;
		hold = 0;
		LIST_INSERT_HEAD(head, this, arp_link);
	}
	inline ARPEntry* nextarp() { return arp_link.le_next; }

private:
	LIST_ENTRY(ARPEntry)	arp_link;

	int		up;
	nsaddr_t	ipaddr;
	int		macaddr;
	Packet		*hold;
	int		count;
#define ARP_MAX_REQUEST_COUNT   3
};


class ARPTable : public LinkDelay {
public:
	ARPTable(const char *tclnode, const char *tclmac);

        int     command(int argc, const char*const* argv);
	int     arpresolve(Packet *p, newLL *ll);
	void    arpinput(Packet *p, newLL *ll);

	void	Terminate(void);
protected:
	int off_mac_;
	int off_ll_;
	int off_arp_;

private:
	inline int initialized() { return node && mac; }
	ARPEntry* arplookup(nsaddr_t dst);
	void arprequest(nsaddr_t src, nsaddr_t dst, newLL *ll);

	ARPEntry_List	arphead;
	MobileNode	*node;
	newMac		*mac;

	/*
	 * Used to purge all of the ll->hold packets at the end of the
	 * simulation.
	 */
public:
	LIST_ENTRY(ARPTable) link;
	static ARPTable_List athead;
};

#endif /* __arp_h__ */


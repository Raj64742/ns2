#ifndef __cmu_trace__
#define __cmu_trace__

#include <trace.h>
#include <god.h>

/* ======================================================================
   Global Defines
   ====================================================================== */
#define	DROP            'D'
#define	RECV            'r'
#define	SEND    	's'
#define	FWRD    	'f'

#define TR_ROUTER	0x01
#define TR_MAC		0x02
#define TR_IFQ		0x04
#define TR_AGENT	0x08

#define DROP_END_OF_SIMULATION		"END"
#define	DROP_MAC_COLLISION		"COL"
#define DROP_MAC_DUPLICATE		"DUP"
#define DROP_MAC_PACKET_ERROR		"ERR"
#define DROP_MAC_RETRY_COUNT_EXCEEDED	"RET"
#define DROP_MAC_INVALID_STATE		"STA"
#define DROP_MAC_BUSY			"BSY"

#define DROP_RTR_NO_ROUTE		"NRTE"  // no route
#define DROP_RTR_ROUTE_LOOP		"LOOP"  // routing loop
#define DROP_RTR_TTL                    "TTL"   // ttl reached zero
#define DROP_RTR_QFULL                  "IFQ"   // queue full
#define DROP_RTR_QTIMEOUT               "TOUT"  // packet expired
#define DROP_RTR_MAC_CALLBACK           "CBK"   // MAC callback

#define DROP_IFQ_QFULL                  "IFQ"   // no buffer space in IFQ
#define DROP_IFQ_ARP_FULL               "ARP"   // dropped by ARP

#define MAX_ID_LEN	3

class CMUTrace : public Trace {
public:
	CMUTrace(const char *s, char t);
	void	recv(Packet *p, Handler *h);
	void	recv(Packet *p, const char* why);

private:
	int off_arp_;
	int off_mac_;
	int off_sr_;

	char	tracename[MAX_ID_LEN + 1];
        int     tracetype;
        MobileNode *node_;

        int initialized() { return node_ && 1; }

	int	command(int argc, const char*const* argv);
	void	format(Packet *p, const char *why);

	void	format_mac(Packet *p, const char *why, int offset);
	void	format_ip(Packet *p, int offset);

	void	format_arp(Packet *p, int offset);
	void	format_dsr(Packet *p, int offset);
	void	format_msg(Packet *p, int offset);
	void	format_tcp(Packet *p, int offset);
	void	format_rtp(Packet *p, int offset);
};



#endif /* __cmu_trace__ */

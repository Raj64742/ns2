/* Ported from CMU/Monarch's code, nov'98 -Padma.

*/
#ifndef ns_new_mac_h
#define ns_new_mac_h

#include <assert.h>
#include <new-connector.h>
#include <packet.h>

#include <new-ll.h>
#include <phy.h>

class newMac;



#define EF_COLLISION 2		// collision error flag

/* ======================================================================
   Defines / Macros used by all MACs.
   ====================================================================== */
//#define ETHER_ADDR(x)	*((u_int32_t*) (x))
#define ETHER_ADDR(x)  (u_int32_t)(*(x))
#define MAC_HDR_LEN	64

#define MAC_BROADCAST	((u_int32_t) 0xffffffff)
#define BCAST_ADDR -1

#define ETHER_ADDR_LEN	6
#define ETHER_TYPE_LEN	2
#define ETHER_FCS_LEN	4

#define ETHERTYPE_IP	0x0800
#define ETHERTYPE_ARP	0x0806

enum newMacState {
	newMAC_IDLE	= 0x0000,
	newMAC_POLLING	= 0x0001,
	newMAC_RECV 	= 0x0010,
	newMAC_SEND 	= 0x0100,
	newMAC_RTS		= 0x0200,
	newMAC_CTS		= 0x0400,
	newMAC_ACK		= 0x0800,
	newMAC_COLL	= 0x1000,
};

enum newMacFrameType {
	MF_BEACON	= 0x0008, // beaconing
	MF_CONTROL	= 0x0010, // used as mask for control frame
	MF_SLOTS	= 0x001a, // announce slots open for contention
	MF_RTS		= 0x001b, // request to send
	MF_CTS		= 0x001c, // clear to send, grant
	MF_ACK		= 0x001d, // acknowledgement
	MF_CF_END	= 0x001e, // contention free period end
	MF_POLL		= 0x001f, // polling
	MF_DATA		= 0x0020, // also used as mask for data frame
	MF_DATA_ACK	= 0x0021, // ack for data frames
};

struct newhdr_mac {
	newMacFrameType ftype_;	// frame type
	int macSA_;		// source MAC address
	int macDA_;		// destination MAC address
	double txtime_;		// transmission time
	double sstime_;		// slot start time

	static int offset_;
	inline static int& offset() { return offset_; }
	//inline static newhdr_mac* access(Packet* p) {
	//return (newhdr_mac*) p->access(offset_);
	//}

	inline void set(newMacFrameType ft, int sa, int da=-1) {
		ftype_ = ft;
		macSA_ = sa;
		if (da != -1)  macDA_ = da;
	}
	inline newMacFrameType& ftype() { return ftype_; }
	inline int& macSA() { return macSA_; }
	inline int& macDA() { return macDA_; }
	inline double& txtime() { return txtime_; }
	inline double& sstime() { return sstime_; }
};


/* ======================================================================
   Objects that want to promiscously listen to the packets before
   address filtering must inherit from class Tap in order to plug into
   the tap
   ====================================================================== */
class Tap {
public:
  virtual void tap(const Packet *p) = 0;
  // tap is given all packets received by the host.
  // it must not alter or free the pkt.  If you want to frob it, copy it.
};

class newMacHandlerResume : public Handler {
public:
	newMacHandlerResume(newMac* m) : mac_(m) {}
	void handle(Event*);
protected:
	newMac* mac_;
};

class newMacHandlerSend : public Handler {
public:
	newMacHandlerSend(newMac* m) : mac_(m) {}
	void handle(Event*);
protected:
	newMac* mac_;
};


/* ======================================================================
   MAC data structure
   ====================================================================== */
class newMac : public BiConnector {
public:
	newMac();
	inline int	        addr() { return index_; }
	
	virtual void		recv(Packet* p, Handler* h);

	virtual void		sendDown(Packet *p);
	virtual void		sendUp(Packet *p);
	
	virtual void            resume(Packet* p = 0);
	virtual void            installTap(Tap *t) { tap_ = t; }
	
	inline newMacState state(int m) { return state_ = (newMacState) m; }
	inline newMacState state() { return state_; }
	inline double bandwidth() const { return bandwidth_; }

	// mac methods to set dst, src and hdt_type in pkt hdrs.
	virtual inline int	hdr_dst(char* hdr, u_int32_t dst = 0) = 0;
	virtual inline int	hdr_src(char* hdr, u_int32_t src = 0) = 0;
	virtual inline int	hdr_type(char *hdr, u_int16_t type = 0) = 0;
	
private:
	virtual void		discard(Packet *p, const char* why = 0) {}
	
protected:
	virtual int		command(int argc, const char*const* argv);
	virtual int initialized() {
		return (netif_ && uptarget_ && downtarget_);
	}

	u_int32_t        index_;		// MAC Address
	double		bandwidth_;        // channel bitrate

	double          delay_;         //MAC overhead
	int             off_newmac_;
	
	Phy		*netif_;
        Tap             *tap_;
	newLL		*ll_;
	Handler		*upcall_;	// callback for end-of-transmission
	Event           intr_;
	newMacHandlerResume hRes_;	  // resume handler
	newMacHandlerSend   hSend_;  // handle delay send due to busy channel
	/* ============================================================
	   Internal MAC State
	   ============================================================ */
	newMacState	state_;		// MAC's current state
	Packet		*pktRx_;
	Packet		*pktTx_;
};

#endif /* __mac_h__ */

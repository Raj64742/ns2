#ifndef __mac_802_3_h__
#define __mac_802_3_h__

#include <assert.h>

#define min(x, y)	((x) < (y) ? (x) : (y))
#define ETHER_HDR_LEN		((ETHER_ADDR_LEN << 1) + ETHER_TYPE_LEN)


#define IEEE_8023_SLOT		0.000051200	// 512 bit times
#define	IEEE_8023_IFS		0.000009600	// 9.6us
#define IEEE_8023_ALIMIT	16		// attempt limit
#define IEEE_8023_BLIMIT	10		// backoff limit
#define IEEE_8023_JAMSIZE	32		// bits
// #define IEEE_8023_JAMTIME	(IEEE_8023_JAMSIZE/(1e7))
#define IEEE_8023_MAXFRAME	1518		// bytes
#define IEEE_8023_MINFRAME	64		// bytes

struct hdr_mac802_3 {
	u_char		mh_da[ETHER_ADDR_LEN];
	u_char		mh_sa[ETHER_ADDR_LEN];
	u_int16_t	mh_type;
};


/* ======================================================================
   Handler Functions
   ====================================================================== */
class Mac802_3;

class MacHandler : public Handler {
public:
	MacHandler(Mac802_3* m) :  callback(0), mac(m), busy_(0) {}
	virtual void handle(Event *e) = 0;
	virtual inline void cancel() {
		assert(0);
#if 0
		Scheduler& s = Scheduler::instance();
		assert(busy_);
		s.cancel(&intr);
		busy_ = 0;
#endif
	}
	inline int busy(void) { return busy_; }
	inline double expire(void) { return intr.time_; }
protected:
	Handler		*callback;
	Mac802_3	*mac;
	Event		intr;
	int		busy_;
};


class MacHandlerDefer : public MacHandler {
public:
	MacHandlerDefer(Mac802_3* m) : MacHandler(m) {}
	void handle(Event*);
	void schedule(Handler *h, double t);
};


class Mac8023HandlerSend : public MacHandler {
public:
	Mac8023HandlerSend(Mac802_3* m) : MacHandler(m) {}
	void handle(Event*);
	void schedule(double t);
};


class MacHandlerBack : public MacHandler {
public:
	MacHandlerBack(Mac802_3* m) : MacHandler(m) {}
	void handle(Event*);
	void schedule(Packet *p, double t);
};


class MacHandlerRecv : public MacHandler {
public:
	MacHandlerRecv(Mac802_3* m) : MacHandler(m), p_(0) {}
	void handle(Event*);
	void schedule(Packet *p, double t);
	virtual inline void cancel() {
		Scheduler& s = Scheduler::instance();
		assert(busy_ && p_);
		s.cancel(p_);
		busy_ = 0;
	}
private:
	Packet *p_;
};


/* ======================================================================
   MAC data structure
   ====================================================================== */
class Mac802_3 : public Mac {
	friend class MacHandlerBack;
	friend class MacHandlerRecv;
	friend class Mac8023HandlerSend;
public:
	Mac802_3();

	void		recv(Packet* p, Handler* h);
	/*
	inline int	hdr_dst(char* hdr, u_int32_t dst = -2);
	inline int	hdr_src(char* hdr, u_int32_t src = -2);
	inline int	hdr_type(char* hdr, u_int16_t type = 0);
	*/
	
	void		recv_complete(Packet *p);
	virtual void	resume();

protected:
	virtual void	send(Packet* p);
private:
	int		command(int argc, const char*const* argv);
	void		send(Packet *p, Handler *h);
	void		discard(Packet *p, const char* why = 0);

	virtual void	backoff(void);
	void		collision(Packet *p);


	int		pktTxcnt;

	MacHandlerBack	mhBack;
	MacHandlerDefer	mhDefer;
	MacHandlerRecv	mhRecv;
	Mac8023HandlerSend	mhSend;


};

#endif /* __mac_802_3_h__ */

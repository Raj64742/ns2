#ifndef __mac_timers_h__
#define __mac_timers_h__

/* ======================================================================
   Timers
   ====================================================================== */
class newMac802_11;

class MacTimer : public Handler {
public:
	MacTimer(newMac802_11* m, double s = 0) : mac(m), slottime(s) {
		busy_ = paused_ = 0; stime = rtime = 0.0;
	}

	virtual void handle(Event *e) = 0;

	virtual void start(double time);
	virtual void stop(void);
	virtual void pause(void) { assert(0); }
	virtual void resume(void) { assert(0); }

	inline int busy(void) { return busy_; }
	inline int paused(void) { return paused_; }
	inline double expire(void) {
		return ((stime + rtime) - Scheduler::instance().clock());
	}

protected:
	newMac802_11	*mac;
	int		busy_;
	int		paused_;
	Event		intr;
	double		stime;	// start time
	double		rtime;	// remaining time
	double		slottime;
};


class BackoffTimer : public MacTimer {
public:
	BackoffTimer(newMac802_11 *m, double s) : MacTimer(m, s), difs_wait(0.0) {}

	void	start(int cw, int idle);
	void	handle(Event *e);
	void	pause(void);
	void	resume(double difs);
private:
	double	difs_wait;
};

class DeferTimer : public MacTimer {
public:
	DeferTimer(newMac802_11 *m, double s) : MacTimer(m,s) {}

	void	start(double);
	void	handle(Event *e);
};

class IFTimer : public MacTimer {
public:
	IFTimer(newMac802_11 *m) : MacTimer(m) {}

	void	handle(Event *e);
};

class NavTimer : public MacTimer {
public:
	NavTimer(newMac802_11 *m) : MacTimer(m) {}

	void	handle(Event *e);
};

class RxTimer : public MacTimer {
public:
	RxTimer(newMac802_11 *m) : MacTimer(m) {}

	void	handle(Event *e);
};

class TxTimer : public MacTimer {
public:
	TxTimer(newMac802_11 *m) : MacTimer(m) {}

	void	handle(Event *e);
};

#endif /* __mac_timers_h__ */


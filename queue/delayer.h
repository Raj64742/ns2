#include "connector.h"
#include "random.h"
#include "timer-handler.h"
#include "ranvar.h"

class Delayer;

/* handler for Delayer's target (link) */
class TargetHandler : public Handler {
public:
        inline TargetHandler(Delayer& d) : delayer_(d) {}
        inline void handle(Event*);
private:
        Delayer& delayer_;
};

/* timer for channel allocation delay */
class AllocTimer : public TimerHandler {
public:
        inline AllocTimer(Delayer& d) : delayer_(d) {}
        inline void expire(Event*);
private:
        Delayer& delayer_;
};

/* timer for delay spikes */
class SpikeTimer : public TimerHandler {
public:
        inline SpikeTimer(Delayer& d) : delayer_(d) {}
        inline void expire(Event*);
private:
        Delayer& delayer_;
};

class Delayer : public Connector {
  public:
	Delayer();
 	int command(int argc, const char*const* argv);
 	void recv(Packet* p, Handler* h);
	void try_send();
	inline void freeTarget() {target_free = 1;}
	inline void freeAlloc() {alloc_free = 1;}
	inline void freeSpike() {spike_free = 1;}
	inline void takeSpike() {spike_free = 0;}
	int getSpike() {return spike_free;}
	double getNextSpikeLen () {return spike_len->value(); }
	double getNextSpikeInt () {return spike_int->value(); }

  private:
        double last_sent;       /* time a packet has last left the queue */
        RandomVariable *alloc_int; /* time of keeping channel without data */
        RandomVariable *alloc_len; /* delay to allocate a new channel */
        int alloc_free;              /* are we in channel allocation delay? */
	AllocTimer  at_;

        RandomVariable *spike_int; /* interval between delay spikes */
        RandomVariable *spike_len; /* delay spike length */
        int spike_free;              /* are we in a delay spike? */
	SpikeTimer st_;

	int target_free;	  /* is the link free? */
	TargetHandler th_;
	Handler *prev_h_;	  /* downstream handler */
	Packet  *pkt_;		  /* packet we store */
	Event e;		  /* something to give to prev_h_ */
};

static class DelayerClass : public TclClass {
public:
        DelayerClass() : TclClass("Delayer") {}
        TclObject* create(int, const char*const*) {
                return (new Delayer());
        }
} delayer_class;

#include "delayer.h"

Delayer::Delayer() : Connector(), last_sent(-100000), alloc_int(NULL), 
alloc_len(NULL),  alloc_free(1), at_(*this), 
spike_int(NULL), spike_len(NULL), spike_free(1),
st_(*this), target_free(1), th_ (*this), prev_h_(NULL), pkt_(NULL) {
}
 
void Delayer::recv(Packet* p, Handler* h) {
	double now = Scheduler::instance().clock(); 		

	if (pkt_ != NULL) {
		printf("delayer not empty!\n");
		exit(1);
 	}

	prev_h_ = h;
	pkt_ = p;

        // if queue has been empty then trigger channel allocation delay
        if (alloc_len && now - last_sent > alloc_int->value()) {        
                        alloc_free = 0;
                        at_.resched(alloc_len->value());
                        return;
        }
	try_send();
}

void Delayer::try_send() {
	double now = Scheduler::instance().clock(); 		
	
	if (debug_)
	        printf("now %f last_sent %f alloc_free %d target_free %d, spike_free %d\n", 
			now, last_sent, alloc_free, target_free, spike_free);

	if (!target_free || !alloc_free || !spike_free)
		return;

	if (pkt_ && target_) {
                target_->recv(pkt_, &th_);
		last_sent = Scheduler::instance().clock(); 		
		target_free = 0;
		pkt_ = NULL;
	}
	prev_h_->handle(&e);
}

int Delayer::command(int argc, const char*const* argv) {
        // explicitly block the queue to model a delay spike
        if (argc == 2 && !strcmp(argv[1],"block") 
	&& !spike_len 
	) {
	        spike_free = 0;
        	return (TCL_OK);
        }
        if (argc == 2 && !strcmp(argv[1],"unblock") 
	//	&& !spike_len
	) {
                spike_free = 1;
                try_send();
                return (TCL_OK);
        }
        // set distributions for channel allocation delay
        if (argc == 4 && !strcmp(argv[1],"alloc")) {
                alloc_int = (RandomVariable *)TclObject::lookup(argv[2]);
                alloc_len = (RandomVariable *)TclObject::lookup(argv[3]);
                return (TCL_OK);
        }
        // set distributions for delay spikes
        if (argc == 4 && !strcmp(argv[1],"spike")) {
                spike_int = (RandomVariable *)TclObject::lookup(argv[2]);
                spike_len = (RandomVariable *)TclObject::lookup(argv[3]);
		st_.sched(getNextSpikeInt());
                return (TCL_OK);
        }
        return Connector::command(argc, argv);
}

void SpikeTimer::expire(Event*) {
//	printf("SpikeHandler, spike_free %d\n", delayer_.getSpike());
  	if (delayer_.getSpike()) {
           	delayer_.takeSpike();
               	resched(delayer_.getNextSpikeLen());
              	return;
   	}
   	delayer_.freeSpike();
   	resched(delayer_.getNextSpikeInt());
   	delayer_.try_send();
}

void AllocTimer::expire(Event*) {
	delayer_.freeAlloc(); 
	delayer_.try_send();
//	printf("AllocHandler\n");
}

void TargetHandler::handle(Event*) {
	delayer_.freeTarget(); 
	delayer_.try_send();
//	printf("TargetHandler\n");
}


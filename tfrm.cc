#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
 
#include "tfrm.h"
#include "formula.h"

int hdr_tfrm::offset_;
int hdr_tfrmc::offset_;

static class TFRMHeaderClass : public PacketHeaderClass {
public:
	TFRMHeaderClass() : PacketHeaderClass("PacketHeader/TFRM",
	       sizeof(hdr_tfrm)) {
    bind_offset(&hdr_tfrm::offset_);
  }
} class_tfrmhdr;

static class TFRMCHeaderClass : public PacketHeaderClass {
public:
	TFRMCHeaderClass() : PacketHeaderClass("PacketHeader/TFRMC",
	       sizeof(hdr_tfrmc)) {
    bind_offset(&hdr_tfrmc::offset_);
  }
} class_tfrmchdr;

static class TfrmClass : public TclClass {
public:
  	TfrmClass() : TclClass("Agent/TFRM") {}
  	TclObject* create(int, const char*const*) {
    		return (new TfrmAgent());
  	}
} class_tfrm;


TfrmAgent::TfrmAgent() : Agent(PT_TFRM), tcp_tick_(0.1), 
	incrrate_(0.1), send_timer_(this), df_(0.5), version_(0), slowincr_(0),
	ndatapack_(0)
{
	bind("packetSize_", &size_);
	bind("df_", &df_);
	bind("version_", &version_);
	bind("slowincr_", &slowincr_);
	bind("tcp_tick_", &tcp_tick_);
	bind("incrrate_", &incrrate_);
	bind("ndatapack_", &ndatapack_);
	bind("srtt_init_", &srtt_init_);
	bind("rttvar_init_", &rttvar_init_);
	bind("rtxcur_init_", &rtxcur_init_);
	bind("rttvar_exp_", &rttvar_exp_);
	bind("T_SRTT_BITS", &T_SRTT_BITS);
	bind("T_RTTVAR_BITS", &T_RTTVAR_BITS);
	bind("InitRate_", &InitRate_);
	bind("SampleSizeMult_", &k_);
	bind("bval_", &bval_);
	bind("overhead_", &overhead_);
	bind("ssmult_", &ssmult_);
}


int TfrmAgent::command(int argc, const char*const* argv)
{
	//Tcl& tcl = Tcl::instance();
	if (argc==2) {
		if (strcmp(argv[1],"start")==0) {
			start();
			return TCL_OK;
		}
		if (strcmp(argv[1],"stop")==0) {
			stop();
			return TCL_OK;
		}
	}
	return (Agent::command(argc, argv));
}

void TfrmAgent::start()
{
	oldrate_ = rate_=InitRate_;  /* Starting rate */
	delta_ = 0;
	rtt_=0;	 /* "accurate" rtt */ 
	seqno_=0;					/* Initial Sequence Number */
	run_=1;
	last_change_=0.0;
	rate_change_ = SLOW_START ;

	sendpkt();
	send_timer_.resched(size_/rate_);

	t_srtt_ = int(srtt_init_/tcp_tick_) << T_SRTT_BITS;
	t_rttvar_ = int(rttvar_init_/tcp_tick_) << T_RTTVAR_BITS;
	t_rtxcur_ = rtxcur_init_;

}

void TfrmAgent::stop()
{
	run_=0;
	send_timer_.resched(0.0);
}

void TfrmAgent::nextpkt()
{
	/* cancel any pending send timer */
	send_timer_.force_cancel () ;
	sendpkt();
	if (rate_>0) {
		if ((rate_change_ == SLOW_START) ||
				(rate_change_ == CONG_AVOID &&
				 version_ == 1 && slowincr_ == 3)) {
			oldrate_ = oldrate_ + delta_ ;
			send_timer_.resched((size_/(oldrate_))+Random::uniform(overhead_)); 
		}
		else {
			send_timer_.resched(size_/rate_+Random::uniform(overhead_));
		}
	}
}

void TfrmAgent::update_rtt (double tao, double now) {

	t_rtt_ = int((now-tao) /tcp_tick_ + 0.5);
	if (t_rtt_==0) t_rtt_=1;
	if (t_srtt_ != 0) {
		register short delta;
		delta = t_rtt_ - (t_srtt_ >> T_SRTT_BITS);    
		if ((t_srtt_ += delta) <= 0)    
			t_srtt_ = 1;
		if (delta < 0)
			delta = -delta;
	  delta -= (t_rttvar_ >> T_RTTVAR_BITS);
	  if ((t_rttvar_ += delta) <= 0)  
			t_rttvar_ = 1;
	} else {
		t_srtt_ = t_rtt_ << T_SRTT_BITS;		
		t_rttvar_ = t_rtt_ << (T_RTTVAR_BITS-1);	
	}
	t_rtxcur_ = (((t_rttvar_ << (rttvar_exp_ + (T_SRTT_BITS - T_RTTVAR_BITS))) + t_srtt_)  >> T_SRTT_BITS ) * tcp_tick_;
	tzero_=t_rtxcur_;

	/* fine grained RTT estimate */

	if (rtt_ > 0) {
		rtt_ = df_*rtt_ + ((1-df_)*(now - tao));
	} else {
		rtt_ = now - tao;
	}

}

void TfrmAgent::recv(Packet *pkt, Handler *)
{
	double now = Scheduler::instance().clock();
	hdr_tfrmc *nck = hdr_tfrmc::access(pkt);
	double ts = nck->timestamp_echo ;
	double flost = nck->flost ; 
	int signal = nck->signal ; 

	update_rtt (ts, now) ;

	if ((rate_change_ == SLOW_START) && !(flost > 0)) {
		slowstart () ;
		Packet::free(pkt);
		return ;
	}
	else if ((rate_change_ == SLOW_START) && (flost > 0)) {
		rate_change_ = OUT_OF_SLOW_START ; 
	}
			
	double rcvrate = p_to_b(flost, rtt_, tzero_, size_, bval_);

	switch (version_) {
		case 0:
			switch (signal) {
				case INCREASE:		
					increase_rate (flost, now) ;
					break ; 
				case DECREASE:
					decrease_rate (flost, now) ;
					break ; 
				case NORMAL:
					break ; 
				default: 
					printf ("Wrong NACK signal %d\n", nck->signal); 
					abort() ; 
			}
			break ; 
		case 1: 
			if (rcvrate>(rate_+(size_/rtt_))) {
				increase_rate (flost, now) ;
			}
			else {
				decrease_rate (flost, now) ;		
			}
	}				
	Packet::free(pkt);
}

void TfrmAgent::slowstart () 
{
	double now = Scheduler::instance().clock(); 
	if (rate_ < size_/rtt_ ) {
		oldrate_ = rate_ ;
		rate_ = size_/rtt_ ;
		delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
		last_change_ = now ;
	}
	else {
		if (now - last_change_ > rtt_) {
			oldrate_ = rate_ ;
			rate_ = ssmult_*rate_ ; 
			delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
			last_change_ = now ; 
		}
		else {
			oldrate_ = rate_ ;
			delta_ = 0 ; 
		}
	}
	nextpkt() ;
}
void TfrmAgent::increase_rate (double p, double now) 
{
	double rcvrate = p_to_b(p, rtt_, tzero_, size_, bval_);
	rate_change_ = CONG_AVOID ;
	switch (slowincr_) {
		case 0:
			rate_+=size_/rtt_;
			last_change_=now;
			break ; 
		case 1: 
			if (now-last_change_ >= (k_-0.5)*rtt_) {
				last_change_=now;
				if (rate_>(size_/(rtt_*4)))
	  			rate_+=size_/rtt_;
				else 
	  			rate_+=rate_/4.0;
			}
			break ;
		case 2:
			if (rate_*(1.0+incrrate_)> rcvrate && version_ == 1)
	  		rate_ = rcvrate;
			else
	  		rate_*=1.0+incrrate_;
			last_change_=now;
			break ;
		case 3:
			/* the idea is to increase the rate to rate_*(1+incrrate_) 
			 * in k_*srate_*rtt round trip times.
			 * srate_ = rate_/size_ ;
			 */
			 oldrate_ = rate_ ;
			 rate_ = rate_ + incrrate_*size_/(k_*rtt_) ; 
			 if (rate_ > rcvrate)
				rate_ =  rcvrate ;
			delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
			last_change_=now;
			break ; 
		default: 
			printf ("error: invalid slowincr_ value %d\n", slowincr_);
			abort () ; 
	}
}

void TfrmAgent::decrease_rate (double p, double now) 
{
	double rcvrate = p_to_b(p, rtt_, tzero_, size_, bval_);
	rate_change_ = RATE_DECREASE ;
	switch (version_) {
		case 0: 
			rate_*=0.5;
			last_change_=now;
			break ; 
		case 1: 
			if (rcvrate<rate_) {
				rate_=rcvrate;
				last_change_=now;
			} 
			oldrate_ = rate_ ; 
			delta_ = 0 ;
			break ; 
		default: 
			printf ("error: invalid version_ value %d\n", version_);
			abort () ; 
	}
}

void TfrmAgent::sendpkt()
{
	Packet* p = allocpkt();
	hdr_tfrm *tfrmh = hdr_tfrm::access(p);

	tfrmh->seqno=seqno_++;
	tfrmh->timestamp=Scheduler::instance().clock();
	tfrmh->rtt=rtt_;
	tfrmh->tzero=tzero_;
	tfrmh->rate=rate_;
	tfrmh->psize=size_;
	tfrmh->version=version_;

	ndatapack_++;
	send(p, 0);
}

void TfrmSendTimer::expire(Event *e) {
  	a_->nextpkt();
}

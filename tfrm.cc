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


TfrmAgent::TfrmAgent() : Agent(PT_TFRM), send_timer_(this), 
                         NoFeedbacktimer_(this) 
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
	bind("SampleSizeMult_", &SampleSizeMult_);
	bind("bval_", &bval_);
	bind("overhead_", &overhead_);
	bind("ssmult_", &ssmult_);

}


int TfrmAgent::command(int argc, const char*const* argv)
{
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
	seqno_=0;				
	rate_ = InitRate_;
	delta_ = 0;
	oldrate_ = rate_;  
	rate_change_ = SLOW_START ;
	UrgentFlag = 1 ;
	rtt_=0;	 
	tzero_ = 0 ;
	last_change_=0;
	maxrate_ = 0; 
	ndatapack_=0 ;

	active_ = 1 ; 

	sendpkt();
	send_timer_.resched(size_/rate_);

	t_srtt_ = int(srtt_init_/tcp_tick_) << T_SRTT_BITS;
	t_rttvar_ = int(rttvar_init_/tcp_tick_) << T_RTTVAR_BITS;
	t_rtxcur_ = rtxcur_init_;
}

void TfrmAgent::stop()
{
	active_ = 0 ;
	send_timer_.force_cancel();
}

void TfrmAgent::nextpkt()
{
	double next = -1 ;
	double xrate = -1 ;

	/* cancel any pending send timer */

	sendpkt();
	
	/*during slow start and congestion avoidance, we increase rate*/
	/*slowly - by amount delta per packet */

	if ((rate_change_ == SLOW_START) || 
			((rate_change_ == CONG_AVOID) &&
			 (version_ == 1 && slowincr_ == 3))) {
		if (oldrate_ < rate_) {
			oldrate_ = oldrate_ + delta_ ;
		}
		xrate = oldrate_ ;
	}
	else {
		xrate = rate_ ;
	}
	if (xrate > SMALLFLOAT) {
		next = size_/xrate ; 
	}
	next = next + Random::uniform(overhead_);
	if (next > SMALLFLOAT ) {
		send_timer_.resched(next); 
	}
}

void TfrmAgent::update_rtt (double tao, double now) 
{

	/* the TCP update */

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
	double rate_since_last_report = nck->rate_since_last_report ;

	UrgentFlag = 0 ;

	/* compute the max rate as two times rcv rate */ 
	if (rate_since_last_report > 0) {
		maxrate_ = 2*rate_since_last_report*size_ ;
	}
	else {
		maxrate_ = 0 ; 
	}


	/* update the round trip time */

	update_rtt (ts, now) ;

	/* if we get no more feedback for 2 more rtts, cur rate in half */
	NoFeedbacktimer_.resched(2*rtt_);
	
	/* if we are in slow start and we just saw a loss */
	/* then come out of slow start */

	if ((rate_change_ == SLOW_START) && !(flost > 0)) {
		slowstart () ;
		Packet::free(pkt);
		return ;
	}
	else if ((rate_change_ == SLOW_START) && (flost > 0)) {
		rate_change_ = OUT_OF_SLOW_START ; 
		/*
		if (oldrate_ < 0.5*maxrate_) {
			rate_ = 0.5*maxrate_ ; delta_ = 0 ; 
		}
		else { 
			rate_ = 0.5*oldrate_ ; delta_ = 0 ;
		}
		Packet::free(pkt);
		nextpkt();
		return ;
		*/
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
	/*
	if (now - last_change_ < SMALLFLOAT) {
		nextpkt() ; 
	}
	*/
}

void TfrmAgent::slowstart () 
{
	double now = Scheduler::instance().clock(); 

	if (rate_ < size_/rtt_ ) {
		/*if this is the first report, change rate to 1 per rtt*/
		/*compute delta so rate increases slowly to new value */

		oldrate_ = rate_ ;
		rate_ = size_/rtt_ ;
		delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
		last_change_ = now ;
	} else {

		/*else multiply the rate by ssmult_, and compute delta, so that the*/
		/*rate increases slowly to new value */

		if (maxrate_ > 0) {
			if (ssmult_*rate_ < maxrate_ && now - last_change_ > rtt_) {
				rate_ = ssmult_*rate_ ; 
				delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
				last_change_=now;
			}
			else {
				if ( (oldrate_ > maxrate_) || (rate_ > maxrate_)) {
					delta_ = 0 ;
					rate_ = oldrate_ = maxrate_ ; 
	//				rate_ = oldrate_ = 0.5 * maxrate_ ;
	//  THINK ABOUT THIS!
					last_change_ = now ; 
				}
				else {
					if (now - last_change_ > rtt_) {
						rate_ = maxrate_ ;
						delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
						last_change_=now;
					}
				}
			}
		}
		else {
			rate_ = ssmult_*rate_ ; 
			delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
			last_change_=now;
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
			if (now-last_change_ >= (SampleSizeMult_-0.5)*rtt_) {
				last_change_=now;
				if (rate_>(size_/(rtt_*4)))
	  			rate_+=size_/rtt_;
				else 
	  			rate_+=rate_/4.0;
			}
			break ;
		case 2:
			if ( (rate_*(1.0+incrrate_)> rcvrate) && (version_ == 1))
	  		rate_ = rcvrate;
			else
	  		rate_*=1.0+incrrate_;
			last_change_=now;
			break ;
		case 3:

			/* the idea is to increase the rate to rate_*(1+incrrate_) 
			 * in SampleSizeMult_*srate_*rtt round trip times.
			 * srate_ = rate_/size_ ;
			 */
			if (maxrate_ > 0) {
				if (rate_*(1+incrrate_) < maxrate_ ) {
					rate_ = rate_*(1+incrrate_); 
					delta_ = (rate_ - oldrate_)/(SampleSizeMult_*rate_*rtt_/size_);
					last_change_=now;
				}
				else {
					if ((oldrate_ > maxrate_)||(rate_ > maxrate_))
						delta_ = 0 ;
					else {
						rate_ = maxrate_; 
						delta_ = (rate_ - oldrate_)/(SampleSizeMult_*rate_*rtt_/size_);
						last_change_=now;
					}
				}
			}
			else {
				rate_ = rate_*(1+incrrate_); 
				delta_ = (rate_ - oldrate_)/(SampleSizeMult_*rate_*rtt_/size_);
				last_change_=now;
			}
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

	if (active_) {
		Packet* p = allocpkt();
		hdr_tfrm *tfrmh = hdr_tfrm::access(p);
	
		tfrmh->seqno=seqno_++;
		tfrmh->timestamp=Scheduler::instance().clock();
		tfrmh->rtt=rtt_;
		tfrmh->tzero=tzero_;
		tfrmh->rate=rate_;
		tfrmh->psize=size_;
		tfrmh->version=version_;
		tfrmh->UrgentFlag=UrgentFlag;
	
		ndatapack_++;
	
		/*
		printf ("s %f %d\n",  tfrmh->timestamp, tfrmh->seqno);
		fflush(stdout); 
		*/
	
		send(p, 0);
	}
}

void TfrmAgent::reduce_rate_on_no_feedback()
{
	rate_change_ = RATE_DECREASE ; 
	rate_*=0.5;
	delta_ = 0 ;
	UrgentFlag = 1 ;
	NoFeedbacktimer_.resched(2*rtt_);
	nextpkt();
	/*
	double now = Scheduler::instance().clock();
	printf ("no feedback timer %f %f %f\n", now, rate_, rtt_);
	fflush(stdout);
	*/
}
void TfrmSendTimer::expire(Event *) {
  	a_->nextpkt();
}

void TfrmNoFeedbackTimer::expire(Event *) {
	a_->reduce_rate_on_no_feedback () ;
}

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
	incrrate_(0.1), send_timer_(this), df_(0.9), version_(0), slowincr_(0),
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
	rate_=InitRate_;  /* Starting rate */
	rtt_=0.0;         /* "accurate" rtt */ 
	seqno_=0;	/* Initial Sequence Number */
	run_=1;
	last_change_=0.0;
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
	if (run_==0) 
		return;
	sendpkt();
	if (rate_>0.0)
		send_timer_.resched((0.5+(Random::uniform()))*size_/rate_);
}

void TfrmAgent::recv(Packet *pkt, Handler *)
{
	double now = Scheduler::instance().clock();
	hdr_tfrmc *nck = hdr_tfrmc::access(pkt);

	/*if we didn't know RTT before, set rate to one packet per RTT*/
	if (rtt_==0.0) {
		rate_=size_/(now - nck->timestamp_echo);
	}
	
	//This code calculates RTO using TCP's course grain algorithm
	//code taken from TCP Tahoe in ns
	t_rtt_ = int((now-nck->timestamp_echo) /tcp_tick_ + 0.5);
	if (t_rtt_==0) t_rtt_=1;
	//
	// srtt has 3 bits to the right of the binary point
	// rttvar has 2
	//
	if (t_srtt_ != 0) {
	          register short delta;
	          delta = t_rtt_ - (t_srtt_ >> T_SRTT_BITS);    
		  // d = (m - a0)
	          if ((t_srtt_ += delta) <= 0)    
		  	// a1 = 7/8 a0 + 1/8 m
	                t_srtt_ = 1;
	          if (delta < 0)
	                delta = -delta;
	          delta -= (t_rttvar_ >> T_RTTVAR_BITS);
	          if ((t_rttvar_ += delta) <= 0)  
			// var1 = 3/4 var0 + 1/4 |d|
	               t_rttvar_ = 1;
	} else {
		t_srtt_ = t_rtt_ << T_SRTT_BITS;                
		// srtt = rtt
		t_rttvar_ = t_rtt_ << (T_RTTVAR_BITS-1);        
		// rttvar = rtt / 2
	}
	//
	// Current retransmit value is
	//    (unscaled) smoothed round trip estimate
	//    plus 2^rttvar_exp_ times (unscaled) rttvar.
	//
	t_rtxcur_ = (((t_rttvar_ << (rttvar_exp_ + (T_SRTT_BITS - T_RTTVAR_BITS))) + t_srtt_)  >> T_SRTT_BITS ) * tcp_tick_;
	tzero_=t_rtxcur_;

	//keep our own RTT accurately for use in the equation
	//the TCP coarse grain version is not useful here
	if (rtt_!=0.0) {
		rtt_ = df_*rtt_ + ((1-df_)*(now - nck->timestamp_echo));
	} else {
		rtt_ = now - nck->timestamp_echo;
	}

	if (version_==0) {
		if (nck->signal==INCREASE) {
			if (slowincr_==1) {
				if (now-last_change_ < (k_-0.5)*rtt_) {
	  				Packet::free(pkt);
	  				return;
				}
				if (rate_>(size_/rtt_*4))
	  				rate_+=size_/rtt_;
				else {
	  				rate_+=rate_/4.0;
				}
			} else if (slowincr_==0) {
				rate_+=size_/rtt_;
			} else if (slowincr_==2) {
				rate_*=1.0+incrrate_;
			}
			last_change_=now;
		} else if (nck->signal==DECREASE) {
			rate_*=0.5;
			last_change_=now;
		}
	} else {
		double rcvrate = p_to_b(nck->flost, rtt_, tzero_, size_);
		if (rcvrate>(rate_+(size_/rtt_))) {
			if (slowincr_==1) {
				if (now-last_change_ < (k_-0.5)*rtt_) {
	  				Packet::free(pkt);
	  				return;
				}
				if (rate_>(size_/rtt_*4))
	  				rate_+=size_/rtt_;
				else {
  					rate_+=rate_/4.0;
				}
			} else if (slowincr_==0) {
				rate_+=size_/rtt_;
			} else if (slowincr_==2) {
				if (rate_*1.0+incrrate_ > rcvrate)
	  				rate_ = rcvrate;
				else
	  				rate_*=1.0+incrrate_;
			}
			last_change_=now;
		} else if (rcvrate<rate_) {
			rate_=rcvrate;
			last_change_=now;
		} 
	}
	Packet::free(pkt);
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

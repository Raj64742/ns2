#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
 
#include "ip.h"
#include "agent.h"
#include "packet.h"
#include "timer-handler.h"
#include "random.h"

//#define FLOYDMATHIS
//#define DEBUG

#define T_SRTT_BITS 3
#define T_RTTVAR_BITS 2

FILE *debug = stderr;
#define B 1
#define MAXRATE 125000000.0

double 
p_to_b(double p, double rtt, double tzero, int psize) 
{
	//equation from Padhye, Firoiu, Towsley and Kurose, Sigcomm 98
	//ignoring the term for rwin limiting
	double tmp1, tmp2, res;
	res=rtt*sqrt(2*B*p/3);
	tmp1=3*sqrt(3*B*p/8);
	if (tmp1>1.0) tmp1=1.0;
	tmp2=tzero*p*(1+32*p*p);
	res+=tmp1*tmp2;
	if (res==0.0) {
		res=MAXRATE;
	} else {
		res=psize/res;
	}
#ifdef DEBUF
	fprintf(debug, "p:%f rtt:%f tzero:%f result:%f\n", p, rtt, tzero, res);
#endif
	return res;
}

double 
b_to_p(double b, double rtt, double tzero, int psize) 
{
	double p, pi, bres;
	int ctr=0;
	//would be much better to invert the p_to_b equation than to
	//solve this numerically...
#ifdef DEBUG
	fprintf(debug, "b_to_p: %f %f %f\n", b, rtt, tzero);
#endif
	p=0.5;pi=0.25;
	while(1) {
		bres=p_to_b(p,rtt,tzero,psize);
		//if we're within 5% of the correct value from below, this is OK
		//for this purpose.
		if ((bres>0.95*b)&&(bres<1.05*b)) return p;
		if (bres>b) {
			p+=pi;
		} else {
			p-=pi;
		}
		pi/=2.0;
		ctr++;
		if (ctr>30) {
			//fprintf(debug, "too many iterations: p:%e pi:%e b:%e bres:%e\n", p, pi, b, bres);
			return p;
		}
	}
}

/********************************Should be in .h****************************/

class TfrmAgent;

class TfrmSendTimer : public TimerHandler {
public:
  	TfrmSendTimer(TfrmAgent *a) : TimerHandler() { a_ = a; }
  	virtual void expire(Event *e);
protected:
  	TfrmAgent *a_;
};

/**************************** class defn ********************************/

class TfrmAgent : public Agent {
  	friend TfrmSendTimer;
public:
  	TfrmAgent();
  	void recv(Packet*, Handler*);
  	void sendpkt();
  	void nextpkt();
  	int command(int argc, const char*const* argv);
  	void trace(TracedVar* v);
  	void start();
  	void stop();
protected:
  	double rate_, rtt_, rttvar_, tzero_;

  	//for TCP tahoe RTO alg.
  	int t_srtt_, t_rtt_, t_rttvar_, rttvar_exp_;
  	double t_rtxcur_;
  	double tcp_tick_;
  	double incrrate_;

  	int seqno_, psize_;
  	TfrmSendTimer send_timer_;
  	int run_;
  	double df_; 			// decay factor for RTT EWMA
  	int version_;
  	int slowincr_;
  	int k_;
  	double last_change_;
  	TracedInt ndatapack_;		// number of packets sent
};

static class TfrmClass : public TclClass {
public:
  	TfrmClass() : TclClass("Agent/TFRM") {}
  	TclObject* create(int, const char*const*) {
    		return (new TfrmAgent());
  	}
} class_tfrm;

/********************************Send Timer****************************/

void TfrmSendTimer::expire(Event *e) {
  	a_->nextpkt();
}


/********************************Packet Types****************************/

struct hdr_tf {
  	int seqno; 		//data sequence number
  	double rate; 		//sender's current rate
  	double rtt;  		//RTT estimate of sender
  	double tzero; 		//RTO in Umass eqn
  	double timestamp;  	//time this message was sent
  	int psize;
  	int version;
} hdr_tf_s;

#define DECREASE 1
#define NORMAL 2
#define INCREASE 3
struct nack {
  	int seqno; // not sure yet
  	double timestamp; //time this nack was sent
  	double timestamp_echo; //timestamp from a data packet
  	double timestamp_offset; //offset since we received that data packet
  	double flost;
  	int signal;
} nack_s;

/************************** methods ********************************/


/********************************TFRM Source****************************/

TfrmAgent::TfrmAgent() : Agent(PT_TF), tcp_tick_(0.1), 
	incrrate_(0.1), send_timer_(this), df_(0.9), version_(0), slowincr_(0),
	ndatapack_(0)
{
#ifdef DEBUG
	fprintf(debug, "new Tfrm agent created\n");
#endif
	size_=1000;
	bind("packetSize_", &size_);
	bind("df_", &df_);
	bind("version_", &version_);
	bind("slowincr_", &slowincr_);
	bind("tcp_tick_", &tcp_tick_);
	bind("incrrate_", &incrrate_);
	bind("ndatapack_", &ndatapack_);
}

void
TfrmAgent::trace(TracedVar* v)
{
#ifdef DEBUG
	fprintf(debug, "TfrmAgent::trace\n");
#endif
}

int TfrmAgent::command(int argc, const char*const* argv)
{
#ifdef DEBUG
	fprintf(debug, "TfrmAgent::command: \n");
#endif
	//Tcl& tcl = Tcl::instance();
#ifdef DEBUG
	for(int i=0;i<argc;i++)
		fprintf(debug, "%s\n", argv[i]);
	fprintf(debug, "\n");
#endif
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
	rate_=1000.0; /*bytes/sec*/
	rtt_=0.0; /*ms*/
	rttvar_=0.0;
	psize_=1000;
	seqno_=0;
	run_=1;
	last_change_=0.0;
	k_=4;
	sendpkt();
	send_timer_.resched(psize_/rate_);

	rttvar_exp_=2;
	t_srtt_ = int(/*srtt_init_*/0 / tcp_tick_) << T_SRTT_BITS;
	t_rttvar_ = int(/*rttvar_init_*/12 / tcp_tick_) << T_RTTVAR_BITS;
	t_rtxcur_ = /*rtxcur_init_*/6.0;

}

void TfrmAgent::stop()
{
	run_=0;
	send_timer_.resched(0.0);
}

void TfrmAgent::nextpkt()
{
	if (run_==0) return;
	sendpkt();
	if (rate_>0.0)
		send_timer_.resched((0.5+(Random::uniform()))*psize_/rate_);
}

void TfrmAgent::recv(Packet *pkt, Handler *)
{
	double now = Scheduler::instance().clock();
#ifdef DEBUG 
 	fprintf(debug, "TfrmAgent::recv\n");
#endif
	struct nack *nck;
	nck = (struct nack*)pkt->accessdata();

	/*if we didn't know RTT before, set rate to one packet per RTT*/
#ifdef DEBUG
	fprintf(debug, "rtt: %f \n", rtt_);
#endif
	if (rtt_==0.0) {
		rate_=psize_/(now - nck->timestamp_echo);
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
	//the TCP course grain version is not useful here
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
				if (rate_>(psize_/rtt_*4))
	  				rate_+=psize_/rtt_;
				else {
	  				rate_+=rate_/4.0;
				}
			} else if (slowincr_==0) {
				rate_+=psize_/rtt_;
			} else if (slowincr_==2) {
				rate_*=1.0+incrrate_;
			}
			last_change_=now;
		} else if (nck->signal==DECREASE) {
			rate_*=0.5;
			last_change_=now;
		}
	} else {
#ifdef FLOYDMATHIS
		double rcvrate = (1.0 * psize_)/(rtt_ * sqrt(nck->flost));
#else
		double rcvrate = p_to_b(nck->flost, rtt_, tzero_, psize_);
#endif
		if (rcvrate>(rate_+(psize_/rtt_))) {
			if (slowincr_==1) {
				if (now-last_change_ < (k_-0.5)*rtt_) {
	  				Packet::free(pkt);
	  				return;
				}
				if (rate_>(psize_/rtt_*4))
	  				rate_+=psize_/rtt_;
				else {
  					rate_+=rate_/4.0;
				}
			} else if (slowincr_==0) {
				rate_+=psize_/rtt_;
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
#ifdef DEBUG
	fprintf(debug, "Rate now: %f\n", rate_);
#endif
	Packet::free(pkt);
}

void TfrmAgent::sendpkt()
{
	struct hdr_tf pld;
#ifdef DEBUG
	fprintf(debug, "TfrmAgent::send: seqno %d rate %f\n", seqno_, rate_);
#endif
	Packet* pkt = allocpkt();
	pkt->allocdata(sizeof(pld));
	pld.seqno=seqno_++;
	pld.timestamp=Scheduler::instance().clock();
	pld.rtt=rtt_;
	pld.tzero=tzero_;
	pld.rate=rate_;
	pld.psize=psize_;
	pld.version=version_;
	memcpy(pkt->accessdata(), &pld, sizeof(pld));
	ndatapack_++;
	send(pkt, 0);
}



/********************************TFRM Sink Classes************************/

class TfrmSinkAgent;

class TfrmNackTimer : public TimerHandler {
public:
	TfrmNackTimer(TfrmSinkAgent *a) : TimerHandler() { a_ = a; }
	virtual void expire(Event *e);
protected:
	TfrmSinkAgent *a_;
};

class TfrmSinkAgent : public Agent {
	friend TfrmNackTimer;
public:
	TfrmSinkAgent();
	void recv(Packet*, Handler*);
protected:
	void sendpkt();
	void nextpkt();
	void increase_pvec(int);

	double rate_;
	double rtt_, tzero_;
	double flost_;
	double last_timestamp_, last_arrival_, last_nack_;
	int *pvec_;
	double *tsvec_;
	int pveclen_;
	int pvecfirst_, pveclast_;
	int prevpkt_;
	int psize_;
	int k_;
	int version_;
	int received_;
	TfrmNackTimer nack_timer_;
};

static class TfrmSinkClass : public TclClass {
public:
	TfrmSinkClass() : TclClass("Agent/TFRMSink") {}
	TclObject* create(int, const char*const*) {
		 return (new TfrmSinkAgent());
	}
} class_tfrmSink;
 
/********************************Send Timer****************************/

void TfrmNackTimer::expire(Event *e) {
	a_->nextpkt();
}

/********************************TFRM Sink****************************/

TfrmSinkAgent::TfrmSinkAgent() : Agent(PT_TFC), rate_(0.0), 
	last_arrival_(0.0),
	last_nack_(0.0), 
	pvec_(NULL), tsvec_(NULL), received_(0), nack_timer_(this)
{
#ifdef DEBUG
	fprintf(debug, "new TfrmSink agent created\n");
#endif
	size_=40;
	k_=4;
	prevpkt_=-1;
}

void TfrmSinkAgent::recv(Packet *pkt, Handler *)
{
	struct hdr_tf *pld;
	received_++;
	pld = (struct hdr_tf*)pkt->accessdata();
#ifdef DEBUG
	fprintf(debug, "TfrmSinkAgent::recv: seq %d\n", pld->seqno);
#endif
	if (pvec_==NULL) {
		/*initially keep a 1000 packet history*/
		pvec_=(int *)malloc(sizeof(int)*1000);
		tsvec_=(double *)malloc(sizeof(double)*1000);
		pveclen_=1000;
		pvecfirst_=0; pveclast_=0;
		pvec_[0]=pld->seqno;
		tsvec_[0]=pld->timestamp;
	} else {
		pveclast_++;
		if (pveclast_==pveclen_) pveclast_=0;
		if (pvecfirst_==pveclast_) pvecfirst_++;
		if (pvecfirst_==pveclen_) pvecfirst_=0;
		pvec_[pveclast_] = pld->seqno;
		tsvec_[pveclast_] = pld->timestamp;
	}
#ifdef DEBUG
	if (pld->seqno-prevpkt_!=1) 
		fprintf(debug, "#####packet lost#####\n");
#endif
	prevpkt_=pld->seqno;
	double packets_sent=1+pvec_[pveclast_]-pvec_[pvecfirst_];
	double packets_received=1+pveclast_ - pvecfirst_;
	if (packets_received<=0) packets_received = pveclen_ - packets_received;
	double prevrtt=rtt_;
	rtt_=pld->rtt;
	tzero_=pld->tzero;
	psize_=pld->psize;
	version_=pld->version;
	last_arrival_=Scheduler::instance().clock();
	last_timestamp_=pld->timestamp;
#ifdef DEBUG
	fprintf(debug, "rcv: %f sent: %f\n", packets_received, packets_sent);
#endif
	if ((rate_==0.0)||(prevrtt==0.0))
		nextpkt();
	rate_=pld->rate;
	Packet::free(pkt);
}

void TfrmSinkAgent::nextpkt() {
	sendpkt();
	if (rtt_!=0.0) 
		nack_timer_.resched(rtt_);
}

void TfrmSinkAgent::sendpkt()
{
	struct nack nck;
	double now = Scheduler::instance().clock();

	//don't send an ACK unless we've received new data
	//if we're sending slower than one packet per RTT, don't need
	//multiple responses per data packet.
	if (last_arrival_ < last_nack_)
		return;
	last_nack_=now;
	
	Packet* pkt = allocpkt();
	pkt->allocdata(sizeof(nck));
	nck.seqno=pvec_[pveclast_];
	nck.timestamp_echo=last_timestamp_;
	nck.timestamp_offset=now-last_arrival_;
	nck.timestamp=now;

#ifdef FLOYDMATHIS
	double p=(1.0*psize_) / (rate_ * rtt_);
	p *= p;
#else
	double p;
	if (rate_!=0) {
		p = b_to_p(rate_, rtt_, tzero_, psize_);
	} else
		p=1;
#endif

	int sample = (int)(k_/p);
	if (sample>received_) sample=received_;
#ifdef DEBUG
	fprintf(debug, "p: %f sample: %d\n", p, sample);
#endif

	nck.signal = NORMAL;

	if (pveclen_<sample) {
		//we don't have a long enough pvec for this low loss rate
		//we'd like this never to happen!
		increase_pvec(sample);
	} else {
		int sent=0;
		int lost=0;
		int rcvd=0;
		int ix=pveclast_;
		int prev=pvec_[pveclast_]+1;
		double last_loss=1000000.0;
		while((sent<sample)||(lost<4)) {      
			sent+=prev-pvec_[ix];
			rcvd++;
			if ((prev-pvec_[ix])!=1) {
				//there was loss
				if ((last_loss - tsvec_[ix]) > rtt_) {
	  			//only count losses if they're 
				//separate congestion events
	  				lost++;
	  				last_loss = tsvec_[ix];
				}
			}
			prev=pvec_[ix];

			//have we any more history?
			if (ix==pvecfirst_) break;

			ix--;
			if (ix < 0) ix=pveclen_-1;
		}
		flost_=((float)lost)/((float)sent);
#ifdef DEBUG
		fprintf(debug, "+++ sample: %d sent: %d rcvd: %d lost: %d\n", sample, sent, rcvd, lost);
#endif
		if (version_==0) {
			if (flost_>p*1.5) {
				//we're nonconformant
#ifdef DEBUG
	fprintf(debug, "##nonconformant: sample: %d sent: %d rcvd: %d k: %d\n",
		sample, sent, rcvd, k_);	
#endif
				nck.signal = DECREASE;
			} else if (flost_<(p/2.0)) {
	//we're overconformant
#ifdef DEBUG
	fprintf(debug, "##overconformant: sample: %d sent: %d rcvd: %d k: %d\n",
		sample, sent, rcvd, k_);
#endif
				nck.signal = INCREASE;
			} else {
#ifdef DEBUG
	fprintf(debug, "##conformant: sample: %d sent: %d rcvd: %d k: %d\n",
		sample, sent, rcvd, k_);
#endif
			}
		}
	}
	nck.flost=flost_;
#ifdef DEBUG
	fprintf(debug, "TfrmSinkAgent::send tse:%f tso:%f flost:%f\n",
	 nck.timestamp_echo, nck.timestamp_offset, nck.flost);
#endif
	memcpy(pkt->accessdata(), &nck, sizeof(nck));
	send(pkt, 0);
}

void TfrmSinkAgent::increase_pvec(int size)
{
#ifdef DEBUG
	fprintf(debug, "WARNING: increasing pvec from %d to %d\n",
	 pveclen_, 2*size);
#endif
	if (size>1000000) {
		abort();
	}
	int *pvec2=(int *)malloc(sizeof(int)*2*size);
	double *tsvec2=(double *)malloc(sizeof(double)*2*size);
	int i,j;
	for(i=pvecfirst_,j=0;;i++,j++) {
		if (i==pveclen_) i=0;
		pvec2[j]=pvec_[i];
		tsvec2[j]=tsvec_[i];
		if (i==pveclast_) break;
	}
	free(pvec_);
	free(tsvec_);
	pvec_=pvec2;
	tsvec_=tsvec2;
	pveclen_=2*size;
	pvecfirst_=0;
	pveclast_=i;
}


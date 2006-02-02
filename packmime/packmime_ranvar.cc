/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/* 
 * Copyright 2002, Statistics Research, Bell Labs, Lucent Technologies and
 * The University of North Carolina at Chapel Hill
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution.
 *    3. The name of the author may not be used to endorse or promote 
 * products derived from this software without specific prior written 
 * permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Reference
 *     Stochastic Models for Generating Synthetic HTTP Source Traffic 
 *     J. Cao, W.S. Cleveland, Y. Gao, K. Jeffay, F.D. Smith, and M.C. Weigle 
 *     IEEE INFOCOM 2004.
 *
 * Documentation available at http://dirt.cs.unc.edu/packmime/
 * 
 * Contacts: Michele Weigle (mcweigle@cs.unc.edu),
 *           Kevin Jeffay (jeffay@cs.unc.edu)
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "packmime_ranvar.h" 

/* 
 * Defining static variables here to avoid errors with gcc 3+ compilers
 * Thanks to Brian Michael Carrig 
 */
const double PackMimeHTTPFlowArriveRandomVariable::shapeParam0;
const double PackMimeHTTPFlowArriveRandomVariable::shapeParam1;
const double PackMimeHTTPFlowArriveRandomVariable::shapeParam2;
const double PackMimeHTTPFlowArriveRandomVariable::p_persistent;
const double PackMimeHTTPFlowArriveRandomVariable::p_1page;
const double PackMimeHTTPFlowArriveRandomVariable::shape_npage;
const double PackMimeHTTPFlowArriveRandomVariable::scale_npage;
const double PackMimeHTTPFlowArriveRandomVariable::shape_ntransfer;
const double PackMimeHTTPFlowArriveRandomVariable::scale_ntransfer;
const double PackMimeHTTPFlowArriveRandomVariable::p_1transfer;
const double PackMimeHTTPFlowArriveRandomVariable::m_loc_w;
const double PackMimeHTTPFlowArriveRandomVariable::v_loc_w;
const double PackMimeHTTPFlowArriveRandomVariable::shape_scale2_w;
const double PackMimeHTTPFlowArriveRandomVariable::rate_scale2_w;
const double PackMimeHTTPFlowArriveRandomVariable::v_error_w;
const double PackMimeHTTPFlowArriveRandomVariable::m_loc_b;
const double PackMimeHTTPFlowArriveRandomVariable::v_loc_b;
const double PackMimeHTTPFlowArriveRandomVariable::shape_scale2_b;
const double PackMimeHTTPFlowArriveRandomVariable::rate_scale2_b;
const double PackMimeHTTPFlowArriveRandomVariable::v_error_b;

const double PackMimeHTTPFileSizeRandomVariable::fsize1_prob_a;
const double PackMimeHTTPFileSizeRandomVariable::fsize1_d; 
const double PackMimeHTTPFileSizeRandomVariable::fsize1_varRatio_intercept;
const double PackMimeHTTPFileSizeRandomVariable::fsize1_varRatio_slope;

const double PackMimeHTTPFileSizeRandomVariable::weibullScaleCacheRun;
const double PackMimeHTTPFileSizeRandomVariable::weibullShapeCacheRun_Asymptoe;
const double PackMimeHTTPFileSizeRandomVariable::weibullShapeCacheRun_Para1;
const double PackMimeHTTPFileSizeRandomVariable::weibullShapeCacheRun_Para2;
const double PackMimeHTTPFileSizeRandomVariable::weibullShapeCacheRun_Para3;
const double PackMimeHTTPFileSizeRandomVariable::weibullScaleDownloadRun;
const double PackMimeHTTPFileSizeRandomVariable::weibullShapeDownloadRun; 

const int PackMimeHTTPFileSizeRandomVariable::fsize0_cache_cutoff; 
const int PackMimeHTTPFileSizeRandomVariable::fsize0_stretch_thres;
const double PackMimeHTTPFileSizeRandomVariable::m_fsize0_notcache;
const double PackMimeHTTPFileSizeRandomVariable::v_fsize0_notcache; 

const double PackMimeHTTPFileSizeRandomVariable::m_loc;
const double PackMimeHTTPFileSizeRandomVariable::v_loc;
const double PackMimeHTTPFileSizeRandomVariable::shape_scale2;
const double PackMimeHTTPFileSizeRandomVariable::rate_scale2;
const double PackMimeHTTPFileSizeRandomVariable::v_error;


/*:::::::::::::::::::::::::::::::: FX  :::::::::::::::::::::::::::::::::::::*/

FX::FX (const double *x, const double *y, int n) :
  x_((double*) x), y_((double*)y), nsteps_(n)
{
	assert (n>=2);
	slope_ = new double[n];
	for (int i = 0; i<n-1; i++) 
		slope_[i] = (y[i+1] - y[i]) / (x[i+1] - x[i]);
	slope_[n-1] = slope_[n-2];
}

FX::~FX()
{
	delete[] slope_;
}

double FX::LinearInterpolate (double xnew)
{
	int i;

	// we should use binary search if nsteps is large, but for now...
	for (i=0; i<nsteps_-2; i++) {
		if (xnew < x_[i+1]) {
			return (y_[i] + slope_[i]*(xnew-x_[i]));
		}	
	}

	return (y_[nsteps_-1] + slope_[nsteps_-1]*(xnew-x_[nsteps_-1]));
}

/*:::::::::::::::::::::::::::: Fractional ARIMA  ::::::::::::::::::::::::::::*/

/** Reference:
 * "Modeling Persistence in Hydrological Time Series 
 * Using Fractional Differencing" 
 * J.R.M. Hosking
 * Water Resources Research, Vol 20, No 12, P1898-1908, Dec 1984
 */

/**
 * y[t] = sum_j=1^p (AR[j] * y[t-j]) + x[t] - sum_j=1^q (MA[j] * x[t-j]
 */

#define Min(a, b)      ((a) < (b) ? (a) : (b))

FARIMA::FARIMA(RNG* rng, double d, int N, int pAR, int qMA) :
  rng_(rng), N_(N), pAR_(pAR), qMA_(qMA), d_(d)
{
	assert(N && qMA);

	if (pAR) {
		AR_ = new double[pAR];
		y_ = new double [pAR];
	}

	if (qMA)
		MA_ = new double[qMA];
	x_ = new double [N];
	phi_ = new double [N];

	/** use 1-theta[1] B-theta[2] B^2 - ..., and 
	 * our model is epsilon_j + epsilon_(j-1) 
	 */
	MA_[0] = -1; 

	for (int i=0; i<pAR; i++) 
		y_[i] = 0.0;

	t_ = tmod_ = 0;

	/* skip the first few in simulation to stablize */
	while (t_ < N + pAR + qMA)
		NextLow();
}

FARIMA::~FARIMA()
{
	if (pAR_ > 0) { 
		delete[] AR_; 
		delete[] y_; 
	} 
	if (qMA_ > 0)
		delete[] MA_; 
	delete[] phi_;
	delete[] x_;
}

double FARIMA::NextLow()
{
	int j;
	double mt, xt, yt, a, b, v0;

	if (t_ == 0) {
		/* d_0.25; gamma(1-2*d)/(gamma(1-d))^2 => 1.180341 */
		v0 = rng_->gammln(1.0-2.0*d_) - 2.0*rng_->gammln(1.0-d_);
		v0 = exp(v0);
		/* phi[0] is not used, so we use it for vt */
		phi_[0] = v0; 
		x_[t_] = rng_->rnorm() * pow(v0, 0.5);
	} else if (t_ < N_) {
		/* get phi_tt */
		phi_[t_] = d_/(t_-d_);
		/* get phi_tj */
		for (j=1; j<t_; j++) {
			if (j <= t_-j) {
				a = phi_[j] - phi_[t_] * phi_[t_-j];
				b = phi_[t_-j] - phi_[t_] * phi_[j];
				phi_[j] = a;
				phi_[t_-j] = b;
			}
		}
		/* get vt (phi[0]) */
		/* v[t] = (1-phi_tt^2) * v[t-1] */
		phi_[0] = (1-phi_[t_]*phi_[t_]) * phi_[0]; 
	}

	/* get m_t, v_t */
	mt = 0;
	for (j=1; j<=Min(t_,N_-1); j++)
		mt += phi_[j] * x_[(t_-j)%N_];

	/* get xt */
	xt = rng_->rnorm() * pow(phi_[0], 0.5) + mt;
	x_[t_%N_] = xt;

	/** add AR, MA parts only after enough points in the error 
	 * fARIMA process 
	 */
	yt = xt;
	if (t_ > qMA_) {
		if (qMA_ > 0) {
			for(j=0; j<qMA_; j++) 
				yt -= x_[(t_-1-j)%N_] * MA_[j];
		}
	}

	t_++;
	tmod_ = (++tmod_ == N_) ? 0 : tmod_;

	return (yt);
}

// assumes t > N and qMA < N
double FARIMA::Next()
{
	double mt, xt, yt;
	int j, m;

	/* get m_t, v_t */
	mt = 0;
	for (j=1; j<=tmod_; j++)
		mt += phi_[j] * x_[tmod_-j];

	for (j=tmod_+1; j<=N_-1; j++)
		mt += phi_[j] * x_[N_+tmod_-j];

	/* get xt */
	xt = rng_->rnorm() * pow(phi_[0], 0.5) + mt;
	x_[tmod_] = xt;

	/** add AR, MA parts only after enough points in the error 
	 * fARIMA process 
	 */
	yt = xt;
	if (t_>qMA_) {
		if (qMA_>0) {
			m = (tmod_ < qMA_) ? tmod_ : qMA_;
			for(j=0; j<m; j++) 
				yt -= x_[tmod_-1-j] * MA_[j];
			for(j=m; j<qMA_; j++) 
				yt -= x_[N_+tmod_-1-j] * MA_[j];
		}
	}

	t_++;
	tmod_ = (++tmod_ == N_) ? 0 : tmod_;

	return (yt);
}

/*:::::::::::::::::::::: PackMimeHTTP Transmission Delay RanVar :::::::::::::::::*/

static class PackMimeHTTPXmitRandomVariableClass : public TclClass {
public:
	PackMimeHTTPXmitRandomVariableClass() : 
	  TclClass("RandomVariable/PackMimeHTTPXmit"){}
	TclObject* create(int argc, const char*const* argv) {
		if (argc == 6) {
		  // rate, type
		  return(new PackMimeHTTPXmitRandomVariable ((double) 
							 atof(argv[4]), 
							 atoi(argv[5])));
		} else if (argc == 7) {
		  // rate, type, RNG
		  RNG* rng = (RNG*)TclObject::lookup(argv[6]);
		  return(new PackMimeHTTPXmitRandomVariable ((double) 
							       atof(argv[4]), 
							       atoi(argv[5]),
							       rng));
		} else {
			return(new PackMimeHTTPXmitRandomVariable());
		}
	 }
} class_packmimexmitranvar;

PackMimeHTTPXmitRandomVariable::PackMimeHTTPXmitRandomVariable() : 
	fARIMA_(NULL), myrng_(rng_)
{
	bind ("rate_", &rate_);
	bind ("type_", &type_);	
	bind ("mean_", &mean_);
	bind ("const_", &const_);
}

PackMimeHTTPXmitRandomVariable::PackMimeHTTPXmitRandomVariable(double rate, int type) :
	rate_(rate), type_(type), mean_(-1), const_(-1), fARIMA_(NULL), myrng_(rng_)
{	
}

PackMimeHTTPXmitRandomVariable::PackMimeHTTPXmitRandomVariable(double rate, int type, RNG* rng) :
  	rate_(rate), type_(type), mean_(-1), const_(-1), myrng_(rng), fARIMA_(NULL)
{
}

PackMimeHTTPXmitRandomVariable::~PackMimeHTTPXmitRandomVariable() {
	if (fARIMA_)
		delete fARIMA_;
}

void PackMimeHTTPXmitRandomVariable::initialize() 
{
	double sigmaTotal;
	struct arima_params * arima = & rtt_arima_params[type_];
	double d = arima->d;
  
	assert(arima->N >= arima->qMA);
	varRatio_ = myrng_->logitinv(arima->varRatioParam0 + 
				     arima->varRatioParam1 * log(rate_) /LOG2);
	sigmaTotal = 1.0;
	sigmaNoise_ = sigmaTotal * pow(varRatio_, 0.5);
	sigmaEpsilon_ = sigmaTotal * 
		pow(exp(2.0*myrng_->gammln(1.0-d) - myrng_->gammln(1.0-2.0*d)) 
		    / (2+2*d/(1-d)) * (1-varRatio_), 0.5);

	fARIMA_ = new FARIMA(myrng_, arima->d, arima->N);
}

/* generate RTT according to the marginal distribution */
double PackMimeHTTPXmitRandomVariable::value(void) {
	double yt, p;
	    
	if (fARIMA_ == NULL)
		initialize();

	yt = fARIMA_->Next();
	yt = yt * sigmaEpsilon_ + myrng_->rnorm() * sigmaNoise_;
	p = myrng_->pnorm(yt);
	p = -log(1-p)/log(10.0);
	yt = rtt_invcdf[type_].LinearInterpolate(p);
	yt = pow(yt, weibull_shape[type_]);

	// split in half for DelayBox -M. Weigle 9/17/01
	if (mean_ < 0)
		return(yt/2);
	else {
		if (const_ <0)
			const_ = mean_/avg();
		return(yt/2*const_);
	}
} 

double PackMimeHTTPXmitRandomVariable::avg(void) {
	if (type_)
		return 0.027;
	else
		return 0.058;
}

const double PackMimeHTTPXmitRandomVariable::short_rtt_invcdf_x[] = { 
    0.00, 0.25, 0.50, 0.75, 1.00, 1.25, 1.50, 
    1.75, 2.00, 2.25, 2.50, 2.75, 3.00, 3.25, 
    3.50, 3.75, 4.00, 4.25, 4.50, 4.75, 5.00, 
    5.30103
  };

const double PackMimeHTTPXmitRandomVariable::short_rtt_invcdf_y[] = {
    0.87741, 1.07293, 1.10812, 1.86701, 2.04735, 2.30645, 2.90111, 
    3.30185, 3.59297, 3.87927, 4.02239, 4.17551, 4.32119, 4.43112, 
    4.57079, 4.66087, 4.73359, 4.78683, 4.84591, 4.91282, 4.95503, 
    5.01453
};

const double PackMimeHTTPXmitRandomVariable::long_rtt_invcdf_x[]={ 
    0.00, 0.25, 0.50, 0.75, 1.00, 1.25, 1.50,  
    1.75, 2.00, 2.25, 2.50, 2.75, 3.00, 4.00
};
const double PackMimeHTTPXmitRandomVariable::long_rtt_invcdf_y[]={
    0.79105, 3.71848, 4.46097, 4.49838, 4.84954, 5.78215, 6.77357, 
    7.78683, 8.75032, 10.3434, 11.4477, 14.0272, 14.6139, 17.2251
};

struct arima_params PackMimeHTTPXmitRandomVariable::rtt_arima_params[] = {
    { 0.31,     /* d */
      1000,     /* number of finite AR rep of farima */
      -0.445,   /* varRatioParam0 */
      0.554,    /* varRatioParam1 */
      0, 1      /* pAR=0, qMA */
    },
    { 0.32,    /* d */
      1000,    /* number of finite AR rep of farima */
      -0.053,  /* varRatioParam0 */
      0.396,   /* varRatioParam1 */
      0, 1     /* pAR=0, qMA */
    }
};

FX PackMimeHTTPXmitRandomVariable::rtt_invcdf[2] = {
	FX (short_rtt_invcdf_x, short_rtt_invcdf_y, 
	    sizeof (short_rtt_invcdf_x) / sizeof(double)),
	FX (long_rtt_invcdf_x, long_rtt_invcdf_y, 
	    sizeof (long_rtt_invcdf_x) / sizeof(double))
};

const double PackMimeHTTPXmitRandomVariable::weibull_shape[2] = { 5, 3 };

/*:::::::::::::::::::::: PackMimeHTTP Flow Arrival RanVar ::::::::::::::::::::::*/

/**
 * This random variable includes both flow interarrivals for non-persistent
 * connections and persistent connections.  This RV is sampled during
 * connection setup.  It will return an array of interarrivals.  The first
 * element in the array wil be the number of requests in this connection.  If
 * the number of elements is 1, then the connection is non-persistent.  If the
 * number of elements is > 1, then the connection is persistent.  
 */

static class PackMimeHTTPFlowArriveRandomVariableClass : public TclClass {
public:
	PackMimeHTTPFlowArriveRandomVariableClass() : 
	  TclClass("RandomVariable/PackMimeHTTPFlowArrive"){}
	TclObject* create(int argc, const char*const* argv) {
		if (argc == 5) {
		  // rate
		  return(new PackMimeHTTPFlowArriveRandomVariable 
			 ((double) atof(argv[4])));
		} else if (argc == 6) {
		  // rate, RNG
		  RNG* rng = (RNG*)TclObject::lookup(argv[5]);
		  return(new PackMimeHTTPFlowArriveRandomVariable ((double) 
							       atof(argv[4]), 
							       rng));
		} else {
		  return(new PackMimeHTTPFlowArriveRandomVariable());
		}
	 }
} class_packmimeflowarriveranvar;


PackMimeHTTPFlowArriveRandomVariable::PackMimeHTTPFlowArriveRandomVariable() : 
  fARIMA_(NULL), myrng_(rng_)
{
	bind ("rate_", &rate_);
}

PackMimeHTTPFlowArriveRandomVariable::PackMimeHTTPFlowArriveRandomVariable 
(double rate) :
  rate_(rate), fARIMA_(NULL), myrng_(rng_)
{
}

PackMimeHTTPFlowArriveRandomVariable::PackMimeHTTPFlowArriveRandomVariable 
(double rate, RNG* rng) :
  rate_(rate), fARIMA_(NULL), myrng_(rng)
{
}

PackMimeHTTPFlowArriveRandomVariable::~PackMimeHTTPFlowArriveRandomVariable() 
{
	if (fARIMA_)
		delete fARIMA_;
}

void PackMimeHTTPFlowArriveRandomVariable::initialize() {
	double sigmaTotal;
	struct arima_params *arima = &flowarrive_arima_params;
	double d = arima->d;
	int N = arima->N;
	// New model Added by Y. Gao June 2003
	d = 0.33;
	
	varRatio_ = myrng_->logitinv(0.333+0.414*log(rate_)/LOG2); 
	sigmaTotal = 1.0;
	sigmaNoise_ = sigmaTotal * pow(varRatio_,0.5);
	sigmaEpsilon_ = sigmaTotal * pow (exp (2.0 * myrng_->gammln(1.0-d) - myrng_->gammln(1.0-2.0*d)) / 
		     (2+2*d/(1-d)) * (1-varRatio_), 0.5);
	weibullShape_ = myrng_->logitinv(0.352+0.388*log(rate_)/LOG2);
	weibullScale_ = 1.0 / (rate_ * exp(myrng_->gammln(1+1.0/weibullShape_)));
	fARIMA_ = new FARIMA(myrng_, d, N);
}

double PackMimeHTTPFlowArriveRandomVariable::avg(void) {
	return 4.3018;
}

double PackMimeHTTPFlowArriveRandomVariable::avg(int gap_type_) {
	if (gap_type_ == 0) {
		return 0.0262;
	}
	else {
		return 3.32;
	}
}

/* generate flow rate according to the marginal distribution */
double PackMimeHTTPFlowArriveRandomVariable::value(void) {
	double yt;

	if (fARIMA_ == NULL) 		
		initialize();

	yt = fARIMA_->Next();
	yt = yt * sigmaEpsilon_ + myrng_->rnorm() * sigmaNoise_;
	yt = myrng_->qweibull(myrng_->pnorm(yt), weibullShape_, weibullScale_);

	return(yt);
}

double* PackMimeHTTPFlowArriveRandomVariable::value_array(void) 
/** 
 * value_array() samples to see if this connection should be
 * persistent or non-persistent.  It also samples to find the number
 * of requests for this connection.
 */
{
	int reqs = 1;        // total number of requests
	int pages = 1;       // default is 1 page per connection

	// is the connection persistent?
	if (myrng_->rbernoulli (p_persistent) == 0) {
		// non-persistent, so no gap needed
		double* np_array = new double[reqs+1];
		np_array[0] = 1;
		np_array[1] = 0;
		return (double*) np_array;
	}

	// persistent connection

	// how many pages to request?
	if (myrng_->rbernoulli (p_1page) == 0) {
		// multiple pages per connection
		pages = (int) ceil (myrng_->rweibull (shape_npage, 
						      scale_npage)) + 1;
		if (pages == 1)
			pages++;
	} 
	int* objs_per_page = new int[pages];

	// objs_per_page will hold an array of objects (requests) per page
	reqs = Template (pages, objs_per_page);

	// create array for inter-request times
	double* array = new double[reqs+1];
	array[0] = (double) reqs;
	// generate inter-request times
	int req = 1;		
	double loc_w = myrng_->rnorm()*sqrt(v_loc_w)+m_loc_w;
	double scale2_w = myrng_->rgamma(shape_scale2_w, 1/rate_scale2_w);
	double loc_b = myrng_->rnorm()*sqrt(v_loc_b)+m_loc_b;
	double scale2_b = myrng_->rgamma(shape_scale2_b, 1/rate_scale2_b);

	for (int page = 0; page < pages; page++) {
		// for each page		
		for (int obj = 0; obj < objs_per_page[page]; obj++) {    
			// for each object in the page
			if (page == 0 && obj == 0) {
				// first request
				array[req] = 0;
			} else {
				if (page != 0 && obj == 0) {
					// main page (between page requests)
					array[req] = pow(2.0, loc_b + sqrt(scale2_b) * 
							 myrng_->rnorm()*sqrt(v_error_b));
				} else {
					// embedded objects (within page requests)
					array[req] = pow(2.0, loc_w + sqrt(scale2_w) * 
							 myrng_->rnorm()*sqrt(v_error_w));
				}
			}
			req++;
		}
	}
	delete []objs_per_page;

	return (double*) array;
}

int PackMimeHTTPFlowArriveRandomVariable::Template(int pages, int *objs_per_page) {
	/**
	 * Template() returns an array with the number of files per page
	 * 
	 * pages - number of pages to request
	 * objs_per_page - array of number of files per page
	 * reqs - total number of requests
	 */

	int z, i;
	int reqs = 0; 

	for (i=0; i<pages; i++){
		if (pages == 1) {
			// only one page in this connection
			z = 0; 
		} else {
			// multiple pages in this connection
			z = myrng_->rbernoulli(p_1transfer);
		}
		if (z == 0) {
			// multiple files in this page
			// masked by Y. Gao
      			objs_per_page[i] = (int) ceil (myrng_->rweibull (shape_ntransfer, scale_ntransfer)) + 1;
			if (objs_per_page[i] == 1)
				objs_per_page[i]++;
		} else {
			// only one file in this page
			objs_per_page[i] = 1;
		}
		reqs += objs_per_page[i];
	}
	return reqs;
}

void PackMimeHTTPFlowArriveRandomVariable::setnpage (double npage)
{
	/*	if (npage <= 1) {
		return;
	}
	scale_npage = -1 / log(1 - 1/(npage - 1));
	*/
}

void PackMimeHTTPFlowArriveRandomVariable::setntrans (double ntrans)
{
	/*	if (ntrans <= 1) {
		return;
	}
	scale_ntransfer = -1 / log(1 - 1/(ntrans - 1));
	*/
}


struct arima_params PackMimeHTTPFlowArriveRandomVariable::flowarrive_arima_params =
{
    0.35,       /* d */
    1000,       /* number of finite AR rep of farima */
    -1.2811,    /* varRatioParam0 */
    -0.3150,    /* varRatioParam1 */
    0, 1        /* pAR=0, qMA */
};


/*:::::::::::::::::::::: PackMimeHTTP File Size RanVar :::::::::::::::::::::::::*/


static class PackMimeHTTPFileSizeRandomVariableClass : public TclClass {
public:
	PackMimeHTTPFileSizeRandomVariableClass() : 
	  TclClass("RandomVariable/PackMimeHTTPFileSize"){}
	TclObject* create(int argc, const char*const* argv) {
		if (argc == 6) {
		  // rate, type
		  return(new PackMimeHTTPFileSizeRandomVariable 
			 ((double) atof(argv[4]), atoi(argv[5])));
		} else if (argc == 7) {
		  // rate, type, RNG
		  RNG* rng = (RNG*)TclObject::lookup(argv[6]);
		  return(new PackMimeHTTPFileSizeRandomVariable ((double) 
							       atof(argv[4]), 
							       atoi(argv[5]),
							       rng));
		} else {
			return(new PackMimeHTTPFileSizeRandomVariable());
		}
	 }
} class_packmimefilesizeranvar;


PackMimeHTTPFileSizeRandomVariable::PackMimeHTTPFileSizeRandomVariable() : 
  fARIMA_(NULL), myrng_(rng_)
{
	bind ("rate_", &rate_);
	bind ("type_", &type_);
}

PackMimeHTTPFileSizeRandomVariable::PackMimeHTTPFileSizeRandomVariable(double rate, int type) :
  rate_(rate), type_(type), fARIMA_(NULL), myrng_(rng_)
{
}

PackMimeHTTPFileSizeRandomVariable::PackMimeHTTPFileSizeRandomVariable(double rate, int type, RNG* rng) :
  rate_(rate), type_(type), fARIMA_(NULL), myrng_(rng)
{	
}


PackMimeHTTPFileSizeRandomVariable::~PackMimeHTTPFileSizeRandomVariable() 
{
	if (fARIMA_)
		delete fARIMA_;
}

void PackMimeHTTPFileSizeRandomVariable::initialize() 
{
	double sigmaTotal;
	struct arima_params *arima = &filesize_arima_params;
	double d = arima->d;
	int N = arima->N;
 
	/* fsize0 run length */
	state_ = 0;
	runlen_ = 0;

	/* fsize1 */
	sigmaTotal = 1.0;
	varRatio_ = myrng_->logitinv(fsize1_varRatio_intercept+ fsize1_varRatio_slope * log(rate_)/LOG2);
	//	fprintf(stderr, "D %f Inter %f Slope %f Filesize varRatio %f Rate %f\n", d, fsize1_varRatio_intercept, fsize1_varRatio_slope, varRatio_, rate_);

	sigmaNoise_ = sigmaTotal * pow(varRatio_,0.5);
	sigmaEpsilon_ = sigmaTotal * 
		pow(exp (2.0 * myrng_->gammln(1.0-d) - myrng_->gammln(1.0-2.0*d)) / 
		    (2+2*d/(1-d)) * (1-varRatio_), 0.5);

	/* fsize0 runlen */
	shape_[0] = myrng_->logitinv(0.718+0.357*log(rate_)/LOG2);
	shape_[1] = myrng_->logitinv(1.293+0.316*log(rate_)/LOG2);

	scale_[0] = 0.775;
	scale_[1] = 3.11;
	
	/* simulate */
	fARIMA_ = new FARIMA(myrng_, d, N);
}

/** random generate the marginal distribution of fsize0 depending
 *  on whether it is a cache validation (0) or downloaded file (1).
 */
int PackMimeHTTPFileSizeRandomVariable::rfsize0(int state)
{
	double yt, p;
  
	p = myrng_->uniform();
	if (state==1) 
		p = log(1-p)/LOG2; 
	yt = fsize_invcdf[0][state].LinearInterpolate(p);
	if (state==1) 
		yt = exp(LOG2*yt); 

	return((int) yt);
}

/* given a probability, find the corresponding quantile of fsize1. */
int PackMimeHTTPFileSizeRandomVariable::qfsize1(double p)
{
	int state;
	double yt;

	/* first convert this to conditional probablity */
	if(p > fsize1_prob_a){
		state = 1; 
		p = (p-fsize1_prob_a)/(1-fsize1_prob_a);
	}else {
		state = 0;
		p = p/fsize1_prob_a;
	}

	if (state==1) 
		p=log(1-p)/LOG2;
	yt = fsize_invcdf[1][state].LinearInterpolate(p);
	if (state==1) 
		yt=exp(LOG2*yt); 

	return((int) yt);
}

double PackMimeHTTPFileSizeRandomVariable::avg(void) {
	if (type_ == PACKMIME_RSP_SIZE) {
		return 2272;
	}
	else {
		return 423;
	}
}

/* obtain the next server file size */
double PackMimeHTTPFileSizeRandomVariable::value() 
{
	int yt, restart;

	if (fARIMA_ == NULL)
		initialize();

	if (type_ == PACKMIME_RSP_SIZE) {
		/* regenerate the runlen if a run is finished */
		if (runlen_ <= 0) { // XXX Y. G. changed to <= from ==
			state_ = 1-state_;
			runlen_ = (int) ceil(myrng_->rweibull(shape_[state_], scale_[state_]));
			restart = 1;
		} else 
			restart = 0;

		yt=rfsize0(state_); 
		runlen_--;
		
		return (double) (yt);
	} else if (type_ == PACKMIME_REQ_SIZE) {		
		double yx;
		yx = fARIMA_->Next();
		yx = yx * sigmaEpsilon_ + myrng_->rnorm() * sigmaNoise_;

		yt = qfsize1(myrng_->pnorm(yx));
		return (double) (yt);
	}
  	return (0);
}

double* PackMimeHTTPFileSizeRandomVariable::value_array(int reqs) 
{
	
	double* array = new double[reqs+1];
	array[0] = (double) reqs;
	double filesz = value();

	if (type_ == PACKMIME_RSP_SIZE) {
		int state;
		double loc, scale2, interrand, interres;		

		for(int i=1; i<=reqs; i++) {
		        array[i] = filesz;
		}
		if (filesz <= fsize0_cache_cutoff) {
			state = 0; 
		} else {
			state = 1;
		}
		
		if (state == 0) {
			for(int i=1; i<=reqs; i++) {
				array[i] = filesz;
			}
		}  else {
			if (reqs>=2) {
				loc = myrng_->rnorm() * sqrt(v_loc)+m_loc;
				scale2 = myrng_->rgamma(shape_scale2, 1/rate_scale2);			
				for (int i=2; i<=reqs; i++) { 
					interrand = myrng_->rnorm();
					interres = loc + sqrt(scale2) * interrand * sqrt(v_error);	 
					array[i] = pow(2.0, interres);
				}
			}
			array[1] = filesz;
		}
		
		return (double*) array;
	} else if (type_ == PACKMIME_REQ_SIZE) {
		for (int i=1; i<=reqs; i++) {
			array[i] = filesz;
		}
		return (double*) array;
	}
	return 0;
}

/* fitted inverse cdf curves for file sizes  */
const double PackMimeHTTPFileSizeRandomVariable::fsize0_invcdf_a_x[]={ 
    1e-04, 0.0102, 0.0203, 0.0304, 0.0405, 0.0506, 0.0607, 0.0708, 
    0.0809, 0.091, 0.1011, 0.1112, 0.1213, 0.1314, 0.1415, 0.1516, 
    0.1617, 0.1718, 0.1819, 0.192, 0.2021, 0.2122, 0.2223, 0.2324, 
    0.2425, 0.2526, 0.2627, 0.2728, 0.2829, 0.293, 0.3031, 0.3132, 
    0.3233, 0.3334, 0.3435, 0.3536, 0.3637, 0.3738, 0.3839, 0.394, 
    0.4041, 0.4142, 0.4243, 0.4344, 0.4445, 0.4546, 0.4647, 0.4748, 
    0.4849, 0.495, 0.5051, 0.5152, 0.5253, 0.5354, 0.5455, 0.5556, 
    0.5657, 0.5758, 0.5859, 0.596, 0.6061, 0.6162, 0.6263, 0.6364, 
    0.64649, 0.65659, 0.66669, 0.67679, 0.68689, 0.69699, 0.70709, 
    0.71719, 0.72729, 0.73739, 0.74749, 0.75759, 0.76769, 0.77779, 
    0.78789, 0.79799, 0.80809, 0.81819, 0.82829, 0.83839, 0.84849, 
    0.85859, 0.86869, 0.87879, 0.88889, 0.89899, 0.90909, 0.91919, 
    0.92929, 0.93939, 0.94949, 0.95959, 0.96969, 0.97979, 0.98989, 
    0.99999
}; 

const double PackMimeHTTPFileSizeRandomVariable::fsize0_invcdf_a_y[]={ 
    4.645, 22.7357, 40.8041, 53.0108, 61.468, 64.4307, 67.3933, 69.6925, 
    71.3981, 73.1037, 74.8092, 80.6677, 87.4875, 93.5693, 97.6514, 
    101.249, 104.58, 107.021, 109.195, 111.087, 112.752, 114.273, 
    115.545, 116.78, 118.014, 119.248, 120.483, 121.551, 122.51, 123.441, 
    124.312, 125.119, 125.927, 126.734, 127.524, 128.288, 129.053, 
    129.818, 130.582, 131.347, 132.143, 132.973, 133.835, 134.71, 
    135.603, 136.537, 137.47, 138.404, 139.337, 140.28, 141.233, 142.186, 
    143.139, 144.092, 145.045, 145.979, 146.904, 147.816, 148.71, 
    149.589, 150.485, 151.478, 152.703, 153.928, 155.403, 157.294, 
    159.389, 161.953, 165.018, 168.54, 172.7, 177.629, 183.365, 189.766, 
    195.845, 200.636, 203.987, 206.43, 208.611, 211.114, 214.424, 
    218.008, 221.925, 225.722, 229.242, 232.747, 236.235, 239.952, 
    244.076, 248.68, 253.434, 257.857, 261.539, 264.471, 266.742, 
    268.487, 269.793, 270.794, 271.571, 272.234
}; 

const double PackMimeHTTPFileSizeRandomVariable::fsize0_invcdf_b_x[]={ 
    -18.1793, -17.9957, -17.8121, -17.6285, -17.4449, -17.2613, -17.0777, 
    -16.8941, -16.7104, -16.5268, -16.3432, -16.1596, -15.976, -15.7924, 
    -15.6088, -15.4252, -15.2415, -15.0579, -14.8743, -14.6907, -14.5071, 
    -14.3235, -14.1399, -13.9563, -13.7726, -13.589, -13.4054, -13.2218, 
    -13.0382, -12.8546, -12.671, -12.4874, -12.3038, -12.1201, -11.9365, 
    -11.7529, -11.5693, -11.3857, -11.2021, -11.0185, -10.8349, -10.6513, 
    -10.4677, -10.284, -10.1004, -9.91682, -9.7332, -9.54959, -9.36598, 
    -9.18237, -8.99876, -8.81515, -8.63154, -8.44792, -8.26431, -8.0807, 
    -7.89709, -7.71348, -7.52987, -7.34626, -7.16265, -6.97903, -6.79542, 
    -6.61181, -6.4282, -6.24459, -6.06098, -5.87737, -5.69376, -5.51014, 
    -5.32653, -5.14292, -4.95931, -4.7757, -4.59209, -4.40848, -4.22486, 
    -4.04125, -3.85764, -3.67403, -3.49042, -3.30681, -3.1232, -2.93959, 
    -2.75597, -2.57236, -2.38875, -2.20514, -2.02153, -1.83792, -1.65431, 
    -1.47069, -1.28708, -1.10347, -0.91986, -0.73625, -0.55264, -0.36903, 
    -0.18542, -0.0018
}; 

const double PackMimeHTTPFileSizeRandomVariable::fsize0_invcdf_b_y[]={ 
    25.8765, 25.7274, 25.5782, 25.4291, 25.2799, 25.1308, 24.9817, 
    24.8325, 24.6834, 24.5342, 24.3851, 24.2359, 24.0868, 23.9377, 
    23.7885, 23.6394, 23.4902, 23.3411, 23.1919, 23.0428, 22.8937, 
    22.7445, 22.5954, 22.4462, 22.2971, 22.1479, 21.9988, 21.8497, 
    21.7005, 21.5514, 21.4022, 21.2531, 21.104, 20.9548, 20.8057, 
    20.6565, 20.5074, 20.3582, 20.2091, 20.06, 19.9108, 19.7617, 19.6125, 
    19.4634, 19.3143, 19.1651, 19.016, 18.8668, 18.7177, 18.5685, 
    18.4194, 18.2703, 18.1211, 17.972, 17.8228, 17.6737, 17.5245, 
    17.3754, 17.2263, 17.0771, 16.928, 16.7788, 16.6297, 16.4806, 
    16.3314, 16.1823, 16.0331, 15.8842, 15.739, 15.6026, 15.4804, 
    15.3778, 15.3003, 15.2465, 15.2045, 15.1597, 15.0978, 15.0044, 
    14.8675, 14.6932, 14.4968, 14.2932, 14.0976, 13.9248, 13.7783, 
    13.6435, 13.5042, 13.3444, 13.1478, 12.9005, 12.6008, 12.2512, 
    11.8545, 11.4133, 10.9303, 10.4104, 9.86075, 9.28886, 8.70213, 
    8.10797
}; 

const double PackMimeHTTPFileSizeRandomVariable::fsize1_invcdf_a_x[]={ 
    3e-05, 0.01013, 0.02023, 0.03033, 0.04043, 0.05053, 0.06063, 0.07074, 
    0.08084, 0.09094, 0.10104, 0.11114, 0.12124, 0.13134, 0.14144, 
    0.15154, 0.16164, 0.17174, 0.18184, 0.19194, 0.20204, 0.21214, 
    0.22225, 0.23235, 0.24245, 0.25255, 0.26265, 0.27275, 0.28285, 
    0.29295, 0.30305, 0.31315, 0.32325, 0.33335, 0.34345, 0.35355, 
    0.36365, 0.37376, 0.38386, 0.39396, 0.40406, 0.41416, 0.42426, 
    0.43436, 0.44446, 0.45456, 0.46466, 0.47476, 0.48486, 0.49496, 
    0.50506, 0.51516, 0.52526, 0.53537, 0.54547, 0.55557, 0.56567, 
    0.57577, 0.58587, 0.59597, 0.60607, 0.61617, 0.62627, 0.63637, 
    0.64647, 0.65657, 0.66667, 0.67677, 0.68688, 0.69698, 0.70708, 
    0.71718, 0.72728, 0.73738, 0.74748, 0.75758, 0.76768, 0.77778, 
    0.78788, 0.79798, 0.80808, 0.81818, 0.82828, 0.83839, 0.84849, 
    0.85859, 0.86869, 0.87879, 0.88889, 0.89899, 0.90909, 0.91919, 
    0.92929, 0.93939, 0.94949, 0.95959, 0.96969, 0.97979, 0.98989, 1
}; 

const double PackMimeHTTPFileSizeRandomVariable::fsize1_invcdf_a_y[]={ 
    50.9777, 93.8217, 123.301, 146.594, 168.813, 184.263, 193.95, 
    200.037, 204.696, 210.069, 217.623, 226.668, 236.226, 245.269, 
    253.057, 259.624, 265.16, 269.811, 273.754, 277.165, 280.193, 
    283.062, 285.893, 288.79, 291.744, 294.738, 297.754, 300.776, 
    303.784, 306.763, 309.698, 312.564, 315.349, 318.042, 320.624, 
    323.107, 325.49, 327.782, 329.987, 332.11, 334.159, 336.135, 338.047, 
    339.898, 341.695, 343.443, 345.145, 346.808, 348.439, 350.04, 
    351.619, 353.177, 354.716, 356.239, 357.744, 359.233, 360.709, 
    362.17, 363.619, 365.056, 366.483, 367.9, 369.31, 370.711, 372.106, 
    373.496, 374.881, 376.263, 377.643, 379.021, 380.399, 381.778, 
    383.159, 384.542, 385.929, 387.321, 388.719, 390.121, 391.528, 
    392.939, 394.355, 395.775, 397.199, 398.626, 400.057, 401.491, 
    402.928, 404.368, 405.81, 407.255, 408.702, 410.151, 411.602, 
    413.054, 414.508, 415.963, 417.418, 418.874, 420.331, 421.788
}; 

const double PackMimeHTTPFileSizeRandomVariable::fsize1_invcdf_b_x[]={ 
    -17.7791, -17.5996, -17.4201, -17.2406, -17.0611, -16.8816, -16.7021, 
    -16.5226, -16.3431, -16.1635, -15.984, -15.8045, -15.625, -15.4455, 
    -15.266, -15.0865, -14.907, -14.7275, -14.548, -14.3685, -14.189, 
    -14.0095, -13.83, -13.6505, -13.471, -13.2915, -13.112, -12.9325, 
    -12.753, -12.5735, -12.394, -12.2145, -12.035, -11.8555, -11.676, 
    -11.4965, -11.317, -11.1375, -10.958, -10.7785, -10.599, -10.4194, 
    -10.24, -10.0604, -9.88094, -9.70144, -9.52193, -9.34243, -9.16293, 
    -8.98342, -8.80392, -8.62442, -8.44491, -8.26541, -8.08591, -7.9064, 
    -7.7269, -7.5474, -7.3679, -7.18839, -7.00889, -6.82939, -6.64988, 
    -6.47038, -6.29088, -6.11137, -5.93187, -5.75237, -5.57286, -5.39336, 
    -5.21386, -5.03435, -4.85485, -4.67535, -4.49584, -4.31634, -4.13684, 
    -3.95733, -3.77783, -3.59833, -3.41883, -3.23932, -3.05982, -2.88032, 
    -2.70081, -2.52131, -2.34181, -2.1623, -1.9828, -1.8033, -1.62379, 
    -1.44429, -1.26479, -1.08528, -0.90578, -0.72628, -0.54678, -0.36727, 
    -0.18777, -0.00827
}; 

const double PackMimeHTTPFileSizeRandomVariable::fsize1_invcdf_b_y[]={ 
    19.7578, 19.6369, 19.5159, 19.3949, 19.274, 19.153, 19.032, 18.9111, 
    18.7901, 18.6691, 18.5482, 18.4272, 18.3063, 18.1853, 18.0643, 
    17.9434, 17.8224, 17.7014, 17.5805, 17.4595, 17.3386, 17.2176, 
    17.0966, 16.9757, 16.8547, 16.7337, 16.6128, 16.4918, 16.3708, 
    16.2499, 16.1289, 16.0079, 15.887, 15.766, 15.6451, 15.5241, 15.4031, 
    15.2822, 15.1612, 15.0402, 14.9193, 14.7983, 14.6774, 14.5564, 
    14.4354, 14.3145, 14.1935, 14.0725, 13.9516, 13.8306, 13.7096, 
    13.5887, 13.4677, 13.3468, 13.2258, 13.1048, 12.9839, 12.8629, 
    12.7419, 12.621, 12.5, 12.3791, 12.2581, 12.1371, 12.0162, 11.8952, 
    11.7742, 11.6533, 11.5323, 11.4113, 11.2904, 11.1694, 11.0485, 
    10.9275, 10.8065, 10.6856, 10.5646, 10.4436, 10.3239, 10.2092, 
    10.1034, 10.0105, 9.93459, 9.87896, 9.8393, 9.80562, 9.76884, 
    9.71919, 9.64712, 9.54737, 9.42776, 9.29852, 9.16992, 9.05219, 
    8.95525, 8.88155, 8.82669, 8.78593, 8.75459, 8.72795
};

const double PackMimeHTTPFileSizeRandomVariable::fsize0_para[] = {
    weibullShapeCacheRun_Asymptoe,
    weibullShapeCacheRun_Para1,
    weibullShapeCacheRun_Para2,
    weibullShapeCacheRun_Para3
};

const double* PackMimeHTTPFileSizeRandomVariable::p = fsize0_para;

struct arima_params PackMimeHTTPFileSizeRandomVariable::filesize_arima_params  = {
    fsize1_d,   /* d */
    5000,       /* number of finite AR rep of farima */
    0,    /* varRatioParam0 */
    0,    /* varRatioParam1 */
    0, 1        /* pAR=0, qMA */
};

FX PackMimeHTTPFileSizeRandomVariable::fsize_invcdf[2][2] = {
    { FX(fsize0_invcdf_a_x, fsize0_invcdf_a_y, 
	 sizeof(fsize0_invcdf_a_x)/sizeof(double)), 
      FX(fsize0_invcdf_b_x, fsize0_invcdf_b_y,
	 sizeof(fsize0_invcdf_b_x)/sizeof(double))
    },
    { FX(fsize1_invcdf_a_x, fsize1_invcdf_a_y,
	 sizeof(fsize1_invcdf_a_x)/sizeof(double)),
      FX(fsize1_invcdf_b_x, fsize1_invcdf_b_y,
	 sizeof(fsize1_invcdf_b_x)/sizeof(double))
    }
};

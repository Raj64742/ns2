/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tools/rng.h,v 1.12 1998/09/21 22:56:56 polly Exp $ (LBL)";
 */

/* new random number generator */


#ifndef _rng_h_
#define _rng_h_


// Define rng_test to build the test harness.
#define rng_test

#include <math.h>
#include <stdlib.h>			// for atoi
#include <tclcl.h>

/*
 * RNGImplementation is internal---do not use it, use RNG.
 */
class RNGImplementation {
public:
	RNGImplementation(long seed = 1L) {
		seed_ = seed;
	};
	void set_seed(long seed) { seed_ = seed; }
	long seed() { return seed_; }
	long next();			// return the next one
	double next_double();	 
private:
	long seed_;
};

/*
 * Use class RNG in real programs.
 */
class RNG : public TclObject {

public:
	enum RNGSources { RAW_SEED_SOURCE, PREDEF_SEED_SOURCE, HEURISTIC_SEED_SOURCE };

	RNG() : stream_(1L) {};
	RNG(RNGSources source, int seed = 1) { set_seed(source, seed); };

	void set_seed(RNGSources source, int seed = 1);
	int seed() { return stream_.seed(); }
	static RNG* defaultrng() { return (default_); }

	int command(int argc, const char*const* argv);

	// These are primitive but maybe useful.
	inline int uniform_positive_int() {  // range [0, MAXINT]
		return (int)(stream_.next());
	}
	inline double uniform_double() { // range [0.0, 1.0)
		return stream_.next_double();
	}

	// these are for backwards compatibility
 	// don't use them in new code
	inline int random() { return uniform_positive_int(); }
	inline double uniform() {return uniform_double();}

	// these are probably what you want to use
	inline int uniform(int k) 
		{ return (uniform_positive_int() % (unsigned)k); }
	inline double uniform(double r) 
		{ return (r * uniform());}
	inline double uniform(double a, double b)
		{ return (a + uniform(b - a)); }
	inline double exponential()
		{ return (-log(uniform())); }
	inline double exponential(double r)
		{ return (r * exponential());}
	inline double pareto(double scale, double shape)
		{ return (scale * (1.0/pow(uniform(), 1.0/shape)));}
        inline double paretoII(double scale, double shape)
                { return (scale * ((1.0/pow(uniform(), 1.0/shape)) - 1));}

protected:   // need to be public?
	RNGImplementation stream_;
	static RNG* default_;
};

/*
 * Create an instance of this class to test RNGImplementation.
 * Do .verbose() for even more.
 */
#ifdef rng_test
class RNGTest {
public:
	RNGTest();
	void verbose();
	void first_n(RNG::RNGSources source, long seed, int n);
};
#endif /* rng_test */

#endif /* _rng_h_ */


/*
 * Copyright (c) 1995 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tools/random.h,v 1.8 1997/09/13 00:22:28 breslau Exp $ (LBL)
 */

#ifndef ns_random_h
#define ns_random_h

#include <math.h>
#include "config.h"

#define USE_RNG   /* run new rng by default as of  8-Sep-97 */

#ifdef USE_RNG

/* new rng version under developement */


#include "rng.h"

class Random {
private:
	static RNG rng_;
	static RNG &rng() { return rng_; }

public:
	static void seed(int s) { rng().set_seed(RNG::RAW_SEED_SOURCE, s); }
	static int seed_heuristically() { rng().set_seed(RNG::HEURISTIC_SEED_SOURCE); return rng().seed(); };

	static int random() { return rng().uniform_positive_int(); }
	static double uniform() { return rng().uniform_double();}
	static double uniform(double r) { return rng().uniform(r); }
	static double uniform(double a, double b) { return rng().uniform(a,b); }
	static double exponential() { return rng().exponential(); }
	static int integer(int k) { return rng().uniform(k); }
        static double exponential(double r) { return rng().exponential(r); }
	static double pareto(double scale, double shape) { return rng().pareto(scale, shape); }
};

#else

class Random {
public:
	static void seed(int);
	static int seed_heuristically();
	static inline int random() {
#if defined(__svr4__) || defined(__SVR4)
		return (::lrand48() & 0x7fffffff);
#else
		return (::random());
#endif
	}
	static inline double uniform() {
		/* random returns numbers in the range [0,2^31-1] */
#if defined(__svr4__) || defined(__SVR4)
		return (drand48());
#else
		return ((double)random() / 0x7fffffff);
#endif
	}
	static inline double uniform(double r) {
		return (r * uniform());
	}
	static inline double uniform(double a, double b) {
		return (a + uniform(b - a));
	}
	static inline double exponential() {
		return (-log(uniform()));
	}
	static inline int integer(int k) {
		return (random() % (unsigned)k);
	}
        static inline double exponential(double r) {
                return (r * exponential());
        }
	/* pareto distribution with specified shape and average
	 * equal to scale * (shape/(shape-1))
	 */
	static inline double pareto(double scale, double shape) {
	        return (scale * (1.0/pow(uniform(), 1.0/shape)));
	}
};

#endif

#endif

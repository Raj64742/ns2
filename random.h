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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/random.h,v 1.2 1997/01/23 00:02:15 gnguyen Exp $ (LBL)
 */

#ifndef ns_random_h
#define ns_random_h

#include <math.h>

#if defined(sun) && !(defined(__svr4__) || defined(__SVR4))
extern "C" long random();
#else
#include <stdlib.h>
#endif

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
};

#endif

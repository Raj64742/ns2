
/*
 * Copyright (c) 1997 Regents of the University of California.
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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/rng.cc,v 1.6 1997/12/19 22:20:12 bajaj Exp $ (LBL)";
#endif

/* new random number generator */

#ifndef WIN32
#include <sys/time.h>   // for gettimeofday
#include <unistd.h>
#endif

#include <stdio.h>
#include "rng.h"

#if defined(sun)
extern "C" {			// XXX Why not include config.h?
	int atoi(...);
	//	int gettimeofday(struct timeval*, struct timezone*);
	int gettimeofday(...); //fix to work on solaris as well
	   }
#endif

/*
 * RNGImplementation
 */

/*
 * Generate a periodic sequence of pseudo-random numbers with
 * a period of 2^31 - 2.  The generator is the "minimal standard"
 * multiplicative linear congruential generator of Park, S.K. and
 * Miller, K.W., "Random Number Generators: Good Ones are Hard to Find,"
 * CACM 31:10, Oct. 88, pp. 1192-1201.
 *
 * The algorithm implemented is:  Sn = (a*s) mod m.
 *   The modulus m can be approximately factored as: m = a*q + r,
 *   where q = m div a and r = m mod a.
 *
 * Then Sn = g(s) + m*d(s)
 *   where g(s) = a(s mod q) - r(s div q)
 *   and d(s) = (s div q) - ((a*s) div m)
 *
 * Observations:
 *   - d(s) is either 0 or 1.
 *   - both terms of g(s) are in 0, 1, 2, . . ., m - 1.
 *   - |g(s)| <= m - 1.
 *   - if g(s) > 0, d(s) = 0, else d(s) = 1.
 *   - s mod q = s - k*q, where k = s div q.
 *
 * Thus Sn = a(s - k*q) - r*k,
 *   if (Sn <= 0), then Sn += m.
 *
 * To test an implementation for A = 16807, M = 2^31-1, you should
 *   get the following sequences for the given starting seeds:
 *
 *   s0, s1,    s2,        s3,          . . . , s10000,     . . . , s551246 
 *    1, 16807, 282475249, 1622650073,  . . . , 1043618065, . . . , 1003 
 *    1973272912, 1207871363, 531082850, 967423018
 *
 * It is important to check for s10000 and s551246 with s0=1, to guard 
 * against overflow.
*/
/*
 * The sparc assembly code [no longer here] is based on Carta, D.G., "Two Fast
 * Implementations of the 'Minimal Standard' Random Number
 * Generator," CACM 33:1, Jan. 90, pp. 87-88.
 *
 * ASSUME that "the product of two [signed 32-bit] integers (a, sn)
 *        will occupy two [32-bit] registers (p, q)."
 * Thus: a*s = (2^31)p + q
 *
 * From the observation that: x = y mod z is but
 *   x = z * the fraction part of (y/z),
 * Let: sn = m * Frac(as/m)
 *
 * For m = 2^31 - 1,
 *   sn = (2^31 - 1) * Frac[as/(2^31 -1)]
 *      = (2^31 - 1) * Frac[as(2^-31 + 2^-2(31) + 2^-3(31) + . . .)]
 *      = (2^31 - 1) * Frac{[(2^31)p + q] [2^-31 + 2^-2(31) + 2^-3(31) + . . 
.]}
 *      = (2^31 - 1) * Frac[p+(p+q)2^-31+(p+q)2^-2(31)+(p+q)3^(-31)+ . . .]
 *
 * if p+q < 2^31:
 *   sn = (2^31 - 1) * Frac[p + a fraction + a fraction + a fraction + . . .]
 *      = (2^31 - 1) * [(p+q)2^-31 + (p+q)2^-2(31) + (p+q)3^(-31) + . . .]
 *      = p + q
 *
 * otherwise:
 *   sn = (2^31 - 1) * Frac[p + 1.frac . . .]
 *      = (2^31 - 1) * (-1 + 1.frac . . .)
 *      = (2^31 - 1) * [-1 + (p+q)2^-31 + (p+q)2^-2(31) + (p+q)3^(-31) + . . .]
 *      = p + q - 2^31 + 1
 */
 

#define A       16807L          /* multiplier, 7**5 */
#define M       2147483647L     /* modulus, 2**31 - 1; both used in random */
#define INVERSE_M ((double)4.656612875e-10)  /* (1.0/(double)M) */

long
RNGImplementation::next()
{
        long L, H;
	L = A * (seed_ & 0xffff);
	H = A * (seed_ >> 16);
	
	seed_ = ((H & 0x7fff) << 16) + L;
	seed_ -= 0x7fffffff;
	seed_ += H >> 15;
	
	if (seed_ <= 0) {
	        seed_ += 0x7fffffff;
	}
	return(seed_);
}

double
RNGImplementation::next_double()
{
	long i = next();
	return i * INVERSE_M;
}



/*
 * RNG implements a nice front-end around RNGImplementation
 */
static class RNGClass : public TclClass {
public:
        RNGClass() : TclClass("RNG") {}
	TclObject* create(int, const char*const*) {
	        return(new RNG());
	}
} class_rng;

/* default RNG */

RNG* RNG::default_ = NULL;

int
RNG::command(int argc, const char*const* argv)
{
        Tcl& tcl = Tcl::instance();

        if (argc == 3) {
		if (strcmp(argv[1], "testint") == 0) {
			int n = atoi(argv[2]);
			tcl.resultf("%d", uniform(n));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "testdouble") == 0) {
			double d = atof(argv[2]);
			tcl.resultf("%6e", uniform(d));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "seed") == 0) {
		        int s = atoi(argv[2]);
			if (s)
			        set_seed(RAW_SEED_SOURCE, s);
			else 
			        set_seed(HEURISTIC_SEED_SOURCE, 0);
			return(TCL_OK);
		}
	}
	if (argc == 2) {
	        if (strcmp(argv[1], "next-random") == 0) {
		        tcl.resultf("%u", uniform_positive_int());
			return(TCL_OK);
		}
		if (strcmp(argv[1], "seed") == 0) {
		        tcl.resultf("%u", stream_.seed());
			return(TCL_OK);
		}
		if (strcmp(argv[1], "default") == 0) {
		        default_ = this;
			return(TCL_OK);
		}
#if 0
	        if (strcmp(argv[1], "test") == 0) {
		        if (test())
			        tcl.resultf("RNG test failed");
			else
			        tcl.resultf("RNG test passed");
			return(TCL_OK);
		}
#endif
	}
	return(TclObject::command(argc, argv));
}

void
RNG::set_seed(RNGSources source, int seed = 1)
{
        /* The following predefined seeds are evenly spaced around
	 * the 2^31 cycle.  Each is approximately 33,000,000 elements
	 * apart.
	 */
#define N_SEEDS_ 64
        static long predef_seeds[N_SEEDS_] = {  
		1973272912L,  188312339L, 1072664641L,  694388766L,
		2009044369L,  934100682L, 1972392646L, 1936856304L,
		1598189534L, 1822174485L, 1871883252L,  558746720L,
		605846893L, 1384311643L, 2081634991L, 1644999263L,
		773370613L,  358485174L, 1996632795L, 1000004583L,
		1769370802L, 1895218768L,  186872697L, 1859168769L,
		349544396L, 1996610406L,  222735214L, 1334983095L,
		144443207L,  720236707L,  762772169L,  437720306L,
		939612284L,  425414105L, 1998078925L,  981631283L,
		1024155645L,  822780843L,  701857417L,  960703545L,
		2101442385L, 2125204119L, 2041095833L,   89865291L,
		898723423L, 1859531344L,  764283187L, 1349341884L,
		678622600L,  778794064L, 1319566104L, 1277478588L,
		538474442L,  683102175L,  999157082L,  985046914L,
		722594620L, 1695858027L, 1700738670L, 1995749838L,
		1147024708L,  346983590L,  565528207L,  513791680L
	};
	static long heuristic_sequence = 0;

	switch (source) {
	case RAW_SEED_SOURCE:
		// use it as it is
		break;
	case PREDEF_SEED_SOURCE:
		if (seed < 0 || seed >= N_SEEDS_)
			abort();
		seed = predef_seeds[seed];
		break;
	case HEURISTIC_SEED_SOURCE:
		timeval tv;
		gettimeofday(&tv, 0);
		heuristic_sequence++;   // Always make sure we're different than last time.
		seed = (tv.tv_sec ^ tv.tv_usec ^ (heuristic_sequence << 8)) & 0x7fffffff;
		break;
	};
	// set it
	// NEEDSWORK: should we throw out known bad seeds?
	// (are there any?)
	if (seed < 0)
		seed = -seed;
	stream_.set_seed(seed);

	// Toss away the first few values of heuristic seed.
	// In practice this makes sequential heuristic seeds
	// generate different first values.
	//
	// How many values to throw away should be the subject
	// of careful analysis.  Until then, I just throw away
	// ``a bunch''.  --johnh
	if (source == HEURISTIC_SEED_SOURCE) {
		int i;
		for (i = 0; i < 128; i++) {
			stream_.next();
		};
	};
}



/*
 * RNGTest:
 * Make sure the RNG makes known values.
 * Optionally, print out some stuff.
 *
 * Simple test program:
 * #include "rng.h"
 * void main() { RNGTest test; test.verbose(); }
 */

#ifdef rng_test
RNGTest::RNGTest()
{
	RNG rng(RNG::RAW_SEED_SOURCE, 1L);
	int i;
	long r;

	for (i = 0; i < 10000; i++) 
		r = rng.uniform_positive_int();

	if (r != 1043618065L)
		abort();

	for (i = 10000; i < 551246; i++)
		r = rng.uniform_positive_int();

	if (r != 1003L)
		abort();
}

void
RNGTest::first_n(RNG::RNGSources source, long seed, int n)
{
	RNG rng(source, seed);

	for (int i = 0; i < n; i++) {
		int r = rng.uniform_positive_int();
		printf("%10d ", r);
	};
	printf("\n");
}

void
RNGTest::verbose()
{
	printf ("default: ");
	first_n(RNG::RAW_SEED_SOURCE, 1L, 5);

	int i;
	for (i = 0; i < 64; i++) {
		printf ("predef source %2d: ", i);
		first_n(RNG::PREDEF_SEED_SOURCE, i, 5);
	};

	printf("heuristic seeds should be different from each other and on each run.\n");
	for (i = 0; i < 64; i++) {
		printf ("heuristic source %2d: ", i);
		first_n(RNG::HEURISTIC_SEED_SOURCE, i, 5);
	};
}
#endif /* rng_test */


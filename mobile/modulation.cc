#include <math.h>
#include <stdlib.h>

#include <debug.h>
#include <modulation.h>

/* ======================================================================
   Binary Phase Shift Keying
   ====================================================================== */
BPSK::BPSK()
{
	Rs = 0;
}

BPSK::BPSK(int S)
{
	Rs = S;
}

int
BPSK::BitError(double Pr)
{
	double Pe;			// probability of error
	double x;
	int nbit = 0;			// number of bit errors tolerated

	if(nbit == 0) {
		Pe = ProbBitError(Pr);
	}
	else {
		Pe = ProbBitError(Pr, nbit);
	}

	// quick check
	if(Pe == 0.0)
		return 0;		// no bit errors

	// scale the error probabilty
	Pe *= 1e3;

	x = random() % 1000;

	if(x < Pe)
		return 1;		// bit error
	else
		return 0;		// no bit errors
}

double
BPSK::ProbBitError(double)
{
	double Pe = 0.0;

	return Pe;
}

double
BPSK::ProbBitError(double, int)
{
	double Pe = 0.0;

	return Pe;
}



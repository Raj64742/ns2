#ifndef __modulation_h__
#define __modulation_h__

/* ======================================================================
   Modulation Schemes

	Using the receive power and information about the modulation
	scheme, amount of forward error correction, etc., this class
	computes whether or not a packet was received correctly or with
	few enough errors that they can be tolerated.

   ====================================================================== */

class Modulation {

	friend class NetIf;

public:
	virtual int BitError(double) = 0;	// success reception?

protected:
	int Rs;					// symbol rate per second

private:
	// Probability of 1 bit error
	virtual double ProbBitError(double) = 0;

	// Probability of n bit errors
	virtual double ProbBitError(double, int) = 0;
};


class BPSK : public Modulation {

public:
	BPSK(void);
	BPSK(int);

	virtual int BitError(double Pr);

private:
	virtual double ProbBitError(double Pr);
	virtual double ProbBitError(double Pr, int n);
};

#endif /* __modulation_h__ */


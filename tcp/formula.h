#define MAXRATE 25000000.0 
#define SAMLLFLOAT 0.0000001

static double p_to_b(double p, double rtt, double tzero, int psize, int bval) 
{
	double tmp1, tmp2, res;

	if (p < 0 || rtt < 0) {
		return MAXRATE ; 
	}
	res=rtt*sqrt(2*bval*p/3);
	tmp1=3*sqrt(3*bval*p/8);
	if (tmp1>1.0) tmp1=1.0;
	tmp2=tzero*p*(1+32*p*p);
	res+=tmp1*tmp2;
	if (res < SAMLLFLOAT) {
		res=MAXRATE;
	} else {
		res=psize/res;
	}
	if (res > MAXRATE) {
		res = MAXRATE ; 
	}
	return res;
}

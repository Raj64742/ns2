#define MAXRATE 125000000.0 

static double p_to_b(double p, double rtt, double tzero, int psize, int bval) 
{
	/*
	 * equation from Padhye, Firoiu, Towsley and Kurose, Sigcomm 98
	 * ignoring the term for rwin limiting
	 */

	double tmp1, tmp2, res;
	res=rtt*sqrt(2*bval*p/3);
	tmp1=3*sqrt(3*bval*p/8);
	if (tmp1>1.0) tmp1=1.0;
	tmp2=tzero*p*(1+32*p*p);
	res+=tmp1*tmp2;
	if (res==0.0) {
		res=MAXRATE;
	} else {
		res=psize/res;
	}
	return res;
}

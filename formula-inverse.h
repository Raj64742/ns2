
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

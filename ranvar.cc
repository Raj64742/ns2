/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *  
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or derivative
 * work. Xerox grants no other licenses expressed or implied. The Xerox trade
 * name should not be used in any advertising without its written permission.
 *  
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *  
 * These notices must be retained in any copies of any part of this software.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/ranvar.cc,v 1.4 1997/08/25 04:04:38 breslau Exp $ (Xerox)";
#endif

#include "ranvar.h"

RandomVariable::RandomVariable()

{
        rng_ = RNG::defaultrng(); 
}


int RandomVariable::command(int argc, const char*const* argv)
{
        Tcl& tcl = Tcl::instance();

        if (argc == 2) {
	        if (strcmp(argv[1], "value") == 0) {
		        tcl.resultf("%6e", value());
			return(TCL_OK);
		}
	}
	if (argc == 3) {
	        if (strcmp(argv[1], "use-rng") == 0) {
		        rng_ = (RNG*)TclObject::lookup(argv[2]);
			if (rng_ == 0) {
			        tcl.resultf("no such RNG %s", argv[2]);
				return(TCL_ERROR);
			}
			return(TCL_OK);
		}
	}
	return(TclObject::command(argc, argv));
}

static class UniformRandomVariableClass : public TclClass {
 public:
         UniformRandomVariableClass() : TclClass("RandomVariable/Uniform"){}
	 TclObject* create(int, const char*const*) {
	         return(new UniformRandomVariable());
	 }
} class_uniformranvar;

UniformRandomVariable::UniformRandomVariable()
{
        bind("min_", &min_);
	bind("max_", &max_); 
}

UniformRandomVariable::UniformRandomVariable(double min, double max)
{
        min_ = min;
	max_ = max;
}

double UniformRandomVariable::value()
{
	return(rng_->uniform(min_, max_));
}

static class ExponentialRandomVariableClass : public TclClass {
 public:
        ExponentialRandomVariableClass() : TclClass("RandomVariable/Exponential") {}
	TclObject* create(int, const char*const*) {
	        return(new ExponentialRandomVariable());
	}
} class_exponentialranvar;

ExponentialRandomVariable::ExponentialRandomVariable()
{
        bind("avg_", &avg_);
}

ExponentialRandomVariable::ExponentialRandomVariable(double avg)
{
        avg_ = avg;
}

double ExponentialRandomVariable::value()
{
	return(rng_->exponential(avg_));
}


static class ParetoRandomVariableClass : public TclClass {
 public:
        ParetoRandomVariableClass() : TclClass("RandomVariable/Pareto") {}
	TclObject* create(int, const char*const*) {
	        return(new ParetoRandomVariable());
	}
} class_paretoranvar;

ParetoRandomVariable::ParetoRandomVariable()
{
        bind("avg_", &avg_);
	bind("shape_", &shape_);
}

ParetoRandomVariable::ParetoRandomVariable(double avg, double shape)
{
        avg_ = avg;
	shape_ = shape;
}

double ParetoRandomVariable::value()
{
        /* yuck, user wants to specify shape and avg, but the
	 * computation here is simpler if we know the 'scale'
	 * parameter.  right thing is to probably do away with
	 * the use of 'bind' and provide an API such that we
	 * can update the scale everytime the user updates shape
	 * or avg.
	 */
	return(rng_->pareto(avg_ * (shape_ -1)/shape_, shape_));
}
	

static class ConstantRandomVariableClass : public TclClass {
 public:
        ConstantRandomVariableClass() : TclClass("RandomVariable/Constant"){}
	TclObject* create(int, const char*const*) {
	        return(new ConstantRandomVariable());
	}
} class_constantranvar;

ConstantRandomVariable::ConstantRandomVariable()
{
        bind("val_", &val_);
}

ConstantRandomVariable::ConstantRandomVariable(double val)
{
        val_ = val;
}

double ConstantRandomVariable::value()
{
        return(val_);
}


/* Hyperexponential distribution code adapted from code provided
 * by Ion Stoica.
 */

static class HyperExponentialRandomVariableClass : public TclClass {
 public:
        HyperExponentialRandomVariableClass() : 
	TclClass("RandomVariable/HyperExponential") {}
	TclObject* create(int, const char*const*) {
	        return(new HyperExponentialRandomVariable());
	}
} class_hyperexponentialranvar;

HyperExponentialRandomVariable::HyperExponentialRandomVariable()
{
        bind("avg_", &avg_);
	bind("cov_", &cov_);
	alpha_ = .95;
}

HyperExponentialRandomVariable::HyperExponentialRandomVariable(double avg, double cov)
{
	alpha_ = .95;
        avg_ = avg;
	cov_ = cov;
}

double HyperExponentialRandomVariable::value()
{
        double temp, res;
	double u = Random::uniform();

	temp = sqrt((cov_ * cov_ - 1.0)/(2.0 * alpha_ * (1.0 - alpha_)));
	if (u < alpha_)
	        res = Random::exponential(avg_ - temp * (1.0 - alpha_) * avg_);
	else
	        res = Random::exponential(avg_ + temp * (alpha_) * avg_);
	return(res);
}

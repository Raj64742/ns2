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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/ranvar.cc,v 1.3 1997/07/23 01:09:27 kfall Exp $ (Xerox)";
#endif

#include "ranvar.h"

int RandomVariable::command(int argc, const char*const* argv)
{
        Tcl& tcl = Tcl::instance();

        if (argc == 2) {
	        if (strcmp(argv[1], "value") == 0) {
		        tcl.resultf("%6e", value());
			return(TCL_OK);
		}
	}
	return(TclObject::command(argc, argv));
}

class UniformRandomVariable : public RandomVariable {
 public:
        virtual double value();
	UniformRandomVariable();
	UniformRandomVariable(double, double);
 private:
        double min_;
	double max_;
};

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
        bind("min_", &min_);
	bind("max_", &max_); 
        min_ = min;
	max_ = max;
}

double UniformRandomVariable::value()
{
        return(Random::uniform(min_, max_));
}


class ExponentialRandomVariable : public RandomVariable {
 public:
        virtual double value();
	ExponentialRandomVariable();
	ExponentialRandomVariable(double);
 private:
	double avg_;
};

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
        bind("avg_", &avg_);
        avg_ = avg;
}

double ExponentialRandomVariable::value()
{
        return(Random::exponential(avg_));
}


class ParetoRandomVariable : public RandomVariable {
 public:
        virtual double value();
	ParetoRandomVariable();
	ParetoRandomVariable(double, double);
 private:
	double avg_;
	double shape_;
	double scale_;
};

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
        bind("avg_", &avg_);
	bind("shape_", &shape_);
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
        return(Random::pareto(avg_ * (shape_ - 1)/shape_, shape_));
}
	

class ConstantRandomVariable : public RandomVariable {
 public:
        virtual double value();
	ConstantRandomVariable();
	ConstantRandomVariable(double);
 private:
	double avg_;
};

static class ConstantRandomVariableClass : public TclClass {
 public:
        ConstantRandomVariableClass() : TclClass("RandomVariable/Constant"){}
	TclObject* create(int, const char*const*) {
	        return(new ConstantRandomVariable());
	}
} class_constantranvar;

ConstantRandomVariable::ConstantRandomVariable()
{
        bind("avg_", &avg_);
}

ConstantRandomVariable::ConstantRandomVariable(double avg)
{
        bind("avg_", &avg_);
        avg_ = avg;
}

double ConstantRandomVariable::value()
{
        return(avg_);
}


/* Hyperexponential distribution code adapted from code provided
 * by Ion Stoica.
 */

class HyperExponentialRandomVariable : public RandomVariable {
 public:
        virtual double value();
	HyperExponentialRandomVariable();
	HyperExponentialRandomVariable(double, double);
 private:
	double avg_;
	double cov_;
	double alpha_;
};

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
        bind("avg_", &avg_);
	bind("cov_", &cov_);
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

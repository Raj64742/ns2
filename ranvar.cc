/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/ranvar.cc,v 1.14 1999/02/04 06:13:22 yaxu Exp $ (Xerox)";
#endif

#include <stdio.h>
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

/* Pareto distribution of the second kind, aka. Lomax distribution */
static class ParetoIIRandomVariableClass : public TclClass {
 public:
        ParetoIIRandomVariableClass() : TclClass("RandomVariable/ParetoII") {}
        TclObject* create(int, const char*const*) {
                return(new ParetoIIRandomVariable());
        }
} class_paretoIIranvar;

ParetoIIRandomVariable::ParetoIIRandomVariable()
{
        bind("avg_", &avg_);
        bind("shape_", &shape_);
}

ParetoIIRandomVariable::ParetoIIRandomVariable(double avg, double shape)
{
        avg_ = avg;
        shape_ = shape;
}

double ParetoIIRandomVariable::value()
{
        return(rng_->paretoII(avg_ * (shape_ - 1), shape_));
}

static class NormalRandomVariableClass : public TclClass {
 public:
        NormalRandomVariableClass() : TclClass("RandomVariable/Normal") {}
        TclObject* create(int, const char*const*) {
                return(new NormalRandomVariable());
        }
} class_normalranvar;
 
NormalRandomVariable::NormalRandomVariable()
{ 
        bind("avg_", &avg_);
        bind("std_", &std_);
}
 
double NormalRandomVariable::value()
{
        return(rng_->normal(avg_, std_));
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

/*
// Empirical Random Variable:
//  CDF input from file with the following column
//   1.  Possible values in a distrubutions
//   2.  Number of occurances for those values
//   3.  The CDF for those value
//  code provided by Giao Nguyen
*/

static class EmpiricalRandomVariableClass : public TclClass {
public:
	EmpiricalRandomVariableClass() : TclClass("RandomVariable/Empirical"){}
	TclObject* create(int, const char*const*) {
		return(new EmpiricalRandomVariable());
	}
} class_empiricalranvar;

EmpiricalRandomVariable::EmpiricalRandomVariable() : minCDF_(0), maxCDF_(1), maxEntry_(32), table_(0)
{
	bind("minCDF_", &minCDF_);
	bind("maxCDF_", &maxCDF_);
	bind("interpolation_", &interpolation_);
	bind("maxEntry_", &maxEntry_);
}

int EmpiricalRandomVariable::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "loadCDF") == 0) {
			if (loadCDF(argv[2]) == 0) {
				tcl.resultf("%s loadCDF %s: invalid file",
					    name(), argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
	}
	return RandomVariable::command(argc, argv);
}

int EmpiricalRandomVariable::loadCDF(const char* filename)
{
	FILE* fp;
	char line[256];
	CDFentry* e;

	fp = fopen(filename, "r");
	if (fp == 0)
		return 0;

	if (table_ == 0)
		table_ = new CDFentry[maxEntry_];
	for (numEntry_=0;  fgets(line, 256, fp);  numEntry_++) {
		if (numEntry_ >= maxEntry_) {	// resize the CDF table
			maxEntry_ *= 2;
			e = new CDFentry[maxEntry_];
			for (int i=numEntry_-1; i >= 0; i--)
				e[i] = table_[i];
			delete table_;
			table_ = e;
		}
		e = &table_[numEntry_];
		// Use * and l together raises a warning
		sscanf(line, "%lf %*f %lf", &e->val_, &e->cdf_);
	}
	return numEntry_;
}

double EmpiricalRandomVariable::value()
{
	if (numEntry_ <= 0)
		return 0;
	double u = rng_->uniform(minCDF_, maxCDF_);
	int mid = lookup(u);
	if (mid && interpolation_ && u < table_[mid].cdf_)
		return interpolate(u, table_[mid-1].cdf_, table_[mid-1].val_,
				   table_[mid].cdf_, table_[mid].val_);
	return table_[mid].val_;
}

double EmpiricalRandomVariable::interpolate(double x, double x1, double y1, double x2, double y2)
{
	double value = y1 + (x - x1) * (y2 - y1) / (x2 - x1);
	if (interpolation_ == INTER_INTEGRAL)	// round up
		return ceil(value);
	return value;
}

int EmpiricalRandomVariable::lookup(double u)
{
	// always return an index whose value is >= u
	int lo, hi, mid;
	if (u <= table_[0].cdf_)
		return 0;
	for (lo=1, hi=numEntry_-1;  lo < hi; ) {
		mid = (lo + hi) / 2;
		if (u > table_[mid].cdf_)
			lo = mid + 1;
		else hi = mid;
	}
	return lo;
}

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
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tools/ranvar.h,v 1.4 1997/08/25 04:04:39 breslau Exp $ (Xerox)
 */

#ifndef ns_ranvar_h
#define ns_ranvar_h


/* XXX still need to clean up dependencies among parameters such that
 * when one parameter is changed, other parameters are recomputed as
 * appropriate.
 */

#include "tclcl.h"
#include "random.h"
#include "rng.h"

class RandomVariable : public TclObject {
 public:
        virtual double value() = 0;
	int command(int argc, const char*const* argv);
	RandomVariable();
 protected:
	RNG* rng_;
};

class UniformRandomVariable : public RandomVariable {
 public:
        virtual double value();
	UniformRandomVariable();
	UniformRandomVariable(double, double);
	double* minp() { return &min_; };
	double* maxp() { return &max_; };
	double min()   { return min_; };
	double max()   { return max_; };
	void setmin(double d)  { min_ = d; };
	void setmax(double d)  { max_ = d; };
 private:
        double min_;
	double max_;
};

class ExponentialRandomVariable : public RandomVariable {
 public:
        virtual double value();
	ExponentialRandomVariable();
	ExponentialRandomVariable(double);
	double* avgp() { return &avg_; };
	double avg() { return avg_; };
	void setavg(double d) { avg_ = d; };
 private:
	double avg_;
};

class ParetoRandomVariable : public RandomVariable {
 public:
	virtual double value();
	ParetoRandomVariable();
	ParetoRandomVariable(double, double);
	double* avgp() { return &avg_; };
	double* shapep() { return &shape_; };
	double avg()   { return avg_; };
	double shape()   { return shape_; };
	void setavg(double d)  { avg_ = d; };
	void setshape(double d)  { shape_ = d; };
 private:
	double avg_;
	double shape_;
	double scale_;
};

class ConstantRandomVariable : public RandomVariable {
 public:
        virtual double value();
	ConstantRandomVariable();
	ConstantRandomVariable(double);
	double* valp() { return &val_; };
	double val() { return val_; };
	void setval(double d) { val_ = d; };
 private:
	double val_;
};

class HyperExponentialRandomVariable : public RandomVariable {
 public:
        virtual double value();
	HyperExponentialRandomVariable();
	HyperExponentialRandomVariable(double, double);
	double* avgp() { return &avg_; };
	double* covp() { return &cov_; };
	double avg()   { return avg_; };
	double cov()   { return cov_; };
	void setavg(double d)  { avg_ = d; };
	void setcov(double d)  { cov_ = d; };
 private:
	double avg_;
	double cov_;
	double alpha_;
};



#endif

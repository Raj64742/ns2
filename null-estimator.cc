/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or
 * derivative work. Xerox grants no other licenses expressed or
 * implied. The Xerox trade name should not be used in any advertising
 * without its written permission. 
 *
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * software. 
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/null-estimator.cc,v 1.1 1998/05/04 18:29:26 breslau Exp $";
#endif

/* Dummy estimator that only measures actual utilization */

#include <stdlib.h>
#include "estimator.h"

class Null_Est : public Estimator {
 public:
        Null_Est();
 protected:
	void estimate();
};

Null_Est::Null_Est()
{

}

void Null_Est::estimate() {

	measload_ = meas_mod_->bitcnt()/period_;
	meas_mod_->resetbitcnt();

}
static class Null_EstClass : public TclClass {
public:
	Null_EstClass() : TclClass ("Est/Null") {}
	TclObject* create(int,const char*const*) {
		return (new Null_Est());
	}
}class_null_est;

/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- 
 *
 * Copyright (c) 1997, 2000 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mobile/energy-model.h,v 1.10 2000/08/30 18:54:03 haoboy Exp $
 */

// Contributed by Satish Kumar (kkumar@isi.edu)

#ifndef energy_model_h_
#define energy_model_h_

#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "config.h"
#include "trace.h"
#include "rng.h"

class EnergyModel : public TclObject {
public:
	EnergyModel(double energy, double l1, double l2) :
		energy_(energy), initialenergy_(energy), 
		level1_(l1), level2_(l2) {}
	inline double energy() const { return energy_; }
	inline double initialenergy() const { return initialenergy_; }
	inline double level1() const { return level1_; }
	inline double level2() const { return level2_; }
	inline void setenergy(double e) { energy_ = e; }
   
	// ------------------------------------
	// Modified by Chalermek 
	virtual void DecrTxEnergy(double txtime, double P_tx);
	virtual void DecrRcvEnergy(double rcvtime, double P_rcv);
	virtual void DecrIdleEnergy(double idletime, double P_idle);
	inline virtual double MaxTxtime(double P_tx) {
		return(energy_/P_tx);
	}
	inline virtual double MaxRcvtime(double P_rcv) {
		return(energy_/P_rcv);
	}
	inline virtual double MaxIdletime(double P_idle) {
		return(energy_/P_idle);
	}
	// ------------------------------------

protected:
	double energy_;
	double initialenergy_;
	double level1_;
	double level2_;
};


#endif




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
 * $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/energy-model.cc,v 1.4 2000/08/30 18:54:03 haoboy Exp $
 */

// Contributed by Satish Kumar <kkumar@isi.edu>

#include <stdarg.h>
#include <float.h>

#include "energy-model.h"
#include "god.h"

static class EnergyModelClass:public TclClass
{
public:
	EnergyModelClass ():TclClass ("EnergyModel") { }
	TclObject *create (int argc, const char *const *argv) {
		if (argc == 7)
			return (new EnergyModel(atof(argv[4]), atof(argv[5]), 
						atof(argv[6])));
		else 
			return 0;
	}
} class_energy_model;


// Modified by Chalermek
void EnergyModel::DecrTxEnergy(double txtime, double P_tx) 
{
	double dEng = P_tx * txtime;
	if (energy_ <= dEng)
		energy_ = 0.0;
	else
		energy_ = energy_ - dEng;
	if (energy_ <= 0.0)
		God::instance()->ComputeRoute();
}


void EnergyModel::DecrRcvEnergy(double rcvtime, double P_rcv) 
{
	double dEng = P_rcv * rcvtime;
	if (energy_ <= dEng)
		energy_ = 0.0;
	else
		energy_ = energy_ - dEng;
	if (energy_ <= 0.0)
		God::instance()->ComputeRoute();
}

void EnergyModel::DecrIdleEnergy(double idletime, double P_idle) 
{
	double dEng = P_idle * idletime;
	if (energy_ <= dEng)
		energy_ = 0.0;
	else
		energy_ = energy_ - dEng;
	if (energy_ <= 0.0)
		God::instance()->ComputeRoute();
}





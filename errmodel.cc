/*
 * Copyright (c) 1996-1997 Regents of the University of California.
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
 *	Engineering Group at Lawrence Berkeley Laboratory and the Daedalus
 *	research group at UC Berkeley.
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
 */

#include <iostream.h>
#include <stdlib.h>
#include "random.h"
#include "packet.h"
#include "errmodel.h"


static class ErrorModelClass : public TclClass {
public:
	ErrorModelClass() : TclClass("ErrorModel") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new ErrorModel);
	}
} class_errormodel;


int 
ErrorModel::command(int argc, const char*const* argv)
{
	int ac = 0;
	if (!strcmp(argv[ac+1], "uniform")) {
		rate_ = atof(argv[ac+2]);
		return (TCL_OK);
	}
	if (!strcmp(argv[ac+1], "unit")) {
		if (!strcmp(argv[ac+2], "bit"))
			unit_ = EuBit;
		else if (!strcmp(argv[ac+2], "time"))
			unit_ = EuTime;
		else
			unit_ = EuPacket;
		return (TCL_OK);
	}
	if (!strcmp(argv[ac+1], "dump")) {
		ofs_.open(argv[ac+2]);
		if (ofs_)
			return (TCL_OK);
	}
	return (TCL_ERROR);
}


void
ErrorModel::recv(Packet* p)
{
	corrupt(p);
	target_->recv(p);
}


int
ErrorModel::corrupt(Packet* p)
{
	double u = Random::uniform();
	if (u < rate_) {
		loss_++;
		p->error() |= 1;
	}
	else {
		if (loss_) {
			dump();
			loss_ = good_ = 0;
		}
		good_++;
	}
	return (u < rate_);
}


void
ErrorModel::dump()
{
	if (ofs_) {
		ofs_ << Scheduler::instance().clock()
			<< "\t" << loss_ << "\t" << good_ << "\n";
	}
}

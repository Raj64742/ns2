/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
//
// Copyright (c) 2001 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// Management function to playback TCPDump trace file (after post precssing)
// Xuan Chen (xuanc@isi.edu)
//
#include "tpm.h"

// Timer to send requests
TPMTimer::TPMTimer(TPM* t) {
	tpm = t;
};

void TPMTimer::expire(Event *e) {
	//if (e) 
	//	Packet::free((Packet *)e);

	tpm->run();
}


static class TPMClass : public TclClass {
public:
        TPMClass() : TclClass("TPM") {}
        TclObject* create(int, const char*const*) {
		return (new TPM());
	}
} class_tpm;

TPM::TPM() {
	tp_default_ = tp_num_ = 0;
	tp = NULL;
	
	// initialize next request
	next_p.time = 0;
	next_p.saddr = 0;
	next_p.sport = 0;
	next_p.daddr = 0;
	next_p.dport = 0;
	next_p.len = 0;

	// initialize request timer
	timer = new TPMTimer(this);
	start_t = 0;
}

TPM::~TPM() {
	if (fp)
		fclose(fp);
	if (timer)
		delete timer;
}

int TPM::loadLog(const char* filename) {
	fp = fopen(filename, "r");
	if (fp == 0)
		return(0);
	
	return(1);
}

int TPM::start() {
	start_t = Scheduler::instance().clock();
	processLog();
	return(1);
}

int TPM::processLog() {
	int time, sport, dport, len;
	unsigned int saddr, daddr;

	while (!feof(fp)) {
		fscanf(fp, "%d %u %d %u %d %d\n",
		       &time, &saddr, &sport, &daddr, &dport, &len);

		if (time > 0) {
			// save information for next request
			next_p.time = time;
			next_p.saddr = saddr;
			next_p.sport = sport;
			next_p.daddr = daddr;
			next_p.dport = dport;
			next_p.len = len;	
			
			//double now = Scheduler::instance().clock();
			//double delay = time + start_t - now;
			double delay = time * 1.0 / 1000000.0;
			timer->resched(delay);
			
			return(1);
		} else {
			genPkt(saddr, sport, daddr, dport, len);
		}
	}
	return(1);
}

int TPM::run() {
	genPkt(next_p.saddr, next_p.sport, next_p.daddr, next_p.dport, next_p.len);
	processLog();
	return(1);
}

int TPM::genPkt(unsigned int saddr, int sport, unsigned int daddr, int dport, int len) {
	// we can send packets to anybody with twinks...
	if (saddr == daddr) {
	       return(0);
	}

	// use the default TP agent unless specified.
  	unsigned int tp_id = tp_default_;

	if (saddr < (unsigned int)tp_num_) {
		tp_id = saddr;
	}
	printf("tpid %d\n", tp_id);
	tp[tp_id]->sendto(len, saddr, sport, daddr, dport);
	
	return(1);
}

int TPM::command(int argc, const char*const* argv) {
	if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			if (start())
				return(TCL_OK);
			else
				return(TCL_ERROR);
			
		}
		
	} else if (argc == 3) {
		if (strcmp(argv[1], "loadLog") == 0) {
			if (loadLog(argv[2]))
				return(TCL_OK);
			else
				return(TCL_ERROR);

		} else if (strcmp(argv[1], "tp-num") == 0) {
			tp_num_ = atoi(argv[2]);
			tp_default_ = tp_num_ - 1;
			if (tp_default_ < 0)
			  tp_default_ = 0;
			tp = new TPAgent*[tp_num_];
			return(TCL_OK);
		} else if (strcmp(argv[1], "tp-default") == 0) {
		        tp_default_ = atoi(argv[2]);
			return(TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "tp-reg") == 0) {
			int index = atoi(argv[2]);
			if (index >= tp_num_)
				return(TCL_ERROR);

			tp[index] = (TPAgent *) TclObject::lookup(argv[3]);
			return(TCL_OK);
		}
	}

	printf("Command specified does not exist\n");
	return(TCL_ERROR);
}

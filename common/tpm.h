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
#ifndef ns_tpm_h
#define ns_tpm_h

#include "tp.h"

// Data structure for next packet
struct nextp_s {
	int time;
	unsigned int saddr, daddr; 
	int sport, dport;
	int len;
};

class TPM;

// Data structure for the timer to send requests
class TPMTimer : public TimerHandler{
public:
	TPMTimer(TPM*);
	
	virtual void expire(Event *e);

private:
	TPM *tpm;
};

class TPM  : public TclObject {
public: 
	TPM(); 
	virtual ~TPM(); 

	int run();
protected:
	virtual int command(int argc, const char*const* argv);

private:
	// handler to the http log file
	FILE* fp;

	double start_t;
	TPMTimer *timer;
	nextp_s next_p;

	int loadLog(const char*);
	int start();
	int genPkt(unsigned int, int, unsigned int, int, int);
	int processLog();

	TPAgent** tp;
	int tp_num_;
	int tp_default_;
};


#endif // ns_tpm_h

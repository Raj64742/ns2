/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/* fsm.h
 * Copyright (C) 1999 by USC/ISI
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 * 
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/fsm.h,v 1.1 1999/05/13 23:59:58 polly Exp $ (LBL)
 */

#include "scheduler.h"

#define RTT 1
#define TIMEOUT 2

class FSMState {
public:
	/* number of packets in this batch of transmission */
	int batch_size_; 
	/* time to make transition from previous state to this one */
	int transition_[17];
	/* next states if dropping packet #1-16, 0 for none */
	FSMState* drop_[17];
};


class FSM : public TclObject {
public:
	FSM() {};
	inline FSMState* start_state() {	// starting state
		return (start_state_);
	}
	static FSM& instance() {
		return (*instance_);		// general access to scheduler
	}
	void print_FSM(FSMState* state);
protected:
	FSMState* start_state_;
	static FSM* instance_;
};



class TahoeAckFSM : public FSM {
public:
	TahoeAckFSM();
	inline FSMState* start_state() {	// starting state
		return (start_state_);
	}
	static TahoeAckFSM& instance() {
		return (*instance_);	       // general access to TahoeAckFSM
	}
protected:
	FSMState* start_state_;
	static TahoeAckFSM* instance_;

};

class RenoAckFSM : public FSM {
public:
	RenoAckFSM();
	inline FSMState* start_state() {	// starting state
		return (start_state_);
	}
	static RenoAckFSM& instance() {
		return (*instance_);	       // general access to TahoeAckFSM
	}
protected:
	FSMState* start_state_;
	static RenoAckFSM* instance_;
};


class TahoeDelAckFSM : public FSM {
public:
	TahoeDelAckFSM();
	inline FSMState* start_state() {	// starting state
		return (start_state_);
	}
	static TahoeDelAckFSM& instance() {
		return (*instance_);	       // general access to TahoeAckFSM
	}
protected:
	FSMState* start_state_;
	static TahoeDelAckFSM* instance_;
};

class RenoDelAckFSM : public FSM {
public:
	RenoDelAckFSM();
	inline FSMState* start_state() {	// starting state
		return (start_state_);
	}
	static RenoDelAckFSM& instance() {
		return (*instance_);	       // general access to TahoeAckFSM
	}
protected:
	FSMState* start_state_;
	static RenoDelAckFSM* instance_;
};

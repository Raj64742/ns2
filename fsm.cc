/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/* fsm.cc
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/fsm.cc,v 1.3 1999/07/16 17:06:18 heideman Exp $ (LBL)
 */

#include "fsm.h"
#include <assert.h>

FSM* FSM::instance_;
TahoeAckFSM* TahoeAckFSM::instance_;
RenoAckFSM* RenoAckFSM::instance_;
TahoeDelAckFSM* TahoeDelAckFSM::instance_;
RenoDelAckFSM* RenoDelAckFSM::instance_;



void
FSMState::number_all()
{
	if (processed())
		return;
	static int next_i = 0;
	print_i_ = ++next_i;
	//
	int i;
	for (i = 0; i < 17; i++)
		if (drop_[i])
			drop_[i]->number_all();
}

void
FSMState::reset_all_processed()
{
	if (print_i_ == 0)
		number_all();
	// requires a full traversal always to work
	if (!processed())
		return;
	print_i_ = -print_i_;
	int i;
	for (i = 0; i < 17; i++)
		if (drop_[i])
			drop_[i]->reset_all_processed();
}

void
FSMState::print_all(int level)
{
	if (processed())
		return;

	const int SPACES_PER_LEVEL = 2;
	printf("#%-2d %*s %d:\n", print_i_, level * SPACES_PER_LEVEL + 1, " ", batch_size_);
	int i;
	for (i = 0; i <= batch_size_; i++) {
		static char *delay_names[] = {"done", "error", "RTT", "timeout" };
		assert(transition_[i] >= -1 && transition_[i] <= TIMEOUT);
		printf("   %*s %d %s -> #%d\n", level * SPACES_PER_LEVEL + 3, " ",
		       i,
		       delay_names[transition_[i]+1],
		       drop_[i] ? drop_[i]->print_i_ : 0);
		if (drop_[i])
			drop_[i]->print_all(level + 1);
	};
}

void
FSMState::print_all_stats(int desired_pkts, int pkts,
			  int rtts,
			  int timeouts,
			  int ps,
			  int qs,
			  int num_states)
{
	int i;
	static FSMState *states[17];

	// remember us!
	states[num_states] = this;
	num_states++;

	if (pkts >= desired_pkts || qs > 5) {
		// done; print states and probability
		printf("%s: p^%d*q^%d, %d rtt, %d to, %d states:",
		       (pkts >= desired_pkts ? "desired_pkts" : "qs exceeded"),
		       ps, qs,
		       rtts, timeouts,
		       num_states);
		char ch = ' ';
		for (i = 0; i < num_states; i++) {
			printf ("%c#%d", ch, states[i]->print_i_);
			ch = ',';
		};
		printf("\n");
		return;
	};

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif
	int desired_pkts_this_round = MIN(desired_pkts - pkts, batch_size_);
	int desired_pkts_with_loss = MAX(desired_pkts_this_round - 1, 0);

	// xxx: doesn't handle tail behavior

	// no losses?
	drop_[0]->print_all_stats(desired_pkts, pkts + desired_pkts_this_round,
				  rtts + 1, timeouts,
				  ps + desired_pkts_this_round, qs,
				  num_states);
	if (qs) {
		printf ("unimplemented: p^%d*q^%d\n", ps + desired_pkts_with_loss, qs + 1);
		return;
	};
	// losses
	for (i = 1; i <= batch_size_; i++) {
		drop_[i]->print_all_stats(desired_pkts, pkts + desired_pkts_with_loss,
					  rtts + (transition_[i] == RTT ? 1 : 0),
					  timeouts + (transition_[i] == TIMEOUT ? 1 : 0),
					  ps + desired_pkts_with_loss, qs + 1,
					  num_states);
	};
}


void
FSM::print_FSM(FSMState* state)
{
#if 0
	int i;

	if (state != NULL) {
		for (i=0; i<17; i++) {
			if (state->drop_[i] != NULL) {
				if (i==0) 
					printf("%d->(%d) ", state->transition_[i], state->drop_[i]->batch_size_);
				else
					printf("\n%d->(%d) ", state->transition_[i], state->drop_[i]->batch_size_);
				print_FSM(state->drop_[i]);
			}
		}
	}
#else /* ! 0 */
	state->reset_all_processed();
	state->print_all(0);
#endif /* 0 */
}

void
FSM::print_FSM_stats(FSMState* state, int n)
{
	state->reset_all_processed();
	state->print_all_stats(n);
}

static class TahoeAckFSMClass : public TclClass {
public:
        TahoeAckFSMClass() : TclClass("FSM/TahoeAck") {}
        TclObject* create(int , const char*const*) {
                return (new TahoeAckFSM);
        }
} class_tahoeackfsm;

static class RenoAckFSMClass : public TclClass {
public:
        RenoAckFSMClass() : TclClass("FSM/RenoAck") {}
        TclObject* create(int , const char*const*) {
                return (new RenoAckFSM);
        }
} class_renoackfsm;

static class TahoeDelAckFSMClass : public TclClass {
public:
        TahoeDelAckFSMClass() : TclClass("FSM/TahoeDelAck") {}
        TclObject* create(int , const char*const*) {
                return (new TahoeDelAckFSM);
        }
} class_tahoedelackfsm;

static class RenoDelAckFSMClass : public TclClass {
public:
        RenoDelAckFSMClass() : TclClass("FSM/RenoDelAck") {}
        TclObject* create(int , const char*const*) {
                return (new RenoDelAckFSM);
        }
} class_renodelackfsm;


// ***********************************************
// Tahoe-Ack TCP Connection Finite State Machine *
// ***********************************************
TahoeAckFSM::TahoeAckFSM() : FSM(), start_state_(NULL) 
{
	int i;
	FSMState* tmp;


	instance_ = this;
	// (wnd, ssh) == (1, 20)
	start_state_ = new FSMState;
	//printf("creating Tahoe Ack FSM\n"); 
	for (i=0; i<17; i++) {
		start_state_->drop_[i] = NULL;
		start_state_->transition_[i] = -1;
	}
	start_state_->batch_size_ = 1;

	// (wnd, ssh) == (2, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0] = tmp;
	start_state_->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->transition_[2] = RTT;

	// (wnd, ssh) == (4, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[3] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[4] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[4] = RTT;

	//(wnd, ssh) == (8, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 8;
	start_state_->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[3] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[4] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 8;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[5] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 10;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[6] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[6] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 12;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[7] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 14;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[8] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[8] = RTT;

	//(wnd, ssh) == (16, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 16;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	for (i=1; i<17; i++) start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[i] = tmp;
	for (i=1; i<14; i++)
		start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[i] = RTT;
	for (i=14; i<17; i++)
		start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[i] = TIMEOUT;


	//(wnd, ssh) == (1, 2), timeout
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[1] = tmp;
	start_state_->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[2]->transition_[0] = TIMEOUT;
	start_state_->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[1] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[2]->transition_[0] = RTT;

	//(wnd, ssh) == (2, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0] = tmp;
	start_state_->drop_[1]->transition_[0] = RTT;

	//(wnd, ssh) == (2.5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (3, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (6, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (7, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 7;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (7, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 7;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (1, 3)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[4]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[4]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (1, 4)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[1] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->transition_[0] = 0;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->transition_[0] = RTT;


	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (1, 5)	
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->transition_[0] = 0;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4]->transition_[0] = 0;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (1, 6)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->transition_[0] = 0;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[6]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[6]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (1, 7)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[8]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[8]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//print_FSM(start_state_);
	//printf("\n");
}

// **********************************************
// Reno-ACK TCP Connection Finite State Machine *
// **********************************************
RenoAckFSM::RenoAckFSM() : FSM(), start_state_(NULL) 
{
	int i;
	FSMState* tmp;
	//printf("creating Reno Ack FSM\n");

	instance_ = this;
	// (wnd, ssh) == (1, 20)
	start_state_ = new FSMState;
	for (i=0; i<17; i++) {
		start_state_->drop_[i] = NULL;
		start_state_->transition_[i] = -1;
	}
	start_state_->batch_size_ = 1;

	// (wnd, ssh) == (2, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0] = tmp;
	start_state_->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->transition_[2] = RTT;

	// (wnd, ssh) == (4, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[3] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[4] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[4] = RTT;

	//(wnd, ssh) == (8, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 8;
	start_state_->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[3] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[4] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 8;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[5] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 10;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[6] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[6] = RTT;


	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[6]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[6]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 12;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[7] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 14;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[8] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[8] = RTT;

	//(wnd, ssh) == (16, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 16;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	for (i=1; i<17; i++) start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[i] = tmp;
	for (i=1; i<14; i++)
		start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[i] = RTT;
	for (i=14; i<17; i++)
		start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[i] = TIMEOUT;

	//(wnd, ssh) == (1, 2), timeout
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[1] = tmp;
	start_state_->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[2]->transition_[0] = TIMEOUT;

	//(wnd, ssh) == (2, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0] = tmp;
	start_state_->drop_[1]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[1] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[2]->transition_[0] = RTT;

	//(wnd, ssh) == (2.5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (3, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (6, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (7, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 7;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (7.5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 7;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (3, 3)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[4]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[4]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;



	//(wnd, ssh) == (4, 4)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[1] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->transition_[0] = RTT;


	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (5, 5)	
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->transition_[0] = 0;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4]->transition_[0] = 0;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 7;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (6, 6)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->transition_[0] = 0;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[6]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[6]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 7;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (7, 7)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 7;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[7]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[8]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[8]->transition_[0] = RTT;

	//print_FSM(start_state_);
	//printf("\n");

}

// *****************************************************
// Tahoe-Delay Ack TCP Connection Finite State Machine *
// *****************************************************
TahoeDelAckFSM::TahoeDelAckFSM() : FSM(), start_state_(NULL) 
{
	int i;
	FSMState* tmp;
	//printf("creating Tahoe DelAck FSM\n");

	instance_ = this;
	// (wnd, ssh) == (1, 20)
	start_state_ = new FSMState;
	for (i=0; i<17; i++) {
		start_state_->drop_[i] = NULL;
		start_state_->transition_[i] = -1;
	}
	start_state_->batch_size_ = 1;

	// (wnd, ssh) == (2, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0] = tmp;
	start_state_->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->transition_[2] = RTT;

	// (wnd, ssh) == (3, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[3] = RTT;

	//(wnd, ssh) == (5, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[3] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[4] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[5] = RTT;

	//(wnd, ssh) == (8, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 8;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[3] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[4] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[5] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[5] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 8;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[6] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[6] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 9;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[7] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[7] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 11;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[8] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[8] = RTT;

	//(wnd, ssh) == (12, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 12;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	for (i=1; i<13; i++) start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[i] = tmp;
	for (i=1; i<10; i++)
		start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[i] = RTT;
	for (i=10; i<13; i++)
		start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[i] = TIMEOUT;


	//(wnd, ssh) == (1, 2), timeout
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[1] = tmp;
	start_state_->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[2]->transition_[0] = TIMEOUT;

	start_state_->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[2]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->transition_[0] = RTT;

	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[1] = RTT;

	//(wnd, ssh) == (2, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0] = tmp;
	start_state_->drop_[1]->transition_[0] = RTT;

	//(wnd, ssh) == (2.5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (2.9, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (3, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (3.3, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4.7, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (6, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (1, 3)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (1, 4)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[1] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[2]->transition_[0] = 0;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[3]->transition_[0] = 0;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->transition_[0] = RTT;


	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (1, 5)	
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->transition_[0] = 0;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[5]->transition_[0] = 0;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[6]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[6]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[7]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[7]->transition_[0] = RTT;


	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (1, 6)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[8]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[8]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[8]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[8]->drop_[0]->transition_[0] = RTT;


	//print_FSM(start_state_);
	//printf("\n");
}

// ****************************************************
// Reno-Delay Ack TCP Connection Finite State Machine *
// ****************************************************
RenoDelAckFSM::RenoDelAckFSM() : FSM(), start_state_(NULL) 
{
	int i;
	FSMState* tmp;
	//printf("creating Reno DelAck FSM\n");

	instance_ = this;
	// (wnd, ssh) == (1, 20)
	start_state_ = new FSMState;
	for (i=0; i<17; i++) {
		start_state_->drop_[i] = NULL;
		start_state_->transition_[i] = -1;
	}
	start_state_->batch_size_ = 1;

	// (wnd, ssh) == (2, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0] = tmp;
	start_state_->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->transition_[2] = RTT;

	// (wnd, ssh) == (3, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[3] = RTT;

	//(wnd, ssh) == (5, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[3] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[4] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[5] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[4]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[5]->transition_[0] = RTT;

	//(wnd, ssh) == (8, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 8;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[2] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[2] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[3] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[3] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[2]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[3]->transition_[0] = RTT;


	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[4] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[5] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[5] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 8;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[6] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[6] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 9;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[7] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[7] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 11;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[8] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[8] = RTT;

	//(wnd, ssh) == (12, 20)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 12;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	for (i=1; i<13; i++) start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[i] = tmp;
	for (i=1; i<10; i++)
		start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[i] = RTT;
	for (i=10; i<13; i++)
		start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[i] = TIMEOUT;


	//(wnd, ssh) == (1, 2), timeout
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 1;
	start_state_->drop_[1] = tmp;
	start_state_->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[2]->transition_[0] = TIMEOUT;

	start_state_->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->transition_[1] = TIMEOUT;
	start_state_->drop_[0]->drop_[0]->drop_[2]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[2]->transition_[0] = TIMEOUT;

	//(wnd, ssh) == (2, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0] = tmp;
	start_state_->drop_[1]->transition_[0] = RTT;

	//(wnd, ssh) == (2.5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (2.9, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (3, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (3.3, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4.7, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (6, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[1]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (2, 2), rtt
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->transition_[1] = RTT;

	//(wnd, ssh) == (2.5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 2;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (3, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4.3, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4.7, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (5, 2)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (3.3, 3)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 3;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[3]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (4, 4)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->transition_[1] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 4;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[2]->drop_[0]->transition_[0] = RTT;


	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[1]->drop_[0]->transition_[0] = RTT;


	//(wnd, ssh) == (5, 5)	
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->transition_[0] = 0;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[5]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[5]->transition_[0] = 0;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[6]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[6]->transition_[0] = RTT;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[7]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[7]->transition_[0] = RTT;


	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->transition_[0] = RTT;

	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 5;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->drop_[0]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[4]->drop_[0]->drop_[0]->transition_[0] = RTT;

	//(wnd, ssh) == (6, 6)
	tmp = new FSMState;
	for (i=0; i<17; i++) {
		tmp->drop_[i] = NULL;
		tmp->transition_[i] = -1;
	}
	tmp->batch_size_ = 6;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[8]->drop_[0] = tmp;
	start_state_->drop_[0]->drop_[0]->drop_[0]->drop_[0]->drop_[8]->transition_[0] = RTT;

	//print_FSM(start_state_);
	//printf("\n");
}


// Copyright (c) Xerox Corporation 1998. All rights reserved.
//
// License is granted to copy, to use, and to make and to use derivative
// works for research and evaluation purposes, provided that Xerox is
// acknowledged in all documentation pertaining to any such copy or
// derivative work. Xerox grants no other licenses expressed or
// implied. The Xerox trade name should not be used in any advertising
// without its written permission. 
//
// XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
// MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
// FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
// express or implied warranty of any kind.
//
// These notices must be retained in any copies of any part of this
// software. 
//
// Auxiliary classes for Http multicast invalidation proxy cache
//
// $Headers$

#include "http-aux.h"
#include "http.h"

void PushTimer::expire(Event *) 
{
	a_->timeout(HTTP_UPDATE);
}

void HBTimer::expire(Event *) 
{
	a_->timeout(HTTP_INVALIDATION);
}

void LivenessTimer::expire(Event *)
{
	a_->handle_node_failure(nbr_);
}

void HttpHbData::extract(InvalidationRec*& head)
{
	// XXX head must be passed in from outside, because the 
	// InvalidationRec list will obtain the address of head
	head = NULL;
	InvalidationRec *q = NULL;
	// reconstruct a list of invalidation records.
	// We keep the updating record 
	for (int i = 0; i < num_inv(); i++) {
		// Used only to mark that this page will be send in 
		// the next multicast update. The updating field in 
		// this agent will only be set after it gets the real 
		// update. Here we set this field temporarily so that
		// recv_inv can find out the 
		q = inv_rec()[i].copyto();
		q->insert(&head);
	}
}

NeighborCache::~NeighborCache()
{
	timer_->cancel();
	delete timer_;
	ServerEntry *s = sl_.gethead(), *p;
	while (s != NULL) {
		p = s;
		s = s->next();
		delete p;
	}
}

int NeighborCache::is_server_down(int sid)
{
	ServerEntry *s = sl_.gethead();
	while (s != NULL) {
		if (s->server() == sid)
			return s->is_down();
		s = s->next();
	}
	return 0;
}

void NeighborCache::server_down(int sid)
{
	ServerEntry *s = sl_.gethead();
	while (s != NULL) {
		if (s->server() == sid)
			s->down();
		s = s->next();
	}
}

void NeighborCache::server_up(int sid)
{
	ServerEntry *s = sl_.gethead();
	while (s != NULL) {
		if (s->server() == sid)
			s->up();
		s = s->next();
	}
}

void NeighborCache::invalidate(HttpMInvalCache *c)
{
	ServerEntry *s = sl_.gethead();
	while (s != NULL) {
		c->invalidate_server(s->server());
		s = s->next();
	}
}

void NeighborCache::pack_leave(HttpLeaveData& data)
{
	int i;
	ServerEntry *s;
	for (i = 0, s = sl_.gethead(); s != NULL; s = s->next(), i++)
		data.add(i, s->server());
}

void NeighborCache::add_server(int sid)
{
	ServerEntry *s = new ServerEntry(sid);
	sl_.insert(s);
}

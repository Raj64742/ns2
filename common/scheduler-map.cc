/* -*-	Mode:C++; c-basic-offset:2; tab-width:2; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1994 Regents of the University of California.
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
 */

#include "config.h"
#ifdef HAVE_STL
// Event Scheduler using the Standard Template Library map and deque
// Contributed by George F. Riley, Georgia Tech.  Spring 2002

#include <stdio.h>
#include <map>
#include <deque>

#include "scheduler.h"

typedef pair<double,scheduler_uid_t> KeyPair_t; // The key for the multimap
typedef map<KeyPair_t, Event*, less<KeyPair_t> > EventMap_t;
typedef deque<Event*> UIDDeq_t;                 // For looking up ev by uid

class MapScheduler : public Scheduler {
public:
  MapScheduler();
  virtual ~MapScheduler();
public:
	int command(int argc, const char*const* argv);
  virtual void cancel(Event*);	                // cancel event
  virtual void insert(Event*);	                // schedule event
  virtual Event* lookup(scheduler_uid_t uid);	  // look for event
  virtual Event* deque();		                    // next event (removes from q)
protected:
  EventMap_t      EventList;                    // The actual event list
#ifdef USING_UIDDEQ
	UIDDeq_t        UIDDeq;                       // DEQ Lookup
#endif
  scheduler_uid_t fuid;
  scheduler_uid_t luid;   // First and last+1 in UIDDeq
  unsigned long   totev;  // Total events (debug)
  unsigned long   totrm;  // Total events removed (debug)
	bool            verbose;// True if verbose debug
	unsigned long   verbosemod;  // Mod factor for verbosity
  EventMap_t::iterator hint;   // Hint for insertions (right after prior)
private:
  void CleanUID();        // Clean up the UID Deq
  void DBDump(const char* pMsg = NULL);          // Debug Dump
};

static class MapSchedulerClass : public TclClass {
public:
	MapSchedulerClass() : TclClass("Scheduler/Map") {}
	TclObject* create(int /* argc */, const char*const* /* argv */) {
		return (new MapScheduler);
	}
} class_stl_sched;

MapScheduler::MapScheduler()
  : fuid(1), luid(1), totev(0), totrm(0), verbose(false), verbosemod(1000)
{
	hint = EventList.end();
}

MapScheduler::~MapScheduler()
{
}

int MapScheduler::command(int argc, const char*const* argv)
{
	if (argc == 2)
		{
			if (strcmp(argv[1], "verbose") == 0)
				{
					verbose = true;
					return (TCL_OK);
				}
		}
	if (argc == 3)
		{
			if (strcmp(argv[1], "verbose") == 0)
				{
					verbose = true;
					verbosemod = atol(argv[2]);
					return (TCL_OK);
				}
		}
	return Scheduler::command(argc, argv);
}

void MapScheduler::cancel(Event* p)
{
  EventMap_t::iterator i = EventList.find(KeyPair_t(p->time_, p->uid_));
  if (i != EventList.end())
    {
      EventList.erase(i);
			hint = EventList.end(); 
      totrm++;
#ifdef USING_UIDDEQ
      // Null out the UIDDeq entry
      if (p->uid_ >= fuid && p->uid_ < luid)
				{
					UIDDeq[p->uid_ - fuid] = NULL;
				}
#endif
			p->uid_ = -p->uid_; // Negate the uid for reuse
    }
}

void MapScheduler::insert(Event* p)
{
  hint = EventList.insert(
							hint, EventMap_t::value_type(KeyPair_t(p->time_, p->uid_), p));
  // And manage the UID Deq
  if (p->uid_ != luid)
    {
      printf("HuH?  MapScheduler::insert uid mismatch ");
			printf(UID_PRINTF_FORMAT, p->uid_);
			printf(" ");
			printf(UID_PRINTF_FORMAT, luid);
			printf("\n");
    }
#ifdef USING_UIDDEQ
  UIDDeq.push_back(p);
#endif
	luid++;
	totev++;
	if (verbose && ((totev % verbosemod) == 0))
		{
			printf("STLSched :total of %ld events, current size %ld\n",
						 totev, totev - totrm);
		}
}

Event* MapScheduler::lookup(scheduler_uid_t uid) // look for event
{
#ifdef USING_UIDDEQ
  if (uid >= fuid && uid < luid) return UIDDeq[uid-fuid];
  printf("HuH?  MapScheduler::lookup, uid out of range ");
	printf(UID_PRINTF_FORMAT, uid);
	printf(" ");
	printf(UID_PRINTF_FORMAT, fuid);
	printf(" ");
	printf(UID_PRINTF_FORMAT, luid);
	printf("\n");
#else
	for (EventMap_t::const_iterator i = EventList.begin();
			 i != EventList.end(); ++i)
		{
			if (i->first.second == uid) return i->second; // Found it
		}
#endif
  return NULL;
}

Event* MapScheduler::deque()		// next event (removes from q)
{
  if (EventList.size() == 0) return NULL; // HuH?
  EventMap_t::iterator i = EventList.begin();
  Event* p = i->second;
#ifdef USING_UIDDEQ
  CleanUID();
  if (UIDDeq.size() == 0)
    { 
      printf("HuH? MapScheduler::deque, empty uid list\n");
    }
  else
    {
      if (i->second->uid_ != UIDDeq[0]->uid_)
        {
          if (i->second->uid_ >= fuid && i->second->uid_ < luid)
            { // Not in order, just null it out
							UIDDeq[i->second->uid_ - fuid] = NULL;
            }
          else
            {
              printf("HuH? MapScheduler::deque, uid %ld outofrange %ld %ld\n",
                     (unsigned long) i->second->uid_,
                     (unsigned long) fuid,
                     (unsigned long) luid);
            }
        }
      else
        { // Is head of list, just remove it
          UIDDeq.pop_front(); // Remove
					fuid++;
        }
    }
#endif
  EventList.erase(i);
	hint = EventList.end();
  totrm++;
  return p;
}

void MapScheduler::CleanUID()
{
#ifdef USING_UIDDEQ
  while(UIDDeq.size() > 0 && UIDDeq[0] == NULL)
    { // Remove null entries
      UIDDeq.pop_front();
      fuid++;
    }
#endif
}

void MapScheduler::DBDump(const char* pMsg)
{
  EventMap_t::const_iterator i;
	if (pMsg) printf(pMsg);
	printf("Dumping event list\n");
	for (i = EventList.begin(); i != EventList.end(); ++i)
		{
			printf("Time %f/%f uid ", i->first.first, i->second->time_ );
			printf(UID_PRINTF_FORMAT, i->second->uid_);
			printf(" e %p\n", i->second);
			//Event* e = lookup(i->second->uid_);
			//if (e != i->second) printf("Event mismatch, %p %p\n", e, i->second);
		}
}


#endif // HAVE_STL

/*
 * Copyright (c) 1997 Regents of the University of California.
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
 * 	This product includes software developed by the MASH Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
 * This file contributed by Dante De Lucia <dante@valhalla.internet-cafe.com>,
 * Feb 1997.
 */

#ifndef lint
static char rcsid[] =
"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/log.cc,v 1.1 1997/02/23 06:05:51 mccanne Exp $ (ANS)";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "agent.h"
#include "log.h"
#include "xmalloc.h"

/*
 * tcl command interface
 */



#define current_time       Scheduler::instance().clock()

#ifdef NEVER
static class Log_Class : public TclClass {
public:
	Log_Class() : TclClass("log") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new Log());
	}
} class_log;
#endif

Log::Log() : channel_(0), callback_(0), masks(0), mask (0)
{
}

Log::~Log()
{
}

//
// Add a mask to the mask table
//
void
Log::addmask (char *name, int mask_value)
{
  if (masks == NULL)
    {
      masks_max = 10;
      masks_count = 0;
      masks = (mask_entry **)
	xmalloc (masks_max * (sizeof (mask_entry *) + 1));

      masks[0] = NULL;
    }

  if ((masks_count + 1) >= masks_max)
    {
      masks_max += 4;
      masks = (mask_entry **) 
	xrealloc (masks, masks_max * (sizeof (mask_entry *) + 1));
    }
  masks[masks_count] = (mask_entry *) xmalloc (sizeof (mask_entry));
  masks[masks_count]->name = strdup (name);
  masks[masks_count]->mask = mask_value;
  masks[++masks_count] = NULL;
}

int
Log::lookup_mask (char *name)
{
  int i;

  for (i = 0; masks[i]; i++)
    if (strcmp (masks[i]->name, name) == 0)
      return (masks[i]->mask);
  return (0);
}


/*
 * $log detach
 * $log flush
 * $log attach $fileID
 * $log callback $proc
 * $log trace name [ name ]
 * $log mask name value
 */
int Log::command(int argc, const char*const* argv)
{
  Tcl& tcl = Tcl::instance();
  if (strcmp(argv[1], "trace") == 0) 
    {
      int i;
      for (i = 2; i < argc; i++)
	{
	  int mask_to_add = lookup_mask ((char *) argv[i]);
	  if (mask_to_add == 0)
	    {
	      tcl.resultf("Log trace: can't find mask %s", argv[i]);
	      return (TCL_ERROR);
	    }
	  mask |= mask_to_add;
	}
      return (TCL_OK);
    }
  if (strcmp(argv[1], "untrace") == 0) 
    {
      int i;
      for (i = 2; i < argc; i++)
	{
	  int mask_to_add;
	  mask_to_add = lookup_mask ((char *)argv[i]);
	  if (mask_to_add == 0)
	    {
	      tcl.resultf("Log untrace: can't find mask %s", argv[i]);
	      return (TCL_ERROR);
	    }
	  mask &= ~mask_to_add;
	}
      return (TCL_OK);
    }
  else if (argc == 2) 
    {
      if (strcmp(argv[1], "detach") == 0) 
	{
	  channel_ = 0;
	  return (TCL_OK);
	}
      if (strcmp(argv[1], "flush") == 0) 
	{
	  Tcl_Flush(channel_);
	  return (TCL_OK);
	}
    } 
  else if (argc == 3) 
    {
      if (strcmp(argv[1], "attach") == 0) {
	int mode;
	const char* id = argv[2];
	channel_ = Tcl_GetChannel(tcl.interp(), (char*)id,
				  &mode);
	if (channel_ == 0) 
	  {
	    tcl.resultf("log: can't attach %s for writing", id);
	    return (TCL_ERROR);
	  }
	return (TCL_OK);
      }
      if (strcmp(argv[1], "callback") == 0) 
	{
	  const char* cb = argv[2];
	  delete callback_;
	  if (*cb == 0)
	    callback_ = 0;
	  else 
	    {
	      callback_ = new char[strlen(cb) + 1];
	      strcpy(callback_, cb);
	    }
	  return (TCL_OK);
	}
    }
  else if (argc == 4)
    {
      addmask ((char *)argv[2], atoi (argv[3]));
    }

  tcl.resultf("ns log object: no such command %s", argv[1]);
  return (TCL_ERROR);
}

void
Log::agent (Agent *agent, int type, char *format, ...)
{
  va_list args;
  int len;

  if (!(type & mask))
    return;

  sprintf (wrk_, "%f 0x%x:", current_time, agent->addr_);
  len = strlen (wrk_);

  va_start (args, format);
  vsprintf (&wrk_[len], format, args);
  dump ();
}


void
Log::dump()
{
  if (channel_ != 0) 
    {
      /*
       * tack on a newline (temporarily) instead
       * of doing two writes
       */
      int n = strlen(wrk_);
      //      wrk_[n] = '\n';
      //      wrk_[n + 1] = 0;
      //      (void)Tcl_Write(channel_, wrk_, n + 1);
      (void)Tcl_Write(channel_, wrk_, n);
      //      wrk_[n] = 0;
    }
  if (callback_ != 0) {
    Tcl& tcl = Tcl::instance();
    tcl.evalf("%s { %s }", callback_, wrk_);
  }
}

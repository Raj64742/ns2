/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * mem-trace.h
 * Copyright (C) 1998 by USC/ISI
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
 */

#include "config.h"
#include <stdio.h>

/* Unix platforms should get these from configure */
#ifdef WIN32
#undef HAVE_GETRUSAGE
#undef HAVE_SBRK
#endif

#ifdef HAVE_GETRUSAGE
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

#ifdef HAVE_SBRK
#include <unistd.h>
#endif /* HAVE_SBRK */

/* hpux patch from Ketil Danielsen <Ketil.Danielsen@hiMolde.no> */
#ifdef hpux
#include <sys/resource.h>
#include <sys/syscall.h>
#define getrusage(a, b)  syscall(SYS_GETRUSAGE, a, b)
#endif



#define fDIFF(FIELD) (now_.FIELD - start_.FIELD)
#define absolute(var) (var > 0 ? var : -var)
#define normalize(var) var.tv_sec * 1000 + (int) (var.tv_usec / 1000)

class MemInfo {
public:
	MemInfo() {}
	long stack_;
	long heap_;
	struct timeval utime_, stime_;

	void checkpoint() {
		int i;
		stack_ = (long)&i;

#ifdef HAVE_GETRUSAGE
		struct rusage ru;
		getrusage(RUSAGE_SELF, &ru);
		utime_ = ru.ru_utime;
		stime_ = ru.ru_stime;
#else /* ! HAVE_GETRUSAGE */
		utime_.tv_sec = utime_.tv_usec = 0;
		stime_.tv_sec = stime_.tv_usec = 0;
#endif /* HAVE_GETRUSAGE */

#ifdef HAVE_SBRK
		heap_ = (long)sbrk(0);
#else /* ! HAVE_SBRK */
		heap_ = 0;
#endif /* HAVE_SBRK */
	}
};


class MemTrace {
private:
	MemInfo start_, now_;

public:
	MemTrace() {
		start_.checkpoint();
	}
	void diff(char* prompt) {
		now_.checkpoint();
		fprintf (stdout, "%s: utime/stime: %ld %ld \tstack/heap: %ld %ld\n",
			 prompt, 
			 normalize(now_.utime_) - normalize(start_.utime_), 
			 normalize(now_.stime_) - normalize(start_.stime_), 
			 fDIFF(stack_), fDIFF(heap_));
		start_.checkpoint();
	}
};


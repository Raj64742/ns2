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
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>


#define fDIFF(FIELD) (now_.FIELD - start_.FIELD)
#define absolute(var) (var > 0 ? var : -var)
#define normalize(var) var.tv_sec * 1000 + (int) (var.tv_usec / 1000)

struct MemInfo {
	long stack;
	long heap;
	struct rusage time;
};

void *cur_stack(struct MemInfo *meminfo)
{
	int i;
	meminfo->stack = (long)&i;
}

void *cur_heap(struct MemInfo *meminfo)
{
	meminfo->heap = (long)sbrk(0);
}

class MemTrace {
public:
	MemTrace() {
		(void) cur_stack(&start_);
		(void) cur_heap(&start_);
		(void) getrusage(RUSAGE_SELF, &(start_.time));
	}
	void diff(char* prompt) {
		(void) cur_stack(&now_);
		(void) cur_heap(&now_);
		(void) getrusage(RUSAGE_SELF, &(now_.time));
	fprintf (stdout, "%s: utime/stime: %d %d \tstack/heap: %d %d\n",\
		prompt, \
	normalize(now_.time.ru_utime) - normalize(start_.time.ru_utime), \
	normalize(now_.time.ru_stime) - normalize(start_.time.ru_stime), \
		fDIFF(stack), fDIFF(heap));
		(void) cur_stack(&start_);
		(void) cur_heap(&start_);
		(void) getrusage(RUSAGE_SELF, &(start_.time));
	}
private:
	struct MemInfo start_, now_;
};


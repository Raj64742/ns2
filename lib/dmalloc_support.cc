/*
 * dmalloc_support.cc
 * Copyright (C) 1997 by USC/ISI
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/lib/dmalloc_support.cc,v 1.5 2002/01/05 16:00:07 alefiyah Exp $ (USC/ISI)
 */


/*
 * Redefine new and friends to use dmalloc.
 */

#ifdef HAVE_LIBDMALLOC

/*
 * XXX dmalloc 3.x is no longer supported
 *
 * - haoboy, Aug 2000
 */

/* 
 * This portion copied from ~dmalloc/dmalloc.cc 
 * Copyright 1999 by Gray Watson
 */
extern "C" {
#include <stdlib.h>
/* Prototype declaration for TclpAlloc originally
defined in tcl8.3.2/generic/tclAlloc.c */ 
char * TclpAlloc(unsigned int);

#define DMALLOC_DISABLE

#include "dmalloc.h"
#include "return.h"
}

#ifndef DMALLOC_VERSION_MAJOR
#error DMALLOC 3.x is no longer supported.
#endif

/*
 * An overload function for the C++ new.
 */
void *
operator new[](size_t size)
{
	char	*file;
	GET_RET_ADDR(file);
	return _malloc_leap(file, 0, size);
}

/*
 * An overload function for the C++ delete.
 */
void
operator delete(void *pnt)
{
	char	*file;
	GET_RET_ADDR(file);
	_free_leap(file, 0, pnt);
}

/*
 * An overload function for the C++ delete[].  Thanks to Jens Krinke
 * <j.krinke@gmx.de>
 */
void
operator delete[](void *pnt)
{
	char	*file;
	GET_RET_ADDR(file);
	_free_leap(file, 0, pnt);
}

char *
TclpAlloc(unsigned int nbytes)
{
  char *file;
 
  GET_RET_ADDR(file);
  return (char*) _malloc_leap(file,0,nbytes);
}
 
#endif /* HAVE_LIBDMALLOC */

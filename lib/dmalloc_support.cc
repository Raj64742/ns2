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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/lib/dmalloc_support.cc,v 1.3 1999/09/18 00:51:17 haoboy Exp $ (USC/ISI)
 */


/*
 * Redefine new and friends to use dmalloc.
 */

#ifdef HAVE_LIBDMALLOC

/* 
 * NOTE: If dmalloc does not work for you (e.g., it hangs), make sure that
 * you are using dmalloc 4.2.0 or above. If you are using lower versions, 
 * change the following to:
 * 
 * #define DMALLOC_MAJOR_VERSION 3
 */ 
#define DMALLOC_MAJOR_VERSION 4

#if DMALLOC_MAJOR_VERSION > 3

/* 
 * This portion copied from ~dmalloc/dmalloc.cc 
 * Copyright 1999 by Gray Watson
 */
extern "C" {
#include <stdlib.h>

#define DMALLOC_DISABLE

#include "dmalloc.h"
#include "return.h"
}

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

#else /* DMALLOC_MAJOR_VERSION == 3 */

extern "C" {
#include <stdlib.h>
#include "dmalloc.h"
#include "return.h"
}

void *
operator new(size_t n)
{
	SET_RET_ADDR(_dmalloc_file, _dmalloc_line);
	return _malloc_leap(_dmalloc_file, _dmalloc_line, n);
}

void *
operator new[](size_t n)
{
	SET_RET_ADDR(_dmalloc_file, _dmalloc_line);
	return _malloc_leap(_dmalloc_file, _dmalloc_line, n);
}

void
operator delete(void * cp)
{
	if (!cp)   // silently ignore null pointers
		return;
	SET_RET_ADDR(_dmalloc_file, _dmalloc_line);
	_free_leap(_dmalloc_file, _dmalloc_line, cp);
}

#endif /* DMALLOC_VERSION */

#endif /* HAVE_LIBDMALLOC */

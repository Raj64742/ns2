
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/lib/dmalloc_support.cc,v 1.2 1997/07/25 20:47:12 heideman Exp $ (USC/ISI)
 */


/*
 * Redefine new and friends to use dmalloc.
 */

#ifdef HAVE_LIBDMALLOC

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

#endif /* HAVE_LIBDMALLOC */

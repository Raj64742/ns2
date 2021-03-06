// -*- C++ -*-
/*-
 * Copyright (c) 1997 The Regents of the University of California.
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
 * 3. Neither the name of the University nor of the Laboratory may be used
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
 */

/*
 * RVec is a resizing vector.
 * Automatically resize the vector (and zero-fill the unused slots)
 * when elements beyond the end are accessed.
 *
 * RVecs are zero-filled on init.
 * Vecs are garbage-filled on init.
 */

#ifndef _intRVec_h
#ifdef __GNUG__
#pragma interface
#endif
#define _intRVec_h 1

#include "int.Vec.h"


class intRVec : public intVec
{
protected:
	void			grow(const int n);
public:
				intRVec() : intVec() {};
				intRVec(int l) : intVec(l,0) {};
				intRVec(int l, int fill_value) : intVec(l,fill_value) {};
				intRVec(const intVec&v) : intVec(v) {};
				~intRVec() {};

	int&                  	operator [] (int n);
	int			viable_range(const int n);
};

inline int
intRVec::viable_range (const int n)
{
	return n >= 0 && n < len;
}

inline int&
intRVec::operator[] (int n)
{
	if (n < 0)
		range_error();
	if (n >= len)
		grow(n + 1);
	return s[n];
}

#endif

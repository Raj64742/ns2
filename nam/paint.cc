/*
 * Copyright (c) 1993 Regents of the University of California.
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
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/nam/Attic/paint.cc,v 1.2 1997/03/29 06:20:10 mccanne Exp $ (LBL)";
#endif


#include "paint.h"
#include "Tcl.h"

static class PaintClass : public TclClass {
public:
	PaintClass() : TclClass("Paint") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new Paint);
	}
} class_paint;

Paint* Paint::instance_;

Paint::Paint()
{
	Tcl& tcl = Tcl::instance();
	Tk_Window tk = tcl.tkmain();
	Tk_Uid bg = Tk_GetOption(tk, "viewBackground", "Nam");
	XColor* p = Tk_GetColor(tcl.interp(), tk, bg);
	if (p == 0)
		abort();
	background_ = p->pixel;
	XGCValues v;
	v.background = background_;
	v.foreground = background_;
	const u_long mask = GCForeground|GCBackground;

	gctab_[0] = Tk_GetGC(tk, mask, &v);
	ngc_ = 1;

	Tk_Uid black = Tk_GetUid("black");
	thick_ = lookup(black, 3);
	thin_ = lookup(black, 1);

	/*XXX*/
	instance_ = this;
}

GC Paint::text_gc(Font fid)
{
	Tcl& tcl = Tcl::instance();
	Tk_Window tk = tcl.tkmain();
	XColor* p = Tk_GetColor(tcl.interp(), tk, "black");
	if (p == 0)
		return (0);

	XGCValues v;
	v.background = background_;
	v.foreground = p->pixel;
	v.line_width = 1;
	v.font = fid;
	const u_long mask = GCFont|GCForeground|GCBackground|GCLineWidth;
	return (Tk_GetGC(tk, mask, &v));
}

int Paint::lookup(Tk_Uid colorname, int linewidth)
{
	Tcl& tcl = Tcl::instance();
	Tk_Window tk = tcl.tkmain();
	XColor* p = Tk_GetColor(tcl.interp(), tk, colorname);
	if (p == 0)
		return (-1);

	XGCValues v;
	v.background = background_;
	v.foreground = p->pixel;
	v.line_width = linewidth;
	const u_long mask = GCForeground|GCBackground|GCLineWidth;
	GC gc = Tk_GetGC(tk, mask, &v);
	int i;
	for (i = 0; i < ngc_; ++i)
		if (gctab_[i] == gc)
			return (i);
	gctab_[i] = gc;
	ngc_ = i + 1;
	return (i);
}

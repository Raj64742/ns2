/* 
 * tclAppInit.c --
 *
 *	Provides a default version of the main program and Tcl_AppInit
 *	procedure for Tcl applications (without Tk).
 *
 * Copyright (C) 2000 USC/ISI
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tclAppInit.c 1.17 96/03/26 12:45:29
 */

#include "config.h"

extern void init_misc(void);
extern EmbeddedTcl et_ns_lib;
extern EmbeddedTcl et_ns_ptypes;

/* MSVC requires this global var declaration to be outside of 'extern "C"' */
#ifdef MEMDEBUG_SIMULATIONS
#include "mem-trace.h"
MemTrace *globalMemTrace;
#endif

extern "C" {

/*
 * The following variable is a special hack that is needed in order for
 * Sun shared libraries to be used for Tcl.
 */

#ifdef TCL_TEST
EXTERN int		Tcltest_Init _ANSI_ARGS_((Tcl_Interp *interp));
#endif /* TCL_TEST */

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	This is the main program for the application.
 *
 * Results:
 *	None: Tcl_Main never returns here, so this procedure never
 *	returns either.
 *
 * Side effects:
 *	Whatever the application does.
 *
 *----------------------------------------------------------------------
 */

int
main(int argc, char **argv)
{
    Tcl_Main(argc, argv, Tcl_AppInit);
    return 0;			/* Needed only to prevent compiler warning. */
}


#if defined(linux) && defined(i386)
#ifndef HAVE_FESETPRECISION
/*
 * From:
 |  Floating-point environment <fenvwm.h>                                    |
 | Copyright (C) 1996, 1997, 1998, 1999                                      |
 |                     W. Metzenthen, 22 Parker St, Ormond, Vic 3163,        |
 |                     Australia.                                            |
 |                     E-mail   billm@melbpc.org.au                          |
 * used here with permission.
 */
#define FE_FLTPREC       0x000
#define FE_INVALIDPREC   0x100
#define FE_DBLPREC       0x200
#define FE_LDBLPREC      0x300
/*
 * From:
 * fenvwm.c
 | Copyright (C) 1999                                                        |
 |                     W. Metzenthen, 22 Parker St, Ormond, Vic 3163,        |
 |                     Australia.  E-mail   billm@melbpc.org.au              |
 * used here with permission.
 */
/*
  Set the precision to prec if it is a valid
  floating point precision macro.
  Returns 1 if precision set, 0 otherwise.
  */
int fesetprecision(int prec)
{
  unsigned short cw;
  asm volatile ("fnstcw %0":"=m" (cw));
  if ( !(prec & ~FE_LDBLPREC) && (prec != FE_INVALIDPREC) )
    {
      cw = (cw & ~FE_LDBLPREC) | (prec & FE_LDBLPREC);
      // Bug fix in next line: old "fldcw %0":"=m" is now "fldcw %0"::"m"
      // from Tom Kelly <ctk21@cam.ac.uk> who apparently has some
      // clue about gcc asm.
      asm volatile ("fldcw %0"::"m" (cw));
      return 1;
    }
  else
    return 0;
}
#endif /* !HAVE_FESETPRECISION */

/*
 * Linux i386 uses 60-bit floats for calculation,
 * not 56-bit floats, giving different results.
 * Fix that.
 *
 * See <http://www.linuxsupportline.com/~billm/faq.html>
 * for why we do this fix.
 *
 * This function is derived from wmexcep
 *
 */
void
fix_i386_linux_floats()
{
	fesetprecision(FE_DBLPREC);
}
#endif


/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *	This procedure performs application-specific initialization.
 *	Most applications, especially those that incorporate additional
 *	packages, will have their own version of this procedure.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error
 *	message in interp->result if an error occurs.
 *
 * Side effects:
 *	Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_AppInit(Tcl_Interp *interp)
{
#ifdef MEMDEBUG_SIMULATIONS
        extern MemTrace *globalMemTrace;
        globalMemTrace = new MemTrace;
#endif

#if defined(linux) && defined(i386)
	fix_i386_linux_floats();
#endif
       
	if (Tcl_Init(interp) == TCL_ERROR ||
	    Otcl_Init(interp) == TCL_ERROR)
		return TCL_ERROR;

#ifdef HAVE_LIBTCLDBG
	extern int Tcldbg_Init(Tcl_Interp *);   // hackorama
	if (Tcldbg_Init(interp) == TCL_ERROR) {
		return TCL_ERROR;
	}
#endif

	Tcl_SetVar(interp, "tcl_rcFileName", "~/.ns.tcl", TCL_GLOBAL_ONLY);
	Tcl::init(interp, "ns");
        et_ns_ptypes.load();
	et_ns_lib.load();
	init_misc();

#ifdef TCL_TEST
	if (Tcltest_Init(interp) == TCL_ERROR) {
		return TCL_ERROR;
	}
	Tcl_StaticPackage(interp, "Tcltest", Tcltest_Init,
			  (Tcl_PackageInitProc *) NULL);
#endif /* TCL_TEST */

	return TCL_OK;
}

#ifndef WIN32
void
abort()
{
	Tcl& tcl = Tcl::instance();
	tcl.evalc("[Simulator instance] flush-trace");
#ifdef abort
#undef abort
	abort();
#else
	exit(1);
#endif /*abort*/
	/*NOTREACHED*/
}
#endif

}



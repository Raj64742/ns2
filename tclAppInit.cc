/* 
 * tclAppInit.c --
 *
 *	Provides a default version of the main program and Tcl_AppInit
 *	procedure for Tcl applications (without Tk).
 *
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tclAppInit.c 1.17 96/03/26 12:45:29
 */

#include <tclcl.h>
#include "config.h"

extern void init_misc(void);
extern EmbeddedTcl et_ns_lib;

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
       
	if (Tcl_Init(interp) == TCL_ERROR ||
	    Otcl_Init(interp) == TCL_ERROR)
		return TCL_ERROR;

#ifdef HAVE_LIBTCLDBG
	extern int Dbg_Init(Tcl_Interp *);   // hackorama
	if (Dbg_Init(interp) == TCL_ERROR) {
		return TCL_ERROR;
	}
#endif

	Tcl_SetVar(interp, "tcl_rcFileName", "~/.ns.tcl", TCL_GLOBAL_ONLY);
	Tcl::init(interp, "ns");
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

/*
 * Copyright (c) 1991,1993 Regents of the University of California.
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/nam/Attic/main.cc,v 1.1 1997/03/29 04:37:57 mccanne Exp $ (LBL)";
#endif

#include <unistd.h>
#include <osfcn.h>

#include "netview.h"
#include "Tcl.h"
#include "trace.h"
#include "paint.h"
#include "state.h"

extern "C" {
#include <tk.h>
}

static void
usage()
{
	fprintf(stderr, "\
Usage: nam [X options] [-m -p -j jump -g gran -r rate] tracefile \
[layoutfile]\n");
        exit(1);
}

extern "C" char *optarg;
extern "C" int optind;

const char* disparg(int argc, const char*const* argv, const char* optstr)
{
	const char* display = 0;
	int op;
	while ((op = getopt(argc, (char**)argv, (char*)optstr)) != -1) {
		if (op == 'd') {
			display = optarg;
			break;
		}
	}
	optind = 1;
	return (display);
}
 
#include "bitmap/play.xbm"
#include "bitmap/stop.xbm"
#include "bitmap/rewind.xbm"
#include "bitmap/ff.xbm"

void loadbitmaps(Tcl_Interp* tcl)
{
	Tk_DefineBitmap(tcl, Tk_GetUid("play"),
			play_bits, play_width, play_height);
	Tk_DefineBitmap(tcl, Tk_GetUid("stop"),
			stop_bits, stop_width, stop_height);

	Tk_DefineBitmap(tcl, Tk_GetUid("rewind"),
			rewind_bits, rewind_width, rewind_height);
	Tk_DefineBitmap(tcl, Tk_GetUid("ff"),
			ff_bits, ff_width, ff_height);
}

void adios()
{
	exit(0);
}

static int cmd_adios(ClientData cd, Tcl_Interp* tcl, int argc, char **argv)
{
	adios();
	/*NOTREACHED*/
}

extern "C" char version[];

static int cmd_version(ClientData cd, Tcl_Interp* tcl, int argc, char **argv)
{
	tcl->result = version;
	return (TCL_OK);
}

char* parse_assignment(char* cp)
{
	cp = strchr(cp, '=');
	if (cp != 0) {
		*cp = 0;
		return (cp + 1);
	} else
		return ("true");
}

static void process_geometry(Tk_Window tk, char* geomArg)
{
	/*
	 * Valid formats:
	 *   <width>x<height>[+-]<x>[+-]<y> or
	 *   <width>x<height> or
	 *   [+-]x[+-]y
	 */
	Tcl &tcl = Tcl::instance();
	const int numArgs = 4;
	char **geomArgv = new char*[numArgs];
	for (int i = 0; i < numArgs; i++)
		geomArgv[i] = new char[80];
	geomArgv[0][0] = '\0';
	sprintf(geomArgv[1], "geometry");
	sprintf(geomArgv[2], Tk_PathName(tk));
	sprintf(geomArgv[3], optarg);
	if (Tk_WmCmd(tk, tcl.interp(), numArgs, geomArgv) != TCL_OK) {
		fprintf(stderr, "nam: %s\n", tcl.result());
		exit(1);
	}
	delete geomArgv;
}

extern "C" Blt_Init(Tcl_Interp*);

extern "C" {
int
TkPlatformInit(Tcl_Interp *interp)
{
	Tcl_SetVar(interp, "tk_library", ".", TCL_GLOBAL_ONLY);
#ifndef WIN32
	extern void TkCreateXEventSource(void);
	TkCreateXEventSource();
#endif
	return (TCL_OK);
}
}

int
main(int argc, char **argv)
{
	const char* script = 0;
	const char* optstr = "d:M:j:pG:r:u:X:t:i:P:g:c:S";
	
	/*
	 * Process display option here before the rest of the options
	 * because it's needed when creating the main application window.
	 */
	const char* display = disparg(argc, argv, optstr);

	Tcl_Interp *interp = Tcl_CreateInterp();
	if (Otcl_Init(interp) == TCL_ERROR)
		abort();
	Tcl::init(interp, "nam");
	Tcl& tcl = Tcl::instance();

	tcl.evalf(display? "set argv \"-name nam -display %s\"" :
			   "set argv \"-name nam\"", display);
	Tk_Window tk = 0;
	if (Tk_Init(tcl.interp()) == TCL_OK)
		tk = Tk_MainWindow(tcl.interp());
	if (tk == 0) {
		fprintf(stderr, "nam: %s\n", interp->result);
		Tcl_DeleteInterp(interp);
		exit(1);
	}
	tcl.tkmain(tk);

	extern EmbeddedTcl et_tk, et_nam;
	et_tk.load();
	et_nam.load();

	int op;
	int cacheSize = 100;
	char* graphInput = new char[256];
	char* graphInterval = new char[256];
	char* peer = new char[256];
	char* args = new char[256];
	graphInput[0] = graphInterval[0] = peer[0] = args[0] = 0;

	while ((op = getopt(argc, (char**)argv, (char*)optstr)) != -1) {
		switch (op) {
			
		default:
			usage();

		case 'd':
			break;

/*XXX move to Tcl */
#ifdef notyet
		case 'M':
			tcl.add_option("movie", optarg);
			break;

		case 'j':
			tcl.add_option("jump", optarg);
			break;
	    
		case 'p':
			tcl.add_option("pause", "1");
			break;

		case 'G':
			tcl.add_option("granularity", optarg);
			break;

		case 'r':
			tcl.add_option("rate", optarg);
			break;

		case 'u':
			script = optarg;
			break;

		case 'X': {
			const char* value = parse_assignment(optarg);
			tcl.add_option(optarg, value);
		}
			break;
#endif

		case 'g':
			process_geometry(tk, optarg);
			break;

		case 't':
			/* Use tkgraph. Must supply tkgraph input filename. */
			sprintf(graphInput, "g=%s ", optarg);
			strcat(args, graphInput);
			break;

		case 'i':
			/*
			 * Interval value for graph updates: default is
			 * set by nam_init.
			 */
			sprintf(graphInterval, "i=%s ", optarg);
			strcat(args, graphInterval);
			break;

		case 'P':
			sprintf(peer, "p=%s ", optarg);
			strcat(args, peer);
			break;

		case 'c':
			cacheSize = atoi(optarg);
			break;

		case 'S':
			XSynchronize(Tk_Display(tk), True);
			break;
		}
	}

	if (strlen(graphInterval) && !strlen(graphInput)) {
		fprintf(stderr, "nam: missing graph input file\n");
		exit(1);
	}

	loadbitmaps(interp);

	char* tracename;		/* trace file */
	char* layoutname;		/* layout/topology file */

	/*
	 * Make sure the base name of the trace and topology files
	 * was specified.
	 */
	if (argc - optind < 1)
		usage();

	tracename = argv[optind];
	if (access(tracename, R_OK) < 0) {
		tracename = new char[strlen(argv[optind]) + 4];
		sprintf(tracename, "%s.tr", argv[optind]);
	}

	if (argc - optind > 1) {
		++optind;
		layoutname = argv[optind];
		if (access(layoutname, R_OK) < 0) {
			layoutname = new char[strlen(argv[optind]) + 5];
			sprintf(layoutname, "%s.tcl", argv[optind]);
		}
	} else {
		layoutname = new char[strlen(tracename) + 3];
		strcpy(layoutname, tracename);
		char *cp = strrchr(layoutname, '.');
		if (cp != 0)
			*cp = 0;
		strcat(layoutname, ".tcl");
	}

	tcl.CreateCommand("adios", cmd_adios);
	tcl.CreateCommand("version", cmd_version);

	tcl.EvalFile(layoutname);
	if (script != 0)
		tcl.EvalFile(script);

	Paint::init();
	State::init(cacheSize);

	tcl.evalf("nam_init %s %s", tracename, args);

	Tk_MainLoop();
    
	return (0);
}

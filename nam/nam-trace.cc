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
"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/nam/Attic/nam-trace.cc,v 1.1 1997/03/29 04:38:03 mccanne Exp $ (LBL)";
#endif

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <osfcn.h>
#include <ctype.h>

#include "nam-trace.h"
#include "state.h"

extern "C" off_t tell();
extern "C" double atof();
extern double time_atof(const char*);

static class TraceClass : public TclClass {
 public:
	TraceClass() : TclClass("NamTrace") {}
	TclObject* create(int argc, const char*const* argv);
} nam_trace_class;

TclObject* TraceClass::create(int argc, const char*const* argv)
{
	if (argc != 5)
		return (0);
	/*
	 * Open the trace file for reading and read its first line
	 * to be able to determine needed values: trace start time,
	 * trace duration, etc.
	 */
	NamTrace* tr = new NamTrace(argv[4]);
	if (!tr->valid()) {
		delete tr;
		/*XXX*/
		tr = 0;
	}
	return (tr);
}

NamTrace::NamTrace(const char *fname) : handlers_(0)
{
	strncpy(fileName_, fname, sizeof(fileName_)-1);
	fileName_[sizeof(fileName_)-1] = 0;

	int fd = open(fileName_, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "nam: ");
		perror(fileName_);
		exit(1);
	}

	/*
	 * Make sure the trace file isn't empty and that the last line
	 * is correctly formatted. Compute the time interval for the
	 * entire trace.
	 */
	findLastLine(fd);

	file_ = fdopen(fd, "r");
	if (file_ == 0) {
		perror("nam: fdopen");
		exit(1);
	}

	/*
	 * Go to the beginning of the file, read the first line and
	 * save its contents.
	 */
	rewind(0);

	/*
	 * Minimum time on the nam time slider is the time of the first
	 * trace event.
	 */
	mintime_ = nextTime();

	State::instance()->setTimes(mintime_, maxtime_);
}

int NamTrace::ReadEvent(double now, TraceEvent& e)
{
	/*
	 * If specified time is after the next event's time, read
	 * the next line and return 1. Otherwise, just return 0.
	 */
	if (pending_.time >= 0 && pending_.time < now) {
		e = pending_;
		scan();
		return (1);
	}
	return (0);
}

void
NamTrace::scan()
{
	/*
	 * Read the next line in the trace file and store its contents
	 * depending on line type.
	 */
	while (1) {
		int n;
		
		if (fgets(pending_.image, sizeof(pending_.image), file_) ==
		    0) {
			pending_.time = -1;
			return;
		}
		//++lineno_;
		
		TraceEvent& p = pending_;
		char* cp = p.image;
		p.offset = ftell(file_);
		
		switch (p.tt = *cp++) {
			
		case '#':		/* comment    */
		case '\n':		/* empty line */
			continue;
			
		case 'h':		/* hop          */
		case '+':		/* enqueue      */
		case 'd':		/* drop line    */
		case '-':		/* dequeue line */
			/* <time> <hsrc> <hdst> <size> <attr> <type> <conv> <id> */
			n = sscanf(cp, " %lg %d %d %d %d %7s %31s %d",
				   &p.time, &p.pe.src, &p.pe.dst,
				   &p.pe.pkt.size, &p.pe.pkt.attr,
				   p.pe.pkt.type, p.pe.pkt.convid,
				   &p.pe.pkt.id);
			if (n == 8)
				return;
			break;
			
		case 'v':
			/*   v <time> <tcl expr> */
			n = sscanf(cp, " %lg", &p.time);
			if (n != 1)
				break;
			for (n = strlen(p.image); --n > 0; ) {
				if (! isspace(p.image[n])) {
					p.image[n+1] = 0;
					break;
				}
			}
			while (isspace(*cp))
				++cp;
			while (*cp && !isspace(*cp))
				++cp;
			while (isspace(*cp))
				++cp;
			p.ve.str = cp - p.image;
			return;
			
		default:
			fprintf(stderr,
				"nam: unknown event at offset %d in `%s'\n",
				p.offset, fileName_);
			fprintf(stderr, "nam: `%s'\n", p.image);
			exit(1);
		}
		fprintf(stderr,
			"nam: badly formatted event at offset %d in %s\n",
			p.offset, fileName_);
		for (n = strlen(p.image); --n > 0; ) {
			if (! isspace(p.image[n])) {
				p.image[n+1] = 0;
				break;
			}
		}
		fprintf(stderr, "nam: `%s'\n", p.image);
		exit(1);
	}
	/* NOTREACHED */
}

/*
 * Go to the specified trace file's last line, verify that
 * the file isn't empty and that its last line is correctly
 * formatted. Compute the time interval for the entire trace
 * assuming it started at time 0.
 */
void NamTrace::findLastLine(int fd)
{
	char buf[257];
	struct stat stat;

	/* XXX
	 * (?)The following doesn't move the seek pointer (it's at the
	 * beginning of the file.
	 */
#if defined(__bsdi__) || defined(__FreeBSD__)
	off_t saveseek = lseek(fd, (off_t)0, SEEK_CUR);
#else
	off_t saveseek = tell(fd);
#endif
	
	if (fstat(fd, &stat) < 0) {
		perror("fstat");
		exit(1);
	}

	if (stat.st_size <= 1) {
		fprintf(stderr, "nam: empty trace file `%s'.", fileName_);
		exit(1);
	}

	/*
	 * If the file is larger than 'buf', go to the end of the file
	 * at the point just large enough to fit into 'buf'.
	 */
	if (stat.st_size > sizeof(buf) - 1)
		if (lseek(fd, stat.st_size + 1 - sizeof(buf), SEEK_SET) < 0) {
			perror("lseek");
			exit(1);
		}

	int n = read(fd, buf, sizeof(buf) - 1);
	if (n < 0) {
		perror("read");
		exit(1);
	}
	buf[n] = 0;

	/* Error if the last line doesn't have '\n' in it. */
	char *cp = strrchr(buf, '\n');
	if (cp == 0) {
		fprintf(stderr, "nam: error reading last line of `%s'.",
			fileName_);
		exit(1);
	}

	/*
	 * Look for the first '\n' and check if the line following
	 * it has the correct type, i.e., it starts with any of the
	 * characters in [h+-dv].
	 */
	*cp = 0;
	cp = strrchr(buf, '\n');
	if (cp == 0 || strchr("h+-dv", *++cp) == 0) {
		fprintf(stderr, "nam: error reading last line of `%s'.",
			fileName_);
		exit(1);
	}
	/*
	 * Compute the time interval from the beginning of the trace
	 * to the end.
	 * XXX this should include the duration of the last event but
	 *     since we changed the format of 'size' from 'time' to
	 *     'bytes', we can no longer figure this out.
	 */
	double time = 0.;
	(void)sscanf(++cp, " %lg", &time);
	maxtime_ = time;
}

/* Go to the beginning of the file, read the first line and process it. */
void NamTrace::rewind(long offset)
{
	fseek(file_, offset, 0);
	//lineno_ = 0;
	scan();
}

/*
 * Put the specified trace handler to the beginning of the trace 
 * handler list.
 */
void NamTrace::addHandler(TraceHandler* th)
{
	TraceHandlerList* p = new TraceHandlerList;
	p->th = th;
	p->next = handlers_;
	handlers_ = p;
}

/* Set the current trace time to 'now'. */
void NamTrace::settime(double now, int timeSliderClicked)
{
	if (now < now_) {
		for (TraceHandlerList* p = handlers_; p != 0; p = p->next)
			p->th->reset(now);
		StateInfo si;
		State::instance()->get(now, si);
		rewind(si.offset);
	}

	now_ = now;

	/*
	 * Scan the trace file until the event occurring at time 'now'
	 * is read. For each event scanned, pass it on to all trace
	 * handlers in the trace handler list so they can handle
	 * them as needed.
	 */
	Tcl& tcl = Tcl::instance();
	Tcl_Interp* interp = tcl.interp();
	static char event[128];
	TraceEvent e;
	while (ReadEvent(now, e)) {
		for (TraceHandlerList* p = handlers_; p != 0; p = p->next)
			p->th->handle(e, now);
		if (e.tt == 'h' && (e.pe.src == 0 || e.pe.dst == 0)) {
			sprintf(event, "%d %d %.6f %d/", e.pe.src,
				e.pe.dst, e.time, e.pe.pkt.id);
			if (timeSliderClicked)
				tcl.result((const char *)event);
			else
				Tcl_AppendResult(interp, event, 0);
		}
	}

	for (TraceHandlerList* p = handlers_; p != 0; p = p->next)
		p->th->update(now);
}

/*
 * Process 'nam trace' commands:
 *   mintime - return the trace start time
 *   maxtime - return the trace duration
 *   connect - add the specified trace handler to the trace handler list
 *   settime - reset current time to specified time and adjust the
 *             display as needed
 *   nxtevent - read next event in trace file and animate from that point
 */
int NamTrace::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (argc == 2) {
		if (strcmp(argv[1], "mintime") == 0) {
			char* bp = tcl.buffer();
			sprintf(bp, "%g", mintime_);
			tcl.result(bp);
			return (TCL_OK);
		} else if (strcmp(argv[1], "maxtime") == 0) {
			char* bp = tcl.buffer();
			sprintf(bp, "%g", maxtime_);
			tcl.result(bp);
			return (TCL_OK);
		} else if (strcmp(argv[1], "nxtevent") == 0) {
			char* bp = tcl.buffer();
			sprintf(bp, "%g", pending_.time);
			tcl.result(bp);
			return (TCL_OK);
		}
	}
	if (argc == 3) {
		if (strcmp(argv[1], "connect") == 0) {
			/*
			 * Get the handler for the specified trace and
			 * add it to the trace's list of handlers.
			 */
			TraceHandler* th = (TraceHandler*)TclObject::lookup(argv[2]);
			if (th == 0) {
				tcl.resultf("nam connect: no such trace %s",
					    argv[2]);
				return (TCL_ERROR);
			}
			addHandler(th);
			return (TCL_OK);
		}
	}
	if (argc == 4 ) {
		if (strcmp(argv[1], "settime") == 0) {
			/*
			 * Set current time to the specified time and update
			 * the display to reflect the new current time.
			 */
			double now = atof(argv[2]);
			int timeSliderClicked = atoi(argv[3]);
			settime(now, timeSliderClicked);
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}

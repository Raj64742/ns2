// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/process.cc,v 1.1 1999/03/04 02:22:42 haoboy Exp $

#include "process.h"

static class ProcessClass : public TclClass {
 public:
	ProcessClass() : TclClass("Process") {}
	TclObject* create(int, const char*const*) {
		return (new Process);
	}
} class_process;

void Process::process_data(int, char*)
{
	abort();
}

AppData* Process::get_data(int&, const AppData*)
{
	abort();
	/* NOTREACHED */
}

int Process::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (strcmp(argv[1], "target") == 0) {
		if (argc == 2) {
			tcl.resultf("%s", target()->name());
			return TCL_OK;
		} else if (argc == 3) { 
			Process *p = (Process *)TclObject::lookup(argv[2]);
			if (p == NULL) {
				fprintf(stderr, "Non-existent media app %s\n",
					argv[2]);
				abort();
			}
			target() = p;
			return TCL_OK;
		}
	}
	return TclObject::command(argc, argv);
}

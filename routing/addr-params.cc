// Address parameters. Singleton class
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/routing/addr-params.cc,v 1.1 2000/09/14 18:22:59 haoboy Exp $

#include <stdlib.h>
#include <tclcl.h>

#include "addr-params.h"

AddrParamsClass* AddrParamsClass::instance_ = NULL;

static AddrParamsClass addr_params_class;

void AddrParamsClass::bind()
{
	TclClass::bind();
	add_method("McastShift");
	add_method("McastMask");
	add_method("NodeShift");
	add_method("NodeMask");
	add_method("PortMask");
	add_method("PortShift");
	add_method("hlevel");
	add_method("nodebits");
}

int AddrParamsClass::method(int ac, const char*const* av)
{
	Tcl& tcl = Tcl::instance();
	int argc = ac - 2;
	const char*const* argv = av + 2;

	if (argc == 2) {
		if (strcmp(argv[1], "McastShift") == 0) {
			tcl.resultf("%d", McastShift_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "McastMask") == 0) {
			tcl.resultf("%d", McastMask_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "PortShift") == 0) {
			tcl.resultf("%d", PortShift_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "PortMask") == 0) {
			tcl.resultf("%d", PortMask_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "hlevel") == 0) {
			tcl.resultf("%d", hlevel_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "nodebits") == 0) {
			tcl.resultf("%d", nodebits_);
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "McastShift") == 0) {
			McastShift_ = atoi(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "McastMask") == 0) {
			McastMask_ = atoi(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "PortShift") == 0) {
			PortShift_ = atoi(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "PortMask") == 0) {
			PortMask_ = atoi(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "hlevel") == 0) {
			hlevel_ = atoi(argv[2]);
			if (NodeMask_ != NULL)
				delete []NodeMask_;
			if (NodeShift_ != NULL)
				delete []NodeShift_;
			NodeMask_ = new int[hlevel_];
			NodeShift_ = new int[hlevel_];
			return (TCL_OK);
		} else if (strcmp(argv[1], "nodebits") == 0) {
			nodebits_ = atoi(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "NodeShift") == 0) {
			tcl.resultf("%d", node_shift(atoi(argv[2])-1));
			return (TCL_OK);
		} else if (strcmp(argv[1], "NodeMask") == 0) {
			tcl.resultf("%d", node_mask(atoi(argv[2])-1));
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "NodeShift") == 0) {
			NodeShift_[atoi(argv[2])-1] = atoi(argv[3]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "NodeMask") == 0) {
			NodeMask_[atoi(argv[2])-1] = atoi(argv[3]);
			return (TCL_OK);
		}
	}
	return TclClass::method(ac, av);
}

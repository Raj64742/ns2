// Hierarchical classifier: a wrapper for hierarchical routing
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/classifier-hier.cc,v 1.1 2000/09/14 18:19:25 haoboy Exp $

#include <assert.h>
#include "classifier.h"
#include "addr-params.h"

class HierClassifier : public Classifier {
public:
	HierClassifier() : Classifier() {
		clsfr_ = new Classifier*[AddrParamsClass::instance().hlevel()];
	}
	virtual ~HierClassifier() {
		// Deletion of contents (classifiers) is done in otcl
		delete []clsfr_;
	}
	inline virtual void recv(Packet *p, Handler *h) {
		clsfr_[0]->recv(p, h);
	}
	virtual int command(int argc, const char*const* argv);
private:
	Classifier **clsfr_;
};

int HierClassifier::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "classifier") == 0) {
			// XXX Caller must guarantee that the supplied level
			// is within range, i.e., 0 < n <= level_
			tcl.resultf("%s", clsfr_[atoi(argv[2])-1]->name());
			return TCL_OK;
		} else if (strcmp(argv[1], "defaulttarget") == 0) {
			NsObject *o = (NsObject *)TclObject::lookup(argv[2]);
			for (int i = 0; 
			     i < AddrParamsClass::instance().hlevel(); i++)
				clsfr_[i]->set_default_target(o);
			return (TCL_OK);
		} else if (strcmp(argv[1], "clear") == 0) {
			int slot = atoi(argv[2]);
			for (int i = 0; 
			     i < AddrParamsClass::instance().hlevel(); i++)
				clsfr_[i]->clear(slot);
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "add-classifier") == 0) {
			int n = atoi(argv[2]) - 1;
			Classifier *c=(Classifier*)TclObject::lookup(argv[3]);
			clsfr_[n] = c;
			return (TCL_OK);
		}
	}
	return Classifier::command(argc, argv);
}


static class HierClassifierClass : public TclClass {
public:
	HierClassifierClass() : TclClass("Classifier/Hier") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new HierClassifier());
	}
} class_hier_classifier;

#include "classifier.h"

class MultiPathForwarder : public Classifier {
public:
	MultiPathForwarder() : ns_(0), Classifier() {} 
	virtual int classify(Packet* const) {
		int cl;
		int fail = ns_;
		do {
			cl = ns_++;
			ns_ %= (maxslot_ + 1);
		} while (slot_[cl] == 0 && ns_ != fail);
		return cl;
	}
private:
	int ns_;
};

static class MultiPathClass : public TclClass {
public:
	MultiPathClass() : TclClass("Classifier/MultiPath") {} 
	TclObject* create(int, const char*const*) {
		return (new MultiPathForwarder());
	}
} class_multipath;

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /usr/src/mash/repository/vint/ns-2/filter.cc ";
#endif

#include "packet.h"
#include "filter.h"

static class FilterClass : public TclClass {
public:
	FilterClass() : TclClass("Filter") {}
	TclObject* create(int, const char*const*) {
		return (new Filter);
	}
} class_filter;

Filter::Filter() : filter_target_(0) 
{
}
Filter::filter_e Filter::filter(Packet* p) 
{
	return PASS;  // As simple connector
}
void Filter::recv(Packet* p, Handler* h)
{
	switch(filter(p)) {
	case DROP : 
		if (h) h->handle(p);
		drop(p);
		break;
	case DUPLICATE :
		if (filter_target_)
			filter_target_->recv(p->copy(), h);
		/* fallthrough */
	case PASS :
		send(p, h);
		break;
	case FILTER :
		if (filter_target_)
			filter_target_->recv(p, h);
		break;
	}
}
int Filter::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	/*XXX*/
	if (argc == 2) {
		if (strcmp(argv[1], "filter-target") == 0) {
			if (filter_target_ != 0)
				tcl.result(target_->name());
			return TCL_OK;
		}
	}
	else if (argc == 3) {
		if (strcmp(argv[1], "filter-target") == 0) {
			filter_target_ = (NsObject*)TclObject::lookup(argv[2]);
			return TCL_OK;
		}
	}
	return Connector::command(argc, argv);
}

static class FieldFilterClass : public TclClass {
public:
	FieldFilterClass() : TclClass("Filter/Field") {}
	TclObject* create(int, const char*const*) {
		return (new FieldFilter);
	}
} class_filter_field;

FieldFilter::FieldFilter() 
{
	bind("offset_", &offset_);
	bind("match_", &match_);
}
Filter::filter_e FieldFilter::filter(Packet *p) 
{
	return (*(int *)p->access(offset_) == match_) ? FILTER : PASS;
}


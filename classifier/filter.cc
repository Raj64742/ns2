/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * filter.cc
 * Copyright (C) 1998 by USC/ISI
 * All rights reserved.                                            
 *                                                                
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

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
Filter::filter_e Filter::filter(Packet* /*p*/) 
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
	case DUPLIC :
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

/* 10-5-98, Polly Huang, Filters that filter on multiple fields */
static class MultiFieldFilterClass : public TclClass {
public:
	MultiFieldFilterClass() : TclClass("Filter/MultiField") {}
	TclObject* create(int, const char*const*) {
		return (new MultiFieldFilter);
	}
} class_filter_multifield;

MultiFieldFilter::MultiFieldFilter() : field_list_(0)
{

}

void MultiFieldFilter::add_field(fieldobj *p) 
{
	p->next = field_list_;
	field_list_ = p;
}

MultiFieldFilter::filter_e MultiFieldFilter::filter(Packet *p) 
{
	fieldobj* tmpfield;

	tmpfield = field_list_;
	while (tmpfield != 0) {
		if (*(int *)p->access(tmpfield->offset) == tmpfield->match)
			tmpfield = tmpfield->next;
		else 
			return (PASS);
	}
	return(FILTER);
}


int MultiFieldFilter::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
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
       else if (argc == 4) {
		if (strcmp(argv[1], "filter-field") == 0) {
			fieldobj *tmp = new fieldobj;
			tmp->offset = atoi(argv[2]);
			tmp->match = atoi(argv[3]);
			add_field(tmp);
			return TCL_OK;
		}
	}
	return Connector::command(argc, argv);
}

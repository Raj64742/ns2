// Copyright (c) 2000 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// Ported from CMU/Monarch's code, appropriate copyright applies.  


#include "flowstruct.h"

FlowTable::FlowTable(int size_) : DRTab(size_) {
	assert (size_ > 0);
	size = 0;
	maxSize = size_;
	table = new TableEntry[size_];
	counter = 0;
}

FlowTable::~FlowTable() {
	delete table;
}

TableEntry &FlowTable::operator[](int index) {
	assert(index >= 0 && index < size);
	return table[index];
}

int FlowTable::find(nsaddr_t source, nsaddr_t destination, u_int16_t flow) {
	register int i;
	for (i=size-1; i>=0; i--)
		if (table[i].sourceIP == source &&
		    table[i].destinationIP == destination &&
		    table[i].flowId == flow)
			break;

	return i;
}

int FlowTable::find(nsaddr_t source, nsaddr_t destination, const Path &route) {
	register int i;
	for (i=size-1; i>=0; i--)
		if (table[i].sourceIP == source &&
		    table[i].destinationIP == destination &&
		    table[i].sourceRoute == route)
			break;

	return i;
}

void FlowTable::grow() {
	assert(0);
	TableEntry *temp;
	if (!(temp = new TableEntry[maxSize*2]))
		return;
	bcopy(table, temp, sizeof(TableEntry)*maxSize);
	delete table;
	maxSize *= 2;
	table = temp;
}

u_int16_t FlowTable::generateNextFlowId(nsaddr_t destination, bool allowDefault) {
	if ((counter&1)^allowDefault) // make sure parity is correct
		counter++;

	assert((counter & 1) == allowDefault);
	return counter++;
}

int FlowTable::createEntry(nsaddr_t source, nsaddr_t destination, 
			   u_int16_t flow) {
	if (find(source, destination, flow) != -1)
		return -1;
	if (size == maxSize)
		grow();
	if (size == maxSize)
		return -1;

	table[size].sourceIP = source;
	table[size].destinationIP = destination;
	table[size].flowId = flow;

	if (flow & 1)
		DRTab.insert(source, destination, flow);

	return size++;
}

void FlowTable::noticeDeadLink(const ID &from, const ID &to) {
	double now = Scheduler::instance().clock();

	for (int i=0; i<size; i++)
		if (table[i].timeout >= now && table[i].sourceIP == net_addr)
			for (int n=0; n < (table[i].sourceRoute.length()-1); n++)
				if (table[i].sourceRoute[n] == from &&
				    table[i].sourceRoute[n+1] == to) {
					table[i].timeout = now - 1;
					// XXX ych rediscover??? 5/2/01
				}
}

// if ent represents a default flow, bad things are going down and we need
// to rid the default flow table of them.
static void checkDefaultFlow(DRTable &DRTab, const TableEntry &ent) {
	u_int16_t flow;
	if (!DRTab.find(ent.sourceIP, ent.destinationIP, flow))
		return;
	if (flow == ent.flowId)
		DRTab.flush(ent.sourceIP, ent.destinationIP);
}

void FlowTable::cleanup() {
	int front, back;
	double now = Scheduler::instance().clock();

	return; // it's messing up path orders...

	// init front to the first expired entry
	for (front=0; (front<size) && (table[front].timeout >= now); front++)
		;

	// init back to the last unexpired entry
	for (back = size-1; (front<back) && (table[back].timeout < now); back--)
		checkDefaultFlow(DRTab, table[back]);

	while (front < back) {
		checkDefaultFlow(DRTab, table[front]);
		bcopy(table+back, table+front, sizeof(TableEntry)); // swap
		back--;

		// next expired entry
		while ((front<back) && (table[front].timeout >= now))
			front++;
		while ((front<back) && (table[back].timeout < now)) {
			checkDefaultFlow(DRTab, table[back]);
			back--;
		}
	}

	size = back+1;
}

void FlowTable::setNetAddr(nsaddr_t net_id) {
	net_addr = net_id;
}

bool FlowTable::defaultFlow(nsaddr_t source, nsaddr_t destination, 
			    u_int16_t &flow) {
	return DRTab.find(source, destination, flow);
}

DRTable::DRTable(int size_) {
	assert (size_ > 0);
	size = 0;
	maxSize = size_;
	table = new DRTabEnt[size_];
}

DRTable::~DRTable() {
	delete table;
}

bool DRTable::find(nsaddr_t src, nsaddr_t dst, u_int16_t &flow) {
	for (int i = 0; i < size; i++)
		if (src == table[i].src && dst == table[i].dst) {
			flow = table[i].fid;
			return true;
		}
	return false;
}

void DRTable::grow() {
	assert(0);
	DRTabEnt *temp;
	if (!(temp = new DRTabEnt[maxSize*2]))
		return;
	bcopy(table, temp, sizeof(DRTabEnt)*maxSize);
	delete table;
	maxSize *= 2;
	table = temp;
}

void DRTable::insert(nsaddr_t src, nsaddr_t dst, u_int16_t flow) {
	assert(flow & 1);
	for (int i = 0; i < size; i++) {
		if (src == table[i].src && dst == table[i].dst) {
			if ((short)((flow) - (table[i].fid)) > 0) {
				table[i].fid = flow;
			} else {
			}
			return;
		}
	}

	if (size == maxSize)
		grow();

	assert(size != maxSize);

	table[size].src = src;
	table[size].dst = dst;
	table[size].fid = flow;
	size++;
}

void DRTable::flush(nsaddr_t src, nsaddr_t dst) {
	for (int i = 0; i < size; i++)
		if (src == table[i].src && dst == table[i].dst) {
			table[i].src = table[size-1].src;
			table[i].dst = table[size-1].dst;
			table[i].fid = table[size-1].fid;
			size--;
			return;
		}
	assert(0);
}

ARSTable::ARSTable(int size_) {
	size = size_;
	victim = 0;
	table = new ARSTabEnt[size_];
	bzero(table, sizeof(ARSTabEnt)*size_);
}

ARSTable::~ARSTable() {
	delete table;
}

void ARSTable::insert(int uid, u_int16_t fid, int hopsEarly) {
	int i = victim;
	assert(hopsEarly);
	
	do {
		if (!table[i].hopsEarly)
			break; // we found a victim
		i = (i+1)%size;
	} while (i != victim);

	if (table[i].hopsEarly) // full. need extreme measures.
		victim = (victim+1)%size;

	table[i].hopsEarly = hopsEarly;
	table[i].uid       = uid;
	table[i].fid	   = fid;
}

int ARSTable::findAndClear(int uid, u_int16_t fid) {
	int i, retval;

	for (i=0; i<size; i++)
		if (table[i].hopsEarly && table[i].uid == uid)
			break;

	if (i == size)
		return 0;

	if (table[i].fid == fid)
		retval = table[i].hopsEarly;

	table[i].hopsEarly = 0;
	return retval;
}

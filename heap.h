/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * heap.h
 * Copyright (C) 1997 by USC/ISI
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
 * 
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/heap.h,v 1.4 1998/06/27 01:23:56 gnguyen Exp $ (USC/ISI)
 */

#ifndef ns_heap_h
#define	ns_heap_h

#define	HEAP_DEFAULT_SIZE	32

// This code has a long and rich history.  It was stolen from the MaRS-2.0
// distribution, that had in its turn acquired it from the NetSim
// distribution (or so I have been told).  It has been customised for
// event queue handling.  I, Kannan, added prolific comments to it in order
// to better understand the data structure for myself, and turned into
// a standalone C++ class that you see in this version now.

typedef double heap_key_t;

// This code has (atleast?) one flaw:  It does not check for memory allocated,
// especially in the constructor, and in heap_insert() when trying
// to resize the h_elems[] array.

class Heap 
{
	struct Heap_elem {
		heap_key_t he_key;
		void*		he_elem;
	} *h_elems;
	unsigned int	h_size;
	unsigned int	h_maxsize;
	unsigned int	h_iter;

	unsigned int	parent(unsigned int i)	{ return ((i - 1) / 2); }
	unsigned int	left(unsigned int i)	{ return ((i * 2) + 1); }
	unsigned int	right(unsigned int i)	{ return ((i + 1) * 2); }
	void swap(unsigned int i, unsigned int j) {
		// swap elems[i] with elems[j] in this
		Heap_elem __he = h_elems[i];
		h_elems[i] = h_elems[j];
		h_elems[j] = __he;
		return;
	};
	unsigned int	KEY_LESS_THAN(heap_key_t k1, heap_key_t k2) {
		return (k1 < k2);
	};

public:
	Heap(int size =HEAP_DEFAULT_SIZE)
		: h_size(0), h_maxsize(size), h_iter(0) {
		h_elems = new Heap_elem[h_maxsize];
		for (unsigned int i = 0; i < h_maxsize; i++)
			h_elems[i].he_elem = 0;
	};
	~Heap() {
		delete h_elems;
	};

	/*
	 * int	heap_member(Heap *h, void *elem):		O(n) algorithm.
	 *
	 *	Returns index(elem \in h->he_elems[]) + 1,
	 *			if elem \in h->he_elems[],
	 *		0,	otherwise.
	 *	External callers should just test for zero, or non-zero.
	 *	heap_delete() uses this routine to find an element in the heap.
	 */
	int	heap_member(void* elem) {
		unsigned int i;
		Heap_elem* he;
		for (i = 0, he = h_elems; i < h_size; i++, he++)
			if (he->he_elem == elem)
				return ++i;
		return 0;
	};

	/*
	 * int	heap_delete(Heap *h, void *elem):		O(n) algorithm
	 *
	 *	Returns 1 for success, 0 otherwise.
	 *
	 * find elem in h->h_elems[] using heap_member()
	 *
	 * To actually remove the element from the tree, first swap it to the
	 * root (similar to the procedure applied when inserting a new
	 * element, but no key comparisons--just get it to the root).
	 *
	 * Then call heap_extract_min() to remove it & fix the tree.
	 * 	This process is not the most efficient, but we do not
	 *	particularily care about how fast heap_delete() is.
	 *	Besides, heap_member() is already O(n), 
	 *	and is the dominating cost.
	 *
	 * Actually remove the element by calling heap_extract_min().
	 * 	The key that is now at the root is not necessarily the
	 *	minimum, but heap_extract_min() does not care--it just
	 *	removes the root.
	 */
	int heap_delete(void* elem) {
		int	i;

		if ((i = heap_member(elem)) == 0)
			return 0;

		for (--i; i; i = parent(i)) {
			swap(i, parent(i));
		}
		(void) heap_extract_min();
		return 1;
	};

	/*
	 * Couple of functions to support iterating through all things on the
	 * heap without having to know what a heap looks like.  To be used as
	 * follows:
	 *
	 *	for (elem = heap_iter_init(h); elem; elem = heap_iter(h))
	 *		Do whatever to the elem.
	 *
	 * You cannot use heap_iter to do anything but inspect the heap.  If
	 * heap_insert(), heap_extract_min(), or heap_delete() are called
	 * while iterating through the heap, all bets are off.
	 *
	 * Tue Nov 14 20:17:59 PST 1995 -- kva
	 *	Actually now, heap_create() initialises h_iter correctly,
	 *	and heap_iter() correctly resets h_iter
	 *	when the heap is traversed, So, we do not really need
	 *	heap_iter_init() anymore, except to break off a current
	 *	iteration and restart.
	 */
	void*	heap_iter_init() {
		h_iter = 1;
		return heap_min();
	};
	void*	heap_iter() {
		if (h_iter >= h_size) {
			h_iter = 0;
			return 0;
		} else {
			return h_elems[h_iter++].he_elem;
		}
	};

	/*
	 * void	heap_insert(Heap *h, heap_key_t *key, void *elem)
	 *
	 * Insert <key, elem> into heap h.
	 * Adjust heap_size if we hit the limit.
	 * 
	 *	i := heap_size(h)
	 *	heap_size := heap_size + 1
	 *	while (i > 0 and key < h[Parent(i)])
	 *	do	h[i] := h[Parent(i)]
	 *		i := Parent(i)
	 *	h[i] := key
	 */
	void heap_insert(heap_key_t key, void* elem) {
		unsigned int	i, par;

		if (h_maxsize == h_size) {	/* Adjust heap_size */
			unsigned int osize = h_maxsize;
			Heap_elem *he_old = h_elems;
			h_maxsize *= 2;
			h_elems = new Heap_elem[h_maxsize];
			for (i = 0; i < osize; i++)
				h_elems[i] = he_old[i];
			delete he_old;
		}
	
		i = h_size++;
		par = parent(i);
		while ((i > 0) && (KEY_LESS_THAN(key, h_elems[par].he_key))) {
			h_elems[i] = h_elems[par];
			i = par;
			par = parent(i);
		}
		h_elems[i].he_key  = key;
		h_elems[i].he_elem = elem;
		return;
	};

	/*
	 * void *heap_min(Heap *h)
	 *
	 *	Returns the smallest element in the heap, if it exists,
	 *	NULL otherwise.   The element is still in the heap.
	 *
	 *	if (heap_size > 0)
	 *		return h[0]
	 *	else
	 *		return NULL
	 */
	void* heap_min() {
		return (h_size > 0 ? h_elems[0].he_elem : 0);
	};
			
	/*
	 * void *heap_extract_min(Heap *h)
	 *
	 *	Returns the smallest element in the heap, if it exists.
	 *	NULL otherwise.
	 *
	 *	if heap_size[h] == 0
	 *		return NULL
	 *	min := h[0]
	 *	heap_size[h] := heap_size[h] - 1   # C array indices start at 0
	 *	h[0] := h[heap_size[h]]
	 *	Heapify:
	 *		i := 0
	 *		while (i < heap_size[h])
	 *		do	l = HEAP_LEFT(i)
	 *			r = HEAP_RIGHT(i)
	 *			if (r < heap_size[h])
	 *				# right child exists =>
	 *				#       left child exists
	 *				then	if (h[l] < h[r])
	 *						then	try := l
	 *						else	try := r
	 *				else
	 *			if (l < heap_size[h])
	 *						then	try := l
	 *						else	try := i
	 *			if (h[try] < h[i])
	 *				then	HEAP_SWAP(h[i], h[try])
	 *					i := try
	 *				else	break
	 *		done
	 */
	void* heap_extract_min() {
		void*	min;				  /* return value */
		unsigned int	i;
		unsigned int	l, r, x;

		if (h_size == 0)
			return 0;

		min = h_elems[0].he_elem;
		h_elems[0] = h_elems[--h_size];
	// Heapify:
		i = 0;
		while (i < h_size) {
			l = left(i);
			r = right(i);
			if (r < h_size)
				x = (KEY_LESS_THAN(h_elems[l].he_key,
						   h_elems[r].he_key) ?
				     l : r);
			else
				x = (l < h_size ? l : i);
			if ((x != i) && (KEY_LESS_THAN(h_elems[x].he_key,
						       h_elems[i].he_key))) {
				swap(i, x);
				i = x;
			} else {
				break;
			}
		}
		return min;
	};
};

#endif /* ns_heap_h */

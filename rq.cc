/*
/*
 * Copyright (c) Intel Corporation 2001. All rights reserved.
 *
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Intel is
 * acknowledged in all documentation pertaining to any such copy or
 * derivative work. Intel grants no other licenses expressed or
 * implied. The Intel trade name should not be used in any advertising
 * without its written permission. 
 *
 * INTEL CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * software. 
 */

/*
 * Copyright (c) 1991-1997 Regents of the University of California.
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
 *
 */

#include "rq.h"

#ifdef notdef
/*
 * constructure for reassembly queue-- give it a ref
 * to the containing tcp's rcv_nxt_ field, plus
 * allow it to find off_tcp_ and off_cmn_
 */

ReassemblyQueue::ReassemblyQueue(int& nxt) :
	head_(NULL), tail_(NULL), rcv_nxt_(nxt)
{
}
#endif

/*
 * clear out reassembly queue
 */
void ReassemblyQueue::clear()
{
	top_ = NULL;	// clear stack

	seginfo * p= head_;
	while (head_) {
		p= head_;
		head_= head_->next_;
		delete(p);
	}
	head_ = tail_ = NULL;
	return;
}

/*
 * gensack() -- generate 'maxsblock' sack blocks (start/end seq pairs)
 * at specified address
 *
 * According to RFC2018, a sack block contains:
 *	left edge of block (first seq # of the block)
 *	right edge of block (seq# immed. following last seq# of the block)
 */

int
ReassemblyQueue::gensack(int *sacks, int maxsblock)
{
	seginfo *p = top_;
	int cnt = maxsblock;

	while (p && maxsblock) {
		*sacks++ = top_->startseq_;
		*sacks++ = top_->endseq_;
		--maxsblock;
		p = p->snext_;
	}
	return (cnt - maxsblock);
}

/*
 * dumplist -- print out list (for debugging)
 */

void
ReassemblyQueue::dumplist()
{

	printf("FIFO: ");
	if (head_ == NULL) {
		printf("NULL\n");
	} else {
		register seginfo* p = head_;
		while (p != NULL) {
			printf("[->%d, %d<-]",
				p->startseq_, p->endseq_);
			p = p->next_;
		}
		printf("\n");
	}

	printf("LIFO: ");
	if (top_ == NULL) {
		printf("NULL\n");
	} else {
		register seginfo* s = top_;
		while (s != NULL) {
			printf("[->%d, %d<-]",
				s->startseq_, s->endseq_);
			s = s->snext_;
		}
		printf("\n");
	}
}

/*
 *
 * add() -- add a segment to the reassembly queue
 *	this is where the real action is...
 *	add the segment to both the LIFO and FIFO
 *
 * returns the aggregate header flags covering the block
 * just inserted (for historical reasons)
 *
 * add start/end seq to reassembly queue
 * start specifies starting seq# for segment, end specifies
 * last seq# number in the segment plus one
 */

TcpFlag
ReassemblyQueue::add(TcpSeq start, TcpSeq end, TcpFlag tiflags, RqFlag rqflags)
{
	int needmerge = FALSE;
	if (head_ == NULL) {
		if (top_ != NULL) {
			fprintf(stderr, "problem: FIFO empty, LIFO not\n");
			abort();
		}

		// nobody there, just insert this one

		tail_ = head_ = top_ =  new seginfo;
		head_->prev_ = head_->next_ = head_->snext_ = NULL;
		head_->startseq_ = start;
		head_->endseq_ = end;
		head_->pflags_ = tiflags;
		head_->rqflags_ = rqflags;

		return (tiflags);
	} else {

		seginfo *p = NULL, *q = NULL, *n;

		//
		// in the code below, arrange for:
		// q: points to segment after this one
		// p: points to segment before this one

		if (start >= tail_->endseq_) {
			// at tail
			p = tail_;
			if (start == tail_->endseq_)
				needmerge = TRUE;
			goto endfast;
		}

		if (end <= head_->startseq_) {
			// at head
			q = head_;
			if (end == head_->startseq_)
				needmerge = TRUE;
			goto endfast;
		}

		// not first or last...
		// look for segment before this one, starting at end
		for (q = tail_; q && (q->startseq_ > end); q = q->prev_)
			;
		
		p = q;
		q = q->next_;

		if (p->endseq_ >= start || q->startseq_ <= end) {
			if (p->endseq_ > start || q->startseq_ < end) {
				fprintf(stderr,
				   "ReassemblyQueue::add() fault-- I don't (yet) handle repacketized segments\n");
				abort();
			}
			needmerge = TRUE;
		}

endfast:
		n = new seginfo;
		n->startseq_ = start;
		n->endseq_ = end;
		n->pflags_ = tiflags;
		n->rqflags_ = rqflags;
		n->snext_ = top_;

		n->prev_ = p;
		n->next_ = q;

		top_ = n;

		if (p)
			p->next_ = n;
		else
			head_ = n;

		if (q)
			q->prev_ = n;
		else
			tail_ = n;


		if (needmerge)
			return(coalesce(p, n, q));

		return tiflags;
	}
}

/*
 * We need to see if we can coalesce together the
 * blocks in and around the new block
 *
 * Assumes p is prev, n is new, p is after
 */
TcpFlag
ReassemblyQueue::coalesce(seginfo *p, seginfo *n, seginfo *q)
{
	TcpFlag flags = 0;

	if (p && q && p->endseq_ == n->startseq_ && n->endseq_ == q->startseq_) {
		// new block fills hole between p and q
		// delete the new block and the block after, update p
		p->next_ = q->next_;
		q->next_->prev_ = p;
		p->endseq_ = q->endseq_;
		p->pflags_ |= n->pflags_;
		delete n;
		if (tail_ == q)
			tail_ = p;
		delete q;
		flags = p->pflags_;
	} else if (p && (p->endseq_ == n->startseq_)) {
		// new block joins p, but not q
		// update p with n's seq data, delete new block
		// (note: might be at end of list)
		p->endseq_ = n->endseq_;
		p->next_ = q;
		if (q)
			q->prev_ = p;
		else
			tail_ = p;
		p->pflags_ |= n->pflags_;
		flags = p->pflags_;
		delete n;
	} else if (q && (n->endseq_ == q->startseq_)) {
		// new block joins q, but not p
		// update q with n's seq data, delete new block
		// (note: might be at beginning of list)
		q->startseq_ = n->startseq_;
		q->prev_ = p;
		if (p)
			p->next_ = q;
		else
			head_ = q;
		q->pflags_ |= n->pflags_;
		flags = q->pflags_;
		delete n;
	}
	return (flags);
}

#ifdef	RQDEBUG
main()
{
	int rcvnxt;
	ReassemblyQueue rq(rcvnxt);

	rq.add(1, 10, 0, 0);
	rq.dumplist();
	rq.add(11, 20, 0, 0);
	rq.dumplist();

	exit(0);
}
#endif

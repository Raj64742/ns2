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

/*
 * unlink a seginfo from its FIFO
 */
void
ReassemblyQueue::fremove(seginfo* p)
{
	if (p->prev_)
		p->prev_->next_ = p->next_;
	else
		head_ = p->next_;
	if (p->next_)
		p->next_->prev_ = p->prev_;
	else
		tail_ = p->prev_;
}

/*
 * unlink a seginfo from its LIFO
 */
void
ReassemblyQueue::sremove(seginfo* p)
{
	if (p->sprev_)
		p->sprev_->snext_ = p->snext_;
	else
		top_ = p->snext_;
	if (p->snext_)
		p->snext_->sprev_ = p->sprev_;
	else
		bottom_ = p->sprev_;
}

/*
 * push a seginfo on the LIFO
 */
void
ReassemblyQueue::push(seginfo *p)
{
	p->snext_ = top_;
	p->sprev_ = NULL;
	top_ = p;
	if (p->snext_)
		p->snext_->sprev_ = p;
	else
		bottom_ = p;
}

/*
 * clear out reassembly queue and stack
 */

void
ReassemblyQueue::clear()
{
	// clear stack and end of queue
	tail_ = top_ = bottom_ = NULL;

	seginfo *p = head_;
	while (head_) {
		p = head_;
		head_= head_->next_;
		delete(p);
	}
	tail_ = NULL;
	return;
}

/*
 * clear out reassembly queue (and stack) up
 * to the given sequence number
 */

void
ReassemblyQueue::clearto(TcpSeq seq)
{
	seginfo *p = head_, *q;
	while (p) {
		if (p->endseq_ <= seq) {
			q = p->next_;
			sremove(p);
			fremove(p);
			delete p;
			p = q;
		} else
			break;
	}
	return;
}

/*
 * gensack() -- generate 'maxsblock' sack blocks (start/end seq pairs)
 * at specified address
 * returns the number of blocks written into the buffer specified
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
		*sacks++ = p->startseq_;
		*sacks++ = p->endseq_;
		--maxsblock;
		p = p->snext_;
	}
	return (cnt - maxsblock);
}

/*
 * dumplist -- print out FIFO and LIFO (for debugging)
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
			if (p->rqflags_ & RQF_MARK) {
				printf("OOPS: LOOP1\n");
				abort();
			}
			printf("[->%d, %d<-]",
				p->startseq_, p->endseq_);
			p->rqflags_ |= RQF_MARK;
			p = p->next_;
		}
		printf("\n");
		p = tail_;
		while (p != NULL) {
			printf("[->%d, %d<-]",
				p->startseq_, p->endseq_);
			p = p->prev_;
		}
		printf("\n");
	}

	printf("LIFO: ");
	if (top_ == NULL) {
		printf("NULL\n");
	} else {
		register seginfo* s = top_;
		while (s != NULL) {
			if (s->rqflags_ & RQF_MARK)
				s->rqflags_ &= ~RQF_MARK;
			else {
				printf("OOPS: LOOP2\n");
				abort();
			}
			printf("[->%d, %d<-]",
				s->startseq_, s->endseq_);
			s = s->snext_;
		}
		printf("\n");
		s = bottom_;
		while (s != NULL) {
			printf("[->%d, %d<-]",
				s->startseq_, s->endseq_);
			s = s->sprev_;
		}
		printf("\n");
	}
	printf("RCVNXT: %d\n", rcv_nxt_);
	printf("\n");
	fflush(stdout);
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

		tail_ = head_ = top_ = bottom_ =  new seginfo;
		head_->prev_ = head_->next_ = head_->snext_ = head_->sprev_ = NULL;
		head_->startseq_ = start;
		head_->endseq_ = end;
		head_->pflags_ = tiflags;
		head_->rqflags_ = rqflags;

		//
		// this shouldn't really happen, but
		// do the right thing just in case
		if (rcv_nxt_ >= start)
			rcv_nxt_ = end;

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
		// look for segments before and after this one
		for (p = head_, q = p->next_; q; p = q, q = q->next_) {
			if (p->endseq_ == start || q->startseq_ == end) {
				needmerge = TRUE;
				break;
			} else if (p->endseq_ < start && q->startseq_ > end) {
				break;
			} else {
				fprintf(stderr,
				   "ReassemblyQueue::add(%d,%d,%d,%d) fault-- I don't (yet) handle repacketized segments\n",
				   	start, end, tiflags, rqflags);
				fprintf(stderr, "\tp(%d,%d), q(%d,%d)\n",
					p->startseq_, p->endseq_,
					q->startseq_, q->endseq_);
				abort();
			}
		}

endfast:
		n = new seginfo;
		n->startseq_ = start;
		n->endseq_ = end;
		n->pflags_ = tiflags;
		n->rqflags_ = rqflags;

		n->prev_ = p;
		n->next_ = q;

		push(n);

		if (p)
			p->next_ = n;
		else
			head_ = n;

		if (q)
			q->prev_ = n;
		else
			tail_ = n;


		//
		// If there is an adjacency condition,
		// call coalesce to deal with it.
		// If not, there is a chance we inserted
		// at the head at the rcv_nxt_ point.  In
		// this case we ned to update rcv_nxt_ to
		// the end of the newly-inserted segment
		//

		if (needmerge)
			return(coalesce(p, n, q));
		else if (rcv_nxt_ >= start)
			rcv_nxt_ = end;

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

#ifdef RQDEBUG
printf("coalesce(%p,%p,%p)\n", p, n, q);
printf(" [%d,%d],[%d,%d],[%d,%d]\n",
	p ? p->startseq_ : 0,
	p ? p->endseq_ : 0,
	n ? n->startseq_ : 0,
	n ? n->endseq_ : 0,
	q ? n->startseq_ : 0,
	q ? n->endseq_ : 0);
dumplist();
#endif

	if (p && q && p->endseq_ == n->startseq_ && n->endseq_ == q->startseq_) {
		// new block fills hole between p and q
		// delete the new block and the block after, update p
		sremove(n);
		fremove(n);
		sremove(q);
		fremove(q);
		p->endseq_ = q->endseq_;
		flags = (p->pflags_ |= n->pflags_);
		delete n;
		delete q;
	} else if (p && (p->endseq_ == n->startseq_)) {
		// new block joins p, but not q
		// update p with n's seq data, delete new block
		sremove(n);
		fremove(n);
		p->endseq_ = n->endseq_;
		flags = (p->pflags_ |= n->pflags_);
		delete n;
	} else if (q && (n->endseq_ == q->startseq_)) {
		// new block joins q, but not p
		// update q with n's seq data, delete new block
		sremove(n);
		fremove(n);
		q->startseq_ = n->startseq_;
		flags = (q->pflags_ |= n->pflags_);
		delete n;
		p = q;	// ensure p points to something
	}

	//
	// at this point, p points to the updated/coalesced
	// block.  If it advances the highest in-seq value,
	// update rcv_nxt_ appropriately
	//
	if (rcv_nxt_ >= p->startseq_)
		rcv_nxt_ = p->endseq_;
	return (flags);
}

#ifdef RQDEBUG
main()
{
	int rcvnxt;
	ReassemblyQueue rq(rcvnxt);

	static int blockstore[64];
	int *blocks = blockstore;
	int nblocks = 5;
	int i;

	rq.add(5, 10, 0, 0);
	rq.dumplist();
	rq.add(10, 20, 0, 0);
	rq.dumplist();
	rq.add(1, 3, 0, 0);
	rq.dumplist();
	rq.add(22, 25, 0, 0);
	rq.dumplist();
	rq.add(3, 5, 0, 0);
	rq.dumplist();

	printf("rcvnxt: %d\n", rcvnxt);
	rq.gensack(blocks, nblocks);
	for (i = 0; i < nblocks; i++) {
		printf(">%d, %d<, ", blocks[0], blocks[1]);
		++blocks;
		++blocks;
	}
	printf("\n-->clrto20\n");

	rq.clearto(20);
	rq.dumplist();

	exit(0);
}
#endif

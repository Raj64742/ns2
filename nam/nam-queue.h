/*
 * Copyright (c) 1991,1993 Regents of the University of California.
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/nam/Attic/nam-queue.h,v 1.1 1997/03/29 04:38:03 mccanne Exp $ (LBL)
 */

#ifndef nam_queue_h
#define nam_queue_h

#include "animation.h"

class NetView;
class NamDrop;
class NamQueue;

class QueueItem : public Animation {
	friend NamQueue;
 public:
        QueueItem::QueueItem(const PacketAttr&, double, long);
	void position(float&, float&);
	virtual void draw(NetView*, double) const;
	void locate(float x, float y, float dx, float dy);
	virtual int inside(double now, float px, float py) const;
	virtual const char* info() const;
	inline int id() const { return pkt_.id; }
	inline int size() const { return pkt_.size; }
	inline const char* type() const { return (pkt_.type); }
	inline const char* convid() const { return (pkt_.convid); }
 private:
	QueueItem* qnext_;
	float px_[4];
	float py_[4];
	PacketAttr pkt_;
};

class NamQueue {
 public:
	NamQueue(float angle);
	virtual void reset(double);
	QueueItem* remove(const PacketAttr& p, int& position);
	QueueItem* remove(const PacketAttr& p) { int d; return remove(p, d); }
	inline int size() const { return (cnt_); }
	void enque(QueueItem*);
	void relocate();
	void place(double psize, double x, double y);
	inline QueueItem *head() const { return head_; }

	NamQueue* next_;
 private:
	int cnt_;		/* current size of queue */
	QueueItem* head_;
	QueueItem** tail_;

	/*XXX get rid of this */
	int nb_;		/* aggregate bytes in queue */
	float psize_;		/* packet size */
	float angle_;

	float qx_, qy_;		/* coord of head of queue */
	float dx_, dy_;		/* space between packets */
	float px_, py_;		/* size of each packet */
};

#endif

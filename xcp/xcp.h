/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996-1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/xcp/xcp.h,v 1.3 2004/09/28 18:12:44 haldar Exp $ (LBL)
 */

#ifndef NS_XCP
#define NS_XCP

#include <math.h>
#include "packet.h"
#include "queue.h"
#include "xcpq.h"


#define MAX_QNUM 3

enum {XCPQ=0, TCPQ=1, OTHERQ=2};
// code points for separating XCP/TCP flows
#define CP_XCP 10
#define CP_TCP 20
#define CP_OTHER 30


class XCPWrapQ : public Queue {
 public:
  XCPWrapQ();
  int command(int argc, const char*const* argv);
  void recv(Packet*, Handler*);

 protected:
  XCPQueue** xcpq_;      // array of xcp queue 
  unsigned int    routerId_;
  
  int qToDq_;                 // current queue being dequed
  double wrrTemp_[MAX_QNUM];     // state of queue being serviced
  double queueWeight_[MAX_QNUM]; // queue weight for each queue (dynamic)
  int maxVirQ_;             // num of queues used in xcp router

  // Modified functions
  virtual void enque(Packet* pkt); // high level enque function
  virtual Packet* deque();         // high level deque function
    
  void addQueueWeights(int queueNum, int weight);
  int queueToDeque();              // returns qToDq
  int queueToEnque(int codePt);    // returns queue based on codept
  void mark(Packet *p);             // marks pkt based on flow type
  int getCodePt(Packet *p);        // returns codept in pkt hdr
  void setVirtualQueues();         // setup virtual queues(for xcp/tcp)
};

#endif //NS_XCP

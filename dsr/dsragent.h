/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
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
 */
/* Ported from CMU/Monarch's code, nov'98 -Padma.*/

/* -*- c++ -*-
   dsragent.h

   $Id: dsragent.h,v 1.4 1999/04/22 18:53:47 haldar Exp $
   */

#ifndef _DSRAgent_h
#define _DSRAgent_h

class DSRAgent;

#include <stdarg.h>

#include "object.h"
#include "agent.h"
#include "trace.h"
#include "packet.h"
#include "priqueue.h"
#include "mac.h"

#include "path.h"
#include "srpacket.h"
#include "routecache.h"
#include "requesttable.h"

#define BUFFER_CHECK 0.03	// seconds between buffer checks
#define RREQ_JITTER 0.010	// seconds to jitter broadcast route requests
#define SEND_TIMEOUT 30.0	// # seconds a packet can live in sendbuf
#define SEND_BUF_SIZE 64
#define RTREP_HOLDOFF_SIZE 10

#define GRAT_ROUTE_ERROR 0	// tell_addr indicating a grat route err

class ArpCallbackClass;
struct RtRepHoldoff {
  ID requestor;
  ID requested_dest;
  int best_length;
  int our_length;
};

struct SendBufEntry {
  Time t;			// insertion time
  SRPacket p;
};

struct GratReplyHoldDown {
  Time t;
  Path p;
};

class SendBufferTimer : public TimerHandler {
public:
  SendBufferTimer(DSRAgent *a) : TimerHandler() { a_ = a;}
  void expire(Event *e);
protected:
  DSRAgent *a_;
};

LIST_HEAD(DSRAgent_List, DSRAgent);

class DSRAgent : public Tap, public Agent {
public:

  virtual int command(int argc, const char*const* argv);
  virtual void recv(Packet*, Handler* callback = 0);

  void tap(const Packet *p);
  // tap out all data packets received at this host and promiscously snoop
  // them for interesting tidbits

  void Terminate(void);
	// called at the end of the simulation to purge all packets

  DSRAgent();
  ~DSRAgent();

private:

  Trace *logtarget;
  int off_mac_;
  int off_ll_;
  int off_ip_;
  int off_sr_;

  // will eventually need to handle multiple infs, but this is okay for
  // now 1/28/98 -dam
  ID net_id, MAC_id;		// our IP addr and MAC addr
  NsObject *ll;		        // our link layer output 
  PriQueue *ifq;		// output interface queue

	// extensions for wired cum wireless sim mode
	MobileNode *node_;
	int diff_subnet(ID dest, ID myid);

  /******** internal state ********/
  RequestTable request_table;
  RouteCache *route_cache;
  SendBufEntry send_buf[SEND_BUF_SIZE];
  SendBufferTimer send_buf_timer;
  int route_request_num;	// number for our next route_request
  int num_heldoff_rt_replies;
  RtRepHoldoff rtrep_holdoff[RTREP_HOLDOFF_SIZE]; // not used 1/27/98
  GratReplyHoldDown grat_hold[RTREP_HOLDOFF_SIZE];
  int grat_hold_victim;

  bool route_error_held; // are we holding a rt err to propagate?
  ID err_from, err_to;	 // data from the last route err sent to us 
  Time route_error_data_time; // time err data was filled in

  /****** internal helper functions ******/

  /* all handle<blah> functions either free or hand off the 
     p.pkt handed to them */
  void handlePktWithoutSR(SRPacket& p, bool retry);
  /* obtain a source route to p's destination and send it off */
  void handlePacketReceipt(SRPacket& p);
  void handleForwarding(SRPacket& p);
  void handleRouteRequest(SRPacket &p);
  /* process a route request that isn't targeted at us */

  void handleRteRequestForOutsideDomain(SRPacket& p);
  /* process route reqs for destination outside domain, incase Iam a base-stn */
  void returnSrcRteForOutsideDomainToRequestor(SRPacket &p);
  /* return rte info for outside domain dst, to src, incase Iam a base-stn */

  bool ignoreRouteRequestp(SRPacket& p);
  // assumes p is a route_request: answers true if it should be ignored.
  // does not update the request table (you have to do that yourself if
  // you want this packet ignored in the future)
  void sendOutPacketWithRoute(SRPacket& p, bool fresh, Time delay = 0.0);
  // take packet and send it out packet must a have a route in it
  // fresh determines whether route is reset first
  // time at which packet is sent is scheduled delay secs in the future
  // pkt.p is freed or handed off
  void sendOutRtReq(SRPacket &p, int max_prop = MAX_SR_LEN);
  // turn p into a route request and launch it, max_prop of request is
  // set as specified
  // p.pkt is freed or handed off
  void getRouteForPacket(SRPacket &p, ID dest, bool retry);
  /* try to obtain a route for packet
     pkt is freed or handed off as needed, unless in_buffer == true
     in which case they are not touched */
  void acceptRouteReply(SRPacket &p);
  /* - enter the packet's source route into our cache
     - see if any packets are waiting to be sent out with this source route
     - doesn't free the p.pkt */
  void returnSrcRouteToRequestor(SRPacket &p);
  // take the route in p, add us to the end of it and return the
  // route to the sender of p
  // doesn't free p.pkt
  bool replyFromRouteCache(SRPacket &p); 
  /* - see if can reply to this route request from our cache
     if so, do it and return true, otherwise, return false 
     - frees or hands off p.pkt i ff returns true */
  void processBrokenRouteError(SRPacket& p);
  // take the error packet and proccess our part of it.
  // if needed, send the remainder of the errors to the next person
  // doesn't free p.pkt
  void xmitFailed(Packet *pkt);
  /* mark our route cache reflect the failure of the link between
     srh[cur_addr] and srh[next_addr], and then create a route err
     message to send to the orginator of the pkt (srh[0]) 
     p.pkt freed or handed off */
  void undeliverablePkt(Packet *p, int mine);
  /* when we've got a packet we can't deliver, what to do with it? 
     frees or hands off p if mine = 1, doesn't hurt it otherwise */

  void dropSendBuff(SRPacket &p);
  // log p as being dropped by the sendbuffer in DSR agent
  void stickPacketInSendBuffer(SRPacket& p);
  void sendBufferCheck();
  // see if any packets in send buffer need route requests sent out
  // for them, or need to be expired

  void sendRouteShortening(SRPacket &p, int heard_at, int xmit_at);
  // p was overheard at heard_at in it's SR, but we aren't supposed to
  // get it till xmit_at, so all the nodes between heard_at and xmit_at
  // can be elided.  Send originator of p a gratuitous route reply to 
  // tell them this.

  void testinit();
  void trace(char* fmt, ...);

  friend void XmitFailureCallback(Packet *pkt, void *data);
  friend int FilterFailure(Packet *p, void *data);
  friend class SendBufferTimer;

#if 0
  void scheduleRouteReply(Time t, Packet *new_p);
  // schedule a time to send new_p if we haven't heard a better
  // answer in the mean time.  Do not modify new_p after calling this
  void snoopForRouteReplies(Time t, Packet *p);
  
friend void RouteReplyHoldoffCallback(Node *node, Time time, EventData *data);
#endif // 0

  /* the following variables are used to send end-of-sim notices to all objects */
public:
	LIST_ENTRY(DSRAgent) link;
	static DSRAgent_List agthead;
};

#endif // _DSRAgent_h

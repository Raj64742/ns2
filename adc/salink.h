/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or
 * derivative work. Xerox grants no other licenses expressed or
 * implied. The Xerox trade name should not be used in any advertising
 * without its written permission. 
 *
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * software. 
 */

#ifndef ns_signalmod_h
#define ns_signalmod_h

//no of pending flows decisions
#define NFLOWS 500

struct pending {
	int flowid;
	int status;
};

class SALink : public Connector {
public:
	SALink();
	int command(int argc, const char*const* argv); 
	void trace(TracedVar* v);
	
protected:
	void recv(Packet *,Handler *);
	ADC *adc_;
	int RTT;
	pending pending_[NFLOWS];
	int lookup(int);
	int get_nxt();
	TracedInt numfl_;
	Tcl_Channel tchan_;
	int onumfl_;  /* XXX: store previous value of numfl_ for nam */
	int src_;   /* id of node we're connected to (for nam traces) */
	int dst_;   /* id of node at end of the link */
	int last_;      /* previous ac decision on this link */
};


#endif

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

#ifndef ns_measuremod_h
#define ns_measuremod_h

class MeasureMod : public Connector {
public:
	MeasureMod();
	inline int &bitcnt() { return nbits_;}
	inline void resetbitcnt() { nbits_ = 0;}
protected:
	int nbits_ ;
	int npkts_;
	void recv(Packet *, Handler *);
};

#endif

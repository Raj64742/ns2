/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1999 Regents of the University of California.
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
 *      This product includes software developed by the MASH Research
 *      Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/satlink.h,v 1.1 1999/06/21 18:28:47 tomh Exp $
 *
 * Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
 */

#ifndef ns_satlink_h
#define ns_satlink_h

#include "node.h"
#include "channel.h"
#include "phy.h"
#include "queue.h"
#include "net-interface.h"
#include "sat.h"

#define LINK_HDRSIZE 16 

class SatLinkHead;

class SatLL : public LL {
public:
        SatLL() : LL(), arpcache_(-1), arpcachedst_(-1) {}
        virtual void sendDown(Packet* p);
        virtual void sendUp(Packet* p);
        virtual void recv(Packet* p, Handler* h);
	Channel* channel(); // Helper function used for ``ARP''
protected:
	// Optimization-- cache the last value of Mac address 
        int arpcache_;  
        int arpcachedst_;
};

class SatMac : public Mac {
public:
	SatMac() : Mac() {}
	virtual void recv(Packet* p, Handler* h);
	virtual void sendDown(Packet* p);
	virtual void sendUp(Packet *p);

protected:
	int command(int argc, const char*const* argv);
};

class SatMacRepeater : public SatMac {
public:
	SatMacRepeater() {}
	void recv(Packet* p, Handler* h);
protected:
};

/*
class PureAlohaMac : public SatMac {
public:
	PureAlohaMac : Mac() {}
	virtual void recv(Packet* p, Handler* h);
	virtual void sendDown(Packet* p);
	virtual void sendUp(Packet *p);

protected:
	int command(int argc, const char*const* argv);
};
*/

class SatPhy : public Phy {
 public:
	SatPhy() : Phy() {}
	void sendDown(Packet *p);
	int sendUp(Packet *p);
 protected:
	int command(int argc, const char*const* argv);
};

/*
 * Class SatChannel
 */
class SatChannel : public Channel{
friend class SatRouteObject;
 public:
	SatChannel(void);
	int getId() { return index_; }
	void add_interface(Phy*);
	void remove_interface(Phy*);
	int find_peer_mac_addr(int);

 protected:
	double get_pdelay(Node* tnode, Node* rnode);
 private:

};

class SatNode;
/*
 * For now, this is the API used in the satellite networking code (hopefully
 * a more general one will follow).
 */
class SatLinkHead : public LinkHead {
public:
	SatLinkHead(); 
	// This builds on the API provided by LinkHead
	// (Note: the following pointers are not part of a generic API and may 
	// be changed in the future
	SatPhy* phy_tx() { return phy_tx_; }
	SatPhy* phy_rx() { return phy_rx_; }
	SatMac* mac() { return mac_; }
	SatLL* satll() { return satll_; }
	Queue* queue() { return queue_; }
	int linkup_;
	SatNode* node() { return ((SatNode*) node_); }
	
protected:
	virtual int command(int argc, const char*const* argv); 
	SatPhy* phy_tx_;
	SatPhy* phy_rx_;
	SatMac* mac_;
	SatLL* satll_;
	Queue* queue_;

};

#endif

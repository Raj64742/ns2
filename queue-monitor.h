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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/queue-monitor.h,v 1.2 1997/04/24 03:10:39 kfall Exp $ (UCB)
 */

#include "integrator.h"
#include "connector.h"
#include "packet.h"

class QueueMonitor : public TclObject {
 public: 
        QueueMonitor() : bytesInt_(NULL), pktsInt_(NULL), delaySamp_(NULL),
	    size_(0), pkts_(0), parrivals_(0), barrivals_(0),
	    pdepartures_(0), bdepartures_(0), pdrops_(0), bdrops_(0) {
                bind("size_", &size_);
                bind("pkts_", &pkts_);
                bind("parrivals_", &parrivals_);
                bind("barrivals_", &barrivals_);
                bind("pdepartures_", &pdepartures_);
                bind("bdepartures_", &bdepartures_);
                bind("bdrops_", &bdrops_);
                bind("pdrops_", &pdrops_);
                bind("off_cmn_", &off_cmn_);
        };      
	int size() const { return (size_); }
	int pkts() const { return (pkts_); }
        virtual void in(Packet*);
        virtual void out(Packet*);
	virtual void drop(Packet*);
        virtual int command(int argc, const char*const* argv);
protected:
        Integrator  *bytesInt_;	// q-size integrator (bytes)
        Integrator  *pktsInt_;	// q-size integrator (pkts)
        Samples*	delaySamp_;	// stat samples of q delay
        int size_;	// current queue size (bytes)
        int pkts_;	// current queue size (packets)
	// aggregate counters bytes/packets
	int barrivals_;
	int parrivals_;
	int bdepartures_;
	int pdepartures_;
	int bdrops_;
	int pdrops_;
        int off_cmn_;
};   

class SnoopQueue : public Connector {
 public: 
        SnoopQueue() : qm_(0) {}
        int command(int argc, const char*const* argv) {
                if (argc == 3) { 
                        if (strcmp(argv[1], "set-monitor") == 0) {
                                qm_ = (QueueMonitor*)
                                        TclObject::lookup(argv[2]);
				if (qm_ == NULL)
					return (TCL_ERROR);
                                return (TCL_OK);
                        }
                }
                return (Connector::command(argc, argv));
        }
 protected:
        QueueMonitor* qm_;
};      
                
class SnoopQueueIn : public SnoopQueue {
 public:
        void recv(Packet* p, Handler* h) {
                qm_->in(p);
                send(p, h);
        }
};      
                
class SnoopQueueOut : public SnoopQueue {
 public:
        void recv(Packet* p, Handler* h) {
                qm_->out(p);
                send(p, h);
        }       
};

class SnoopQueueDrop : public SnoopQueue {
 public:
        void recv(Packet* p, Handler* h) {
                qm_->drop(p);
                send(p, h);
        }       
};

/*
 * a 'QueueMonitorCompat', which is used by the compat
 * code to produce the link statistics available in ns-1
 */
#define	QM_FIDMAX	13
class QueueMonitorCompat : public QueueMonitor {
public:
	QueueMonitorCompat();
        void in(Packet*);
        void out(Packet*);
        void drop(Packet*);
        int command(int argc, const char*const* argv);
protected:
	int	off_ip_;
	int	pkts_[QM_FIDMAX];
	int	bytes_[QM_FIDMAX];
	int	drops_[QM_FIDMAX];
};


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

/* god.cc

   General Operations Director

   perform operations requiring omnipotence in the simulation

   NOTE: Tcl node indexs are 0 based, NS C++ node IP addresses (and the
   node->index() are 1 based.

   $Id: god.cc,v 1.6 1999/09/09 03:22:38 salehi Exp $
   */

#include <object.h>
#include <packet.h>
#include <ip.h>

#include <god.h>

God* God::instance_;

static class GodClass : public TclClass {
public:
        GodClass() : TclClass("God") {}
        TclObject* create(int, const char*const*) {
                return (new God);
        }
} class_God;


God::God()
{
        min_hops = 0;
        num_nodes = 0;
}


int
God::hops(int i, int j)
{
        return min_hops[i * num_nodes + j];
}


void
God::stampPacket(Packet *p)
{
        hdr_cmn *ch = HDR_CMN(p);
        struct hdr_ip *ih = HDR_IP(p);
        nsaddr_t src = ih->saddr();
        nsaddr_t dst = ih->daddr();

        assert(min_hops);

        if (!packet_info.data_packet(ch->ptype())) return;

        if (dst > num_nodes || src > num_nodes) return; // broadcast pkt
   
        ch->opt_num_forwards() = min_hops[src * num_nodes + dst];
}


void 
God::recv(Packet *, Handler *)
{
        abort();
}

 
int 
God::command(int argc, const char* const* argv)
{
	Tcl& tcl = Tcl::instance(); 
	if ((instance_ == 0) || (instance_ != this))
          	instance_ = this; 
        if (argc == 2) {
                if(strcmp(argv[1], "dump") == 0) {
                        int i, j;

                        for(i = 1; i < num_nodes; i++) {
                                fprintf(stdout, "%2d) ", i);
                                for(j = 1; j < num_nodes; j++)
                                        fprintf(stdout, "%2d ",
                                                min_hops[i * num_nodes + j]);
                                fprintf(stdout, "\n");
                        }
                        return TCL_OK;
                }
		if(strcmp(argv[1], "num_nodes") == 0) {
			tcl.resultf("%d", nodes());
			return TCL_OK;
		}
        }
        else if(argc == 3) {
                if (strcasecmp(argv[1], "num_nodes") == 0) {
                        assert(num_nodes == 0);

                        // allow for 0 based to 1 based conversion
                        num_nodes = atoi(argv[2]) + 1;

                        min_hops = new int[num_nodes * num_nodes];
                        bzero((char*) min_hops,
                              sizeof(int) * num_nodes * num_nodes);

                        instance_ = this;

                        return TCL_OK;
                }
        }
        else if(argc == 5) {
                if (strcasecmp(argv[1], "set-dist") == 0) {
                        int i = atoi(argv[2]);
                        int j = atoi(argv[3]);
                        int d = atoi(argv[4]);

                        assert(i >= 0 && i < num_nodes);
                        assert(j >= 0 && j < num_nodes);

                        min_hops[i * num_nodes + j] = d;
                        min_hops[j * num_nodes + i] = d;
                        return TCL_OK;
                }
        } 
        return BiConnector::command(argc, argv);
}









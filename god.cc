/* -*- c++ -*-
   god.cc

   General Operations Director

   perform operations requiring omnipotence in the simulation

   NOTE: Tcl node indexs are 0 based, NS C++ node IP addresses (and the
   node->index() are 1 based.

   $Id: god.cc,v 1.1 1998/12/08 19:14:15 haldar Exp $
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
        nsaddr_t src = ih->src();
        nsaddr_t dst = ih->dst();

        assert(min_hops);

        if (!DATA_PACKET(ch->ptype())) return;

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

                        assert(i > 0 && i < num_nodes);
                        assert(j > 0 && j < num_nodes);

                        min_hops[i * num_nodes + j] = d;
                        min_hops[j * num_nodes + i] = d;
                        return TCL_OK;
                }
        } 
        return BiConnector::command(argc, argv);
}









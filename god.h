/* -*- c++ -*-
   god.h

   General Operations Director

   perform operations requiring omnipotence in the simulation
   */

#ifndef __god_h
#define __god_h

#include <stdarg.h>
#include <bi-connector.h>
#include <object.h>
#include <packet.h>
#include <trace.h>

#include <node.h>

class God : public BiConnector {
public:
        God();

        int             command(int argc, const char* const* argv);

        void            recv(Packet *p, Handler *h);
        void            stampPacket(Packet *p);

        int initialized() {
                return num_nodes && min_hops && uptarget_;
        }

        int             hops(int i, int j);
        static God*     instance() { assert(instance_); return instance_; }

private:
        int num_nodes;
        int* min_hops;   // square array of num_nodesXnum_nodes
                         // min_hops[i * num_nodes + j] giving 
			 // minhops between i and j
        static God*     instance_;
};

#endif

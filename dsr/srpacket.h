/* -*- c++ -*-
   srpacket.h

   $Id: srpacket.h,v 1.1 1998/12/08 19:17:26 haldar Exp $
   */

#ifndef _SRPACKET_H_
#define _SRPACKET_H_

#include <packet.h>
#include <dsr/hdr_sr.h>

#include "path.h"

struct SRPacket {

  ID dest;
  ID src;
  Packet *pkt;			/* the inner NS packet */
  Path route;
  SRPacket(Packet *p, struct hdr_sr *srh) : pkt(p), route(srh) {}
  SRPacket() : pkt(NULL) {}
};

#endif  _SRPACKET_H_

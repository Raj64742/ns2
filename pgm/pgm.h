#ifdef PGM

#ifndef PGM_H
#define PGM_H

/*
 * Copyright (c) 2001 University of Southern California.
 * All rights reserved.                                            
 *                                                                
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 **
 * Pragmatic General Multicast (PGM), Reliable Multicast
 *
 * pgm.h
 *
 * This holds the packet header structures, and packet type constants for
 * the PGM implementation.
 *
 * Ryan S. Barnett, 2001
 * rbarnett@catarina.usc.edu
 */

/*
 * Types for PT_PGM packets.
 */

#define PGM_SPM         0x00
#define PGM_ODATA       0x04
#define PGM_RDATA       0x05
#define PGM_NAK         0x08
#define PGM_NCF         0x0A

// ************************************************************
// Header included with all PGM packets.
// ************************************************************
struct hdr_pgm {
  int type_; // PGM sub-type.

  ns_addr_t tsi_; // The transport session ID (source addr and source port).

  // The sequence number for this PGM packet. It can have differing meaning
  // depending on whether the packet is SPM, ODATA, RDATA, NAK, or NCF.
  int seqno_;

  static int offset_; // Used to locate this packet header.
  inline static int& offset() { return offset_; }
  inline static hdr_pgm* access(const Packet *p) {
    return (hdr_pgm *)p->access(offset_);
  }

};

// ************************************************************
// Header included with PGM SPM-type packets.
// ************************************************************
struct hdr_pgm_spm {
  ns_addr_t spm_path_; // Address of upstream PGM node.

  static int offset_; // Used to locate this packet header.
  inline static int& offset() { return offset_; }
  inline static hdr_pgm_spm* access(const Packet *p) {
    return (hdr_pgm_spm *)p->access(offset_);
  }

};

// ************************************************************
// Header included with PGM NAK-type packets.
// ************************************************************
struct hdr_pgm_nak {
  ns_addr_t source_; // Address of ODATA source.
  ns_addr_t group_; // Address for the multicast group.

  static int offset_; // Used to locate this packet header.
  inline static int& offset() { return offset_; }
  inline static hdr_pgm_nak* access(const Packet *p) {
    return (hdr_pgm_nak *)p->access(offset_);
  }

};

// Quick reference macros to access the packet headers.
#define HDR_PGM(p) (hdr_pgm::access(p))
#define HDR_PGM_SPM(p) (hdr_pgm_spm::access(p))
#define HDR_PGM_NAK(p) (hdr_pgm_nak::access(p))

#endif // PGM_H

#endif // PGM

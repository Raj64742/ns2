/*
 * Copyright (c) 2000 University of Southern California.
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
 *
 * Support for NixVector routing
 * contributed to ns from 
 * George F. Riley, Georgia Tech, Spring 2000
 */

#ifndef __NIXVEC_H__
#define __NIXVEC_H__

#include <utility>  // for pair
#ifdef WIN32
#include <pair.h>   // for MSVC 6.0 that doens't have a proper <utility>
#endif /* WIN32 */

// Define a type for the neighbor index
typedef unsigned long Nix_t;
typedef unsigned long Nixl_t; // Length of a NV
const   Nix_t NIX_NONE = 0xffffffff;    // If not a neighbor
const   Nixl_t NIX_BPW = 32;            // Bits per long word
typedef pair<Nix_t,  Nixl_t> NixPair_t; // Index, bits needed
typedef pair<Nix_t*, Nixl_t> NixpPair_t;// NV Pointer, length


// The variable length neighbor index routing vector
class NixVec {
  public :
    NixVec () : m_pNV(0), m_used(0), m_alth(0) { };
    NixVec(NixVec*);              // Construct from existing
    ~NixVec();                    // Destructor
    void   Add(NixPair_t);        // Add bits to the nix vector
    Nix_t  Extract(Nixl_t);       // Extract the specified number of bits
    Nix_t  Extract(Nixl_t, Nixl_t*); // Extract using external "used"
    NixpPair_t Get(void);         // Get the entire nv
    void   Reset();               // Reset used to 0
    Nixl_t Lth();                 // Get length in bits of allocated
    void   DBDump();              // Debug..print it out
    Nixl_t ALth() { return m_alth;} // Debug...how many bits actually used
    static Nixl_t GetBitl(Nix_t); // Find out how many bits needed
  private :
    Nix_t* m_pNV;  // Points to variable lth nixvector (or actual if l == 32)
  //    Nixl_t m_lth;  // Length of this nixvector (computed from m_alth)
    Nixl_t m_used; // Used portion of this nixvector
    Nixl_t m_alth; // Actual length (largest used)
};

#endif

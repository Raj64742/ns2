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
 * Define a bitmap object
 * contributed to ns
 * George Riley, Georgia Tech, Winter 2000
 */

// Creates a bitmap object.  The 'entries' can be more than one bit,
// but default to one bit.  Bits per entry can't be more than 32.
// Entries DO NOT span words, for ease of coding

#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <iostream.h>
#include <sys/types.h>

const unsigned  int BITS_ULONG = 32;

class BitMap {
  public :
    BitMap      ( );
    BitMap      ( u_long Size, u_long BitsPerEntry = 1);
    ~BitMap     () {if (m_Words > 1) delete [] m_pM; }
    void   Set  ( u_long Which, u_long Value = 1);
    void   Clear( u_long Which);
    u_long Get  ( u_long Which);  // Return the specified entry
    size_t Size ( void );         // Return storage used
    void   Log  ( ostream& os);   // Log to ostream
    void DBPrint();
    static u_long FindBPE( u_long );
    static size_t EstimateSize( u_long Size, u_long BitsPerEntry);
    //    friend ostream& operator<<(ostream&, const BitMap* ); // Output a bitmap
  private :
    u_long* GetWordAddress(u_long Which); // Get a pointer to the word needed
    u_long  GetBitMask(u_long Which);     // Get a bit mask to the needed bits
    short   GetShiftCount( u_long Which); // Get a shift count for position
    void    Validate(u_long Which);       // Validate which not too large
    u_long  m_Size; // how many entries
    u_long  m_BPE;  // Bits per entry
    u_long  m_Words;// Number of words needed for bitmap
    short   m_EPW;  // Entries per word
    u_long* m_pM; // Pointer to the actual map (or the data if < 32 bits)
};

#endif

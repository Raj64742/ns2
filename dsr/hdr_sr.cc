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

/* -*- c++ -*-
   hdr_sr.cc

   source route header
   $Id: hdr_sr.cc,v 1.4 2000/08/18 18:34:02 haoboy Exp $
*/

#include <stdio.h>
#include <dsr/hdr_sr.h>

int hdr_sr::offset_;

static class SRHeaderClass : public PacketHeaderClass {
public:
	SRHeaderClass() : PacketHeaderClass("PacketHeader/SR",
					     sizeof(hdr_sr)) {
		bind_offset(&hdr_sr::offset_);
	}
	void export_offsets() {
		field_offset("valid_", OFFSET(hdr_sr, valid_));
		field_offset("num_addrs_", OFFSET(hdr_sr, num_addrs_));
		field_offset("cur_addr_", OFFSET(hdr_sr, cur_addr_));
	}
} class_SRhdr;

char *
hdr_sr::dump()
{
  static char buf[100];
  dump(buf);
  return (buf);
}

void
hdr_sr::dump(char *buf)
{
  char *ptr = buf;
  *ptr++ = '[';
  for (int i = 0; i < num_addrs_; i++)
    {
      sprintf(ptr, "%s%d ", (i == cur_addr_) ? "|" : "", addrs[i].addr);
      ptr += strlen(ptr);
    }
  *ptr++ = ']';
  *ptr = '\0';
}

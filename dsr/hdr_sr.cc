/* -*- c++ -*-
   hdr_sr.cc

   source route header
   $Id: hdr_sr.cc,v 1.1 1998/12/08 19:17:22 haldar Exp $
*/

#include <stdio.h>
#include <dsr/hdr_sr.h>

int hdr_sr::offset_;

static class SRHeaderClass : public PacketHeaderClass {
public:
	SRHeaderClass() : PacketHeaderClass("PacketHeader/SR",
					     sizeof(hdr_sr)) {
		offset(&hdr_sr::offset_);
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
      ptr += strlen(sprintf(ptr, "%s%d ",
		     (i == cur_addr_) ? "|" : "",
		     addrs[i].addr));
    }
  *ptr++ = ']';
  *ptr = '\0';
}

/*
 * ctrMcast.h
 * Copyright (C) 1997 by USC/ISI
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
 * Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 * 
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/ctrMcast.h,v 1.4 1997/08/08 02:55:29 polly Exp $ (USC/ISI)
 */

#ifndef ns_ctrmcast_h
#define ns_ctrmcast_h

#include "packet.h"

struct hdr_CtrMcast {
  nsaddr_t       src_;            /* mcast data source */
  nsaddr_t       group_;          /* mcast data destination group */
  int            fid_;
  
  /* per field member functions */
  nsaddr_t& src() { return src_;   }
  nsaddr_t& group() { return group_;   }
  int& flowid() { return fid_;   }
};

#endif

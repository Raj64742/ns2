/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * prune.h
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/prune.h,v 1.4.2.1 1998/07/15 18:34:16 kannan Exp $
 */

#ifndef ns_prune_h
#define ns_prune_h

struct hdr_prune {
	char           ptype_[15];
        nsaddr_t       from_;
        nsaddr_t       src_;
        nsaddr_t       group_;

        /* per-field member functions */
        char*     type()  { return ptype_; }
        nsaddr_t& from()  { return from_;  }
        nsaddr_t& src()   { return src_;   }
        nsaddr_t& group() { return group_; }
	int maxtype()     { return sizeof(ptype_); }
};

#endif

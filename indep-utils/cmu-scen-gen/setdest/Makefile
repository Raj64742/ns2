# Generated automatically from Makefile.in by configure.
# 
#  Copyright (c) 1999 by the University of Southern California
#  All rights reserved.
# 
#  Permission to use, copy, modify, and distribute this software and its
#  documentation in source and binary forms for non-commercial purposes
#  and without fee is hereby granted, provided that the above copyright
#  notice appear in all copies and that both the copyright notice and
#  this permission notice appear in supporting documentation. and that
#  any documentation, advertising materials, and other materials related
#  to such distribution and use acknowledge that the software was
#  developed by the University of Southern California, Information
#  Sciences Institute.  The name of the University may not be used to
#  endorse or promote products derived from this software without
#  specific prior written permission.
# 
#  THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
#  the suitability of this software for any purpose.  THIS SOFTWARE IS
#  PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
#  INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 
#  Other copyrights might apply to parts of this software and are so
#  noted when applicable.
#
# Makefile for CMU mobile code scenario generation.
#
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/indep-utils/cmu-scen-gen/setdest/Attic/Makefile,v 1.2 1999/03/04 02:21:37 haoboy Exp $

CCX = c++
MKDEP	= ../../../conf/mkdep

CFLAGS = -g -Wall
LDFLAGS = 
LIBS =  

all: setdest calcdest

setdest: setdest.o
	$(CCX) -o setdest $@.o $(LDFLAGS) $(CFLAGS) $(LIBS)

calcdest: calcdest.o
	$(CCX) -o calcdest $@.o $(LDFLAGS) $(CFLAGS) $(LIBS)

setbox: setbox.o
	$(CCX) -o setbox $@.o $(LDFLAGS) $(CFLAGS) $(LIBS)

clean:
	@rm -f setdest setbox *.o *.core

.SUFFIXES: .cc

.cc.o:
	$(CCX) -c $(CFLAGS) -o $@ $*.cc


# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
# Time-stamp: <2000-08-29 11:19:56 haoboy>
# 
#  Copyright (c) 1997 by the University of Southern California
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
#  Original source contributed by Gaeil Ahn. See below.
#
#  $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/mpls/ns-mpls-classifier.tcl,v 1.1 2000/08/29 19:28:03 haoboy Exp $

###########################################################################
# Copyright (c) 2000 by Gaeil Ahn                                	  #
# Everyone is permitted to copy and distribute this software.		  #
# Please send mail to fog1@ce.cnu.ac.kr when you modify or distribute     #
# this sources.								  #
###########################################################################

#############################################################
#                                                           #
#     File: File for Classifier                             #
#     Author: Gaeil Ahn (fog1@ce.cnu.ac.kr), Jan. 2000      #
#                                                           #
#############################################################


Classifier/Addr/MPLS instproc init {args} {
       eval $self next $args
       $self instvar mpls_node_
       $self instvar rtable_
       
       set mpls_node_ ""
       set rtable_ ""
}

Classifier/Addr/MPLS instproc no-slot args {
	# Do nothing, just don't exit like the default no-slot{} does.
}


Classifier/Addr/MPLS instproc trace-packet-switching { time src dst ptype \
		ilabel op oiface olabel ttl psize } {
	$self instvar mpls_node_ 
	puts "$time [$mpls_node_ id]($src->$dst): $ptype $ilabel $op $oiface $olabel $ttl $psize"
}

# XXX Temporary interfaces
#
# All of the following instprocs should be moved into the MPLS functionality
# part of a Node. The big picture is that MPLS code should be packaged in a 
# module, then a Node should intercept add-route and pass it on to the MPLS 
# module, which then dispatches it to route-new, etc., depending on the 
# MPLS classifier status of the slot. This should not be done as a callback 
# from the classifier.

Classifier/Addr/MPLS instproc ldp-trigger-by-switch { fec } {
	$self instvar mpls_node_
	if { [Classifier/Addr/MPLS on-demand?] == 1 } {
		set msgid  1
	} else {
		set msgid -1
	}
	$mpls_node_ ldp-trigger-by-data $msgid [$mpls_node_ id] $fec *
}

# XXX This is a really bad way to check if routing table is built.
# During initialization of dynamic routing (e.g., DV), at each node, for 
# each possible destination, a nullAgent_ will be added to the routing table.
#
# Since rtable-ready does not know this, it will think that routing table
# is ready and start to compute routes. This is the reason that routing-new{}
# must do a special check (route-nochange{} does not need it because it is 
# only called when an existing slot is replaced, which cannot happen during 
# the initialization phase).
#
# A better way should be let Node to intercept add-route{}, and updated 
# rtable_ only when the target is not the default null agent. However, this 
# is difficult with current node-config design, where node types are exclusive.
# But this will change as time goes by.
Classifier/Addr/MPLS instproc rtable-ready { fec } {
	$self instvar rtable_
	#
	# determine whether or not a routing table is stable status
	#
	set ns [Simulator instance]
	if { [lsearch $rtable_ $fec] == -1 } {
		lappend rtable_ $fec
	}
	set rtlen [llength $rtable_]
	set nodelen [$ns array size Node_]
	if { $rtlen == $nodelen } {
		return 1
	} else {
		return 0
	}
}

Classifier/Addr/MPLS instproc routing-new { slot time } {
	$self instvar mpls_node_ rtable_
	if { [$self control-driven?] != 1 } {
		return
	}
	if { [lsearch $rtable_ [$mpls_node_ id]] == -1 } {
		lappend rtable_ [$mpls_node_ id]
	}
	if { [$self rtable-ready $slot] == 1 } {
		# Now, routing table is built.  really ?
		# Check whether static routing or dynamic routing
		set rtlen   [llength $rtable_]
		for {set i 0} {$i < $rtlen} {incr i 1} {
			set nodeid [lindex $rtable_ $i]
			if { [$mpls_node_ get-nexthop $nodeid] == -1 } {
				#
				# It's Dynamic Routing
				#
				set rtable_ "" 
				return
			}
		}
		# XXX The following piece of code is only reached for 
		# static routing but not for dynamic routing. We have to 
		# do the scheduling because this piece may be executed 
		# BEFORE '$ns run' is completed; thus calling ldp-... 
		# will not succeed because other initialization may not 
		# have all finished yet. 
		set rtable_ "" 
		[Simulator instance] at [expr $time] \
				"$mpls_node_ ldp-trigger-by-routing-table"
	}
}

Classifier/Addr/MPLS instproc routing-nochange {slot time} {
	$self instvar mpls_node_ rtable_
	
	if { [$self control-driven?] != 1 } {
		return
	}
	if { [lsearch $rtable_ [$mpls_node_ id]] == -1 } {
		lappend rtable_ [$mpls_node_ id]
	}
	if { [$self rtable-ready $slot] == 1 } {
		set rtable_ "" 
  		[Simulator instance] at $time \
				"$mpls_node_ ldp-trigger-by-routing-table"
	}
}

Classifier/Addr/MPLS instproc routing-update {slot time} {
	$self instvar mpls_node_ rtable_
	if {[$self control-driven?] != 1} {
		return
	}
	set fec $slot
	set pft_outif [$mpls_node_ get-outgoing-iface $fec -1]
	set rt_outif  [$mpls_node_ get-nexthop $fec]
	if { $pft_outif == -1 || $rt_outif == -1 } {
		return
	}
	$mpls_node_ ldp-trigger-by-control $fec *
	return
}


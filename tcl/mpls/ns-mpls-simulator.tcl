#
#  Time-stamp: <2000-08-29 11:12:16 haoboy>
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
#  $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/mpls/ns-mpls-simulator.tcl,v 1.1 2000/08/29 19:28:03 haoboy Exp $

###########################################################################
# Copyright (c) 2000 by Gaeil Ahn                                	  #
# Everyone is permitted to copy and distribute this software.		  #
# Please send mail to fog1@ce.cnu.ac.kr when you modify or distribute     #
# this sources.								  #
###########################################################################

#############################################################
#                                                           #
#     File: File for Simulator class                        #
#     Author: Gaeil Ahn (fog1@ce.cnu.ac.kr), Jan. 2000      #
#                                                           #
#############################################################

Simulator instproc mpls-node args {
	$self node-config -MPLS ON
	set n [$self node]
	$self node-config -MPLS OFF
	return $n
}

Simulator instproc LDP-peer { src dst } {
	# Establish LDP-peering between node $src and $dst. The names src and
	# dst does NOT indicate a single-direction relationship.
        if { ![$src is-neighbor $dst] } {
		return
	}
	set ldpsrc [$src make-ldp]
	set ldpdst [$dst make-ldp]
	$ldpsrc set peer_node_ [$dst id]
	$ldpdst set peer_node_ [$src id]
        $self connect $ldpsrc $ldpdst
}

Simulator instproc ldp-notification-color {color} {
	$self color 101 $color
}

Simulator instproc ldp-request-color {color} {
	$self color 102 $color
}

Simulator instproc ldp-mapping-color {color} {
	$self color 103 $color
}

Simulator instproc ldp-withdraw-color {color} {
	$self color 104 $color
}

Simulator instproc ldp-release-color {color} {
	$self color 105 $color
}

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:

#
# Copyright (c) 1996 Regents of the University of California.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
# 	This product includes software developed by the MASH Research
# 	Group at the University of California Berkeley.
# 4. Neither the name of the University nor of the Research Group may be
#    used to endorse or promote products derived from this software without
#    specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-route.tcl,v 1.1 1997/09/10 06:50:38 kannan Exp $
#

Simulator instproc rtproto {proto args} {
    eval [$self get-routelogic] register $proto $args
}

Simulator instproc get-routelogic {} {
    $self instvar routingTable_
    if ![info exists routingTable_] {
	set routingTable_ [new RouteLogic]
    }
    return $routingTable_
}

Simulator instproc compute-routes {} {
	$self instvar Node_ routingTable_ link_
	#
	# Compute all the routes using the route-logic helper object.
	#
        set r [$self get-routelogic]
        #set r [new RouteLogic]		;# must not create multiple instances
	foreach ln [array names link_] {
		set L [split $ln :]
		set srcID [lindex $L 0]
		set dstID [lindex $L 1]
	        if { [$link_($ln) up?] == "up" } {
			$r insert $srcID $dstID [$link_($ln) cost?]
		} else {
			$r reset $srcID $dstID
		}
	}
	$r compute
	#$r dump $nn

	#
	# Set up each classifer (aka node) to act as a router.
	# Point each classifer table to the link object that
	# in turns points to the right node.
	#
	set i 0
	set n [Node set nn_]
	while { $i < $n } {
		set n1 $Node_($i)
		set j 0
		while { $j < $n } {
			if { $i != $j } {
			        # shortened nexthop to nh, to fit add-route in
			        # a single line
				set nh [$r lookup $i $j]
			        if { $nh >= 0 } {
					$n1 add-route $j [$link_($i:$nh) head]
				}
			} 
			incr j
		}
		incr i
	}
#	delete $r
#XXX used by multicast router
	#set routingTable_ $r		;# already set by get-routelogic
}

RouteLogic instproc register {proto args} {
    $self instvar rtprotos_
    if [info exists rtprotos_($proto)] {
	eval lappend rtprotos_($proto) $args
    } else {
	set rtprotos_($proto) $args
    }
}

RouteLogic instproc configure {} {
    $self instvar rtprotos_
    if [info exists rtprotos_] {
	foreach proto [array names rtprotos_] {
	    eval Agent/rtProto/$proto init-all $rtprotos_($proto)
	}
    } else {
        Agent/rtProto/Static init-all
	# set rtprotos_(Static) 1
    }
}

RouteLogic instproc lookup { nodeid destid } {
    if { $nodeid == $destid } {
	return $nodeid
    }

    set ns [Simulator instance]
    set node [$ns get-node-by-id $nodeid]
    set rtobj [$node rtObject?]
    if { $rtobj != "" } {
	$rtobj lookup [$ns get-node-by-id $destid]
    } else {
	$self cmd lookup $nodeid $destid
    }
}

RouteLogic instproc notify {} {
    $self instvar rtprotos_
    foreach i [array names rtprotos_] {
	Agent/rtProto/$i compute-all
    }

    foreach i [CtrMcastComp info instances] {
	$i notify
    }
}

#
# debugging method to dump table (see route.cc for C++ methods)
#
RouteLogic instproc dump nn {
	set i 0
	while { $i < $nn } {
		set j 0
		while { $j < $nn } {
			puts "$i -> $j via [$self lookup $i $j]"
			incr j
		}
		incr i
	}
}

#####
# Now source the dynamic routing protocol methods
#
source ../rtglib/route-proto.tcl
source ../rtglib/dynamics.tcl

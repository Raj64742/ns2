# 
#  Copyright (c) 2000 by the University of Southern California
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
#  Copyright (C) 1998 by Mingzhou Sun. All rights reserved.
#
#  This software is developed at Rensselaer Polytechnic Institute under 
#  DARPA grant No. F30602-97-C-0274
#  Redistribution and use in source and binary forms are permitted
#  provided that the above copyright notice and this paragraph are
#  duplicated in all such forms and that any documentation, advertising
#  materials, and other materials related to such distribution and use
#  acknowledge that the software was developed by Mingzhou Sun at the
#  Rensselaer  Polytechnic Institute.  The name of the University may not 
#  be used to endorse or promote products derived from this software 
#  without specific prior written permission.
#
# Link State Routing Agent
#
# $Header

Agent/rtProto/LS set UNREACHABLE  [rtObject set unreach_]
Agent/rtProto/LS set preference_        120
Agent/rtProto/LS set INFINITY           [Agent set ttl_]
Agent/rtProto/LS set advertInterval     1800

create-packet-header rtProtoLS off_LS_

Simulator instproc get-number-of-nodes {} {
	$self instvar Node_
	return  [array size Node_] 
}
    
# like DV's, except $self cmd initialize and cmd setNodeNumber
Agent/rtProto/LS proc init-all args {
	if { [llength $args] == 0 } {
		set nodeslist [[Simulator instance] all-nodes-list]
	} else { 
		eval "set nodeslist $args"
	}
	Agent set-maxttl Agent/rtProto/LS INFINITY
	eval rtObject init-all $nodeslist
	foreach node $nodeslist {
		set proto($node) [[$node rtObject?] add-proto LS $node]
	}
	foreach node $nodeslist {
		foreach nbr [$node neighbors] {
			set rtobj [$nbr rtObject?]
			if { $rtobj != "" } {
				set rtproto [$rtobj rtProto? LS]
				if { $rtproto != "" } {
					$proto($node) add-peer $nbr \
						[$rtproto set agent_addr_] \
						[$rtproto set agent_port_]
				}
			}
		}
	}

	# -- LS stuffs --
	set first_node [lindex $nodeslist 0 ]
	foreach node $nodeslist {
		set rtobj [$node rtObject?]
		if { $rtobj != "" } {
			set rtproto [$rtobj rtProto? LS]
			if { $rtproto != "" } {
				$rtproto cmd initialize
				if { $node == $first_node } {
					$rtproto cmd setNodeNumber [[Simulator instance] get-number-of-nodes]
				}
				# $rtproto cmd sendUpdates
			}
		}
	}
}

# like DV's , except LS_ready
Agent/rtProto/LS instproc init node {
	global rtglibRNG

	$self next $node
	$self instvar ns_ rtObject_ ifsUp_ rtsChanged_ rtpref_ nextHop_ \
		nextHopPeer_ metric_ multiPath_
	Agent/rtProto/LS instvar preference_ 
	
	;# -- LS stuffs -- 
	$self instvar LS_ready
	set LS_ready 0
	set rtsChanged_ 1

	set UNREACHABLE [$class set UNREACHABLE]
	foreach dest [$ns_ all-nodes-list] {
		set rtpref_($dest) $preference_
		set nextHop_($dest) ""
		set nextHopPeer_($dest) ""
		set metric_($dest)  $UNREACHABLE
	}
	set ifsUp_ ""
	set multiPath_ [[$rtObject_ set node_] set multiPath_]
	set updateTime [$rtglibRNG uniform 0.0 0.5]
	$ns_ at $updateTime "$self send-periodic-update"
}

# like DV's
Agent/rtProto/LS instproc add-peer {nbr agentAddr agentPort} {
	$self instvar peers_
	$self set peers_($nbr) [new rtPeer $agentAddr $agentPort $class]
}

# like DV's, except cmd sendUpdates, instead of not send-updates
Agent/rtProto/LS instproc send-periodic-update {} {
	global rtglibRNG
	
	$self instvar ns_

	# -- LS stuffs --
	$self cmd sendUpdates

	set updateTime [expr [$ns_ now] + ([$class set advertInterval] * \
			[$rtglibRNG uniform 0.5 1.5])]
	$ns_ at $updateTime "$self send-periodic-update"
}

# like DV's, except cmd computeRoutes
Agent/rtProto/LS instproc compute-routes {} {
	$self instvar node_
	puts "Node [$node_ id]: Agent/rtProto/LS compute-routes"
	$self cmd computeRoutes
	$self install-routes
}

# like DV's, except cmd intfChanged(), comment out DV stuff
Agent/rtProto/LS instproc intf-changed {} {
	$self instvar ns_ peers_ ifs_ ifstat_ ifsUp_ nextHop_ \
			nextHopPeer_ metric_
	set INFINITY [$class set INFINITY]
	set ifsUp_ ""
	foreach nbr [array names peers_] {
		set state [$ifs_($nbr) up?]
		if {$state != $ifstat_($nbr)} {
			set ifstat_($nbr) $state
		}
	}
	# -- LS stuffs --
	$self cmd intfChanged
	$self route-changed
}

;# called by C++ whenever a LSA or Topo causes a change in the routing table
Agent/rtProto/LS instproc route-changed {} {
	$self instvar node_ 
	puts "At [[Simulator instance] now] node [$node_ id]: \
Agent/rtProto/LS route-changed"

	$self instvar rtObject_  rtsChanged_
	$self install-routes
	set rtsChanged_ 1
	$rtObject_ compute-routes
}

# put the routes computed in C++ into tcl space
Agent/rtProto/LS instproc install-routes {} {
	$self instvar ns_ ifs_ rtpref_ metric_ nextHop_ nextHopPeer_
	$self instvar peers_ rtsChanged_ multiPath_
	$self instvar node_  preference_ 
    
	set INFINITY [$class set INFINITY]
	set MAXPREF  [rtObject set maxpref_]
	set UNREACH  [rtObject set unreach_]
	set rtsChanged_ 1 
	
	foreach dst [$ns_ all-nodes-list] {
		# puts "installing routes for $dst"
		if { $dst == $node_ } {
			set metric_($dst) 32  ;# the magic number
			continue
		}
		#  puts " [$node_ id] looking for route to [$dst id]"
		set path [$self cmd lookup [$dst id]]
		# puts "$path" ;# debug
		if { [llength $path ] == 0 } {
			# no path found in LS
			set rtpref_($dst) $MAXPREF
			set metric_($dst) $UNREACH
			set nextHop_($dst) ""
			continue
		} else {
			set cost [lindex $path 0]
			set rtpref_($dst) $preference_
			set metric_($dst) $cost
			
			if { ! $multiPath_ } {
				set nhNode [$ns_ get-node-by-id [lindex $path 1]]
				set nextHop_($dst) $ifs_($nhNode)
				continue
			}
			set nextHop_($dst) ""
			set nh ""
			set count [llength $path]
			foreach nbr [array names peers_] {
				foreach nhId [lrange $path 1 $count ] {
					if { [$nbr id] == $nhId } {
						lappend nextHop_($dst) $ifs_($nbr)
						break
					}
				}
			}
		}
	}
}

Agent/rtProto/LS instproc send-updates changes {
	$self cmd send-buffered-messages
}

Agent/rtProto/LS proc compute-all {} {
    # Because proc methods are not inherited from the parent class.
}

Agent/rtProto/LS instproc get-node-id {} {
	$self instvar node_
	return [$node_ id]
}

Agent/rtProto/LS instproc get-links-status {} {
	$self instvar ifs_ ifstat_ 
	set linksStatus ""
	foreach nbr [array names ifs_] {
		lappend linksStatus [$nbr id]
		if {[$ifs_($nbr) up?] == "up"} {
			lappend linksStatus 1
		} else {
			lappend linksStatus 0
		}
		lappend linksStatus [$ifs_($nbr) cost?]
	}
	set linksStatus
}

Agent/rtProto/LS instproc get-peers {} {
	$self instvar peers_
	set peers ""
	foreach nbr [array names peers_] {
		lappend peers [$nbr id]
		lappend peers [$peers_($nbr) addr?]
		lappend peers [$peers_($nbr) port?]
	}
	set peers
}

# needed to calculate the appropriate timeout value for retransmission 
# of unack'ed LSA or Topo messages
Agent/rtProto/LS instproc get-delay-estimates {} {
	$self instvar ifs_ ifstat_ 
	set total_delays ""
	set packet_size 8000.0 ;# bits
	foreach nbr [array names ifs_] {
		set intf $ifs_($nbr)
		set q_limit [ [$intf queue ] set limit_]
		set bw [bw_parse [ [$intf link ] set bandwidth_ ] ]
		set p_delay [time_parse [ [$intf link ] set delay_] ]
		set total_delay [expr $q_limit * $packet_size / $bw + $p_delay]
		$self cmd setDelay [$nbr id] $total_delay
	}
}

#
# tcl/mcast/ST.tcl
#
# Implementation of a simple shared tree protocol.  No timers.  Nodes send grafts/prunes 
# toward the RP to join/leave the group.  The user needs to set two protocol variables:
# "ST set Groups_ <list of multicast groups>"
# "ST set RP_($group) $node" - indicates that $node acts as an RP for the $group
#

Class ST -superclass McastProtocol

ST set decaps_() "" ;# decapsulators
#ST set star_value_ 0 ;# value of '*' as in (*, G), should be a non-existent address

ST instproc init { sim node } {
	$self instvar ns_ node_ id_
	ST instvar decaps_    ;# decapsulators by group
	#these two instvars are supposed to be initialized by the user
	ST instvar RP_        ;# nodes which serve as RPs.  it will be an "assoc array" RP_(<group>)
	ST instvar Groups_    ;# list of groups.
	if {![info exists RP_] || ![info exists Groups_]} {
		error "ST: 'ST instvar RP_' and 'ST instvar Groups_' must be set"
		exit 1
	}
	$self instvar encaps_     ;# lists of encapsulators by group for a given node
	$self instvar mcast_ctr_  ;# graft/prune agent
	
	$self next $sim $node

	set mcast_ctr_ [new Agent/Mcast/Control $self]
	$node attach $mcast_ctr_
	
	foreach grp $Groups_ {
		foreach agent [$node set agents_] {
			if {[expr $grp] == [expr [$agent set dst_]]}  {
				#found an agent that's sending to a group.
				#need to add Encapsulator
				puts "attaching a Encapsulator for group $grp, node $id_"
				set e [new Agent/Encapsulator]
				$e set class_ 32
				$e set status_ 1
				$node attach $e
				$e decap-target [$node entry]

				$node add-mfc $id_ $grp "?" ""
				set src_rep [$node getReps $id_ $grp]
				$src_rep set ignore_ 1

				lappend encaps_($grp) $e
				$agent target $e
			}
		}
		#if the node is an RP, need to attach a Decapsulator
		if {$RP_($grp) == $node} {
			puts "attaching a Decapsulator for group $grp, node $id_\n"
			set d [new Agent/Decapsulator]
			$node attach $d
			set decaps_($grp) $d
			$node set decaps_($grp) $d ;# a node should know its decapsulator!
		}
	}
}

ST instproc start {} {
	ST instvar decaps_     ;# arrays built during init
	$self instvar encaps_  ;# call
	ST instvar Groups_
	
	#interconnect all agents, decapsulators and encapsulators
	#this had better be done in init()
	foreach grp $Groups_ {
		if [info exists encaps_($grp)] {
			foreach e $encaps_($grp) {
				$e set dst_ [$decaps_($grp) set addr_]
			}
		}
	}
	$self next
}

ST instproc join-group  { group {src "*"} } {
	$self instvar node_ ns_
	ST instvar RP_
	
	set r [$node_ getReps "*" $group]
	
	if {$r == ""} {
		set iif [$self from-node-iface $RP_($group)]
		$self dbg "********* join: adding <*, $group, $iif>"
		$node_ add-mfc "*" $group $iif ""
		set r [$node_ getReps "*" $group]
	}
	if { ![$r is-active] } {
		$self send-ctrl "graft" $RP_($group) $group
	}
	$self next $group ; #annotate
}

ST instproc leave-group { group {src "*"} } {
	ST instvar RP_
	$self next $group
	$self send-ctrl "prune" $RP_($group) $group
}

ST instproc handle-wrong-iif { srcID group iface } {
	$self instvar node_
	$self dbg "ST: wrong iif $iface, src $srcID, grp $group"
}


# There should be only one mfc cache miss: the first time it receives a 
# packet to the group (RP). Just install a (*,G) entry.
ST instproc handle-cache-miss { srcID group iface } {
	$self instvar node_
	ST instvar RP_

	$self dbg "cache miss, src: $srcID, group: $group, iface: $iface"
	$self dbg "********* miss: adding <*, $group, *>"
	$node_ add-mfc "*" $group "*" "" ;# RP doesn't look at incoming iface
}

ST instproc drop { replicator src dst iface} {
	$self instvar node_ ns_
	ST instvar RP_
	
	# No downstream listeners
	# Send a prune back toward the RP
	$self dbg "drops src: $src, dst: $dst, replicator: [$replicator set srcID_]"
	
	if {$iface != "?"} {
		# so, this packet came from outside of the node
		$self send-ctrl "prune" $RP_($dst) $dst
	}
}

ST instproc recv-prune { from src group } {
	$self instvar node_ ns_ id_
	ST instvar RP_ 
	
	set r [$node_ getReps "*" $group]
	if {$r == ""} {
		# it's a cache miss!
		return
	}

	set tmpoif  [[$ns_ link $id_ $from] head]
	if ![$r exists $tmpoif] {
		warn "node $id_, got a prune from $from, trying to prune a non-existing interface?"
	} else {
		$r instvar active_
		if $active_($tmpoif) {
			$r disable $tmpoif
		}
		# If there are no remaining active output links
		# then send a prune upstream.
		if {![$r is-active]} {
			$self send-ctrl "prune" $RP_($group) $group
		}
	}
}

ST instproc recv-graft { from to group } {
	$self instvar node_ ns_ id_
	ST instvar RP_
	
	$self dbg "received a shared graft from: $from, to: $to"
	set r [$node_ getReps "*" $group]
	if {$r == ""} {
		set iif [$self from-node-iface $RP_($group)] ;#will be receiving from this iface
		$self dbg "********* graft: adding <*, $group, $iif>"
		$node_ add-mfc "*" $group $iif ""
		set r [$node_ getReps "*" $group]
	}
	if {[llength $r] > 1} {
		$self dbg "****** several replicators <*, $group> found: $r"
		foreach rep $r {
			$self dbg "      src: [$rep set srcID_]"
		}
		exit 1
	}
	if {![$r is-active]} {
		# if this node isn't on the tree, propagate the graft
		# if it's an RP, don't need to, but send-ctrl takes care of that.
		$self send-ctrl "graft" $RP_($group) $group
	}
	# graft on the interface
	set tmpoif [[$ns_ link $id_ $from] head]
	$r insert $tmpoif
}

#
# send a graft/prune for src/group up the RPF tree towards dst
#
ST instproc send-ctrl { which dst group } {
	$self instvar mcast_ctr_ ns_ node_ id_
	
	if {$node_ != $dst} {
		# send only if current $node_ is different from $dst
		set next_hop [$ns_ upstream-node $id_ [$dst id]]
		$ns_ simplex-connect $mcast_ctr_ \
				[[[$next_hop getArbiter] getType [$self info class]] set mcast_ctr_]
		switch $which {
			"prune" { 
				$mcast_ctr_ set class_ 31
			}
			"graft" {
				$mcast_ctr_ set class_ 30
			}
			default {
				warn {ST: send-ctrl: unknown control message}
				return
			}
		}
		$mcast_ctr_ send $which $id_ [$dst id] $group
	}
}

################ Helpers

ST instproc dbg arg {
	$self instvar ns_ node_ id_
	puts [format "At %.4f : node $id_ $arg" [$ns_ now]]
}

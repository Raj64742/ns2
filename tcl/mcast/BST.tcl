#
# tcl/mcast/BST.tcl
#
# Implementation of a simple shared bi-directional tree protocol.  No
# timers.  Nodes send grafts/prunes toward the RP to join/leave the
# group.  The user needs to set two protocol variables: 

# "BST set Groups_ <list of multicast groups>" 
# "ST set RP_($group) $node" - indicates that $node 
#                              acts as an RP for the $group

Class BST -superclass McastProtocol

BST set decaps_() "" ;# decapsulators
#BST set star_value_ 0 ;# value of '*' as in (*, G), should be a non-existent address

BST instproc init { sim node } {
	$self instvar mcast_ctr_  ;# graft/prune agent
	$self instvar id_

	set mcast_ctr_ [new Agent/Mcast/Control $self]
	$node attach $mcast_ctr_

	$self next $sim $node
}

BST instproc join-group  { group {src "*"} } {
	$self instvar node_ ns_
	BST instvar RP_
	
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

BST instproc leave-group { group {src "*"} } {
	BST instvar RP_
	$self next $group
	$self send-ctrl "prune" $RP_($group) $group
}

BST instproc handle-wrong-iif { srcID group iface } {
	$self instvar node_ id_
	$self dbg "BST: wrong iif $iface, src $srcID, grp $group"

	set next_hop [$ns_ upstream-node $id_ $srcID]
	set tmplh  [[$ns_ link $id_ [$next_hop id]] head]

	set rep [$node_ getReps "*" $group]
	set oifs ""
	if {$rep != ""} {
		$rep instvar active_
		foreach n [array names active_] {
			if {$active_($n) != tmplh} {
				lappend oifs $active_($n)
			}
		}
	} else {
		$self dbg "BST: handle-wrong-iif: no <*, $group> entry"
		exit 1
	}
	$node_ add-mfc $srcID $group $iface $oifs
	$self dbg "BST: installed <$srcID, $group, $iface, $oifs> entry"
}


# There should be only one mfc cache miss: the first time it receives a 
# packet to the group (RP). Just install a (*,G) entry.
BST instproc handle-cache-miss { srcID group iface } {
	$self instvar node_ id_ ns_
	BST instvar RP_

	$self dbg "handle-cache-miss, src: $srcID, group: $group, iface: $iface"
	set tmpoif ""
	if {$node_ != $RP_($group)} {
		set next_hop [$ns_ upstream-node $id_ [$RP_($group) id]]
		set tmpoif  [[$ns_ link $id_ [$next_hop id]] head]
	}
	$node_ add-mfc $srcID $group $iface $tmpoif 
	$self dbg "********* miss: adding <$srcID, $group, $iface, $tmpoif>"
}

BST instproc drop { replicator src dst iface} {
	$self instvar node_ ns_
	BST instvar RP_
	
	# No downstream listeners
	# Send a prune back toward the RP
	$self dbg "drops src: $src, dst: $dst, replicator: [$replicator set srcID_]"
	
	if {$iface != "?"} {
		# so, this packet came from outside of the node
		$self send-ctrl "prune" $RP_($dst) $dst
	}
}

BST instproc recv-prune { from src group iface} {
	$self instvar node_ ns_ id_
	BST instvar RP_ 
	
	set reps [$node_ getReps-group $group]
	if {$reps == ""} {
		# it's a cache miss!
		return
	}

	set tmpoif  [[$ns_ link $id_ $from] head]
	set propagate 1
	foreach r $reps {
		$r instvar active_
		$r disable $tmpoif
		# If there are no remaining active output links
		# then send a prune upstream.
		if {[$r is-active]} {
			set propagate 0
		}
	}
	if $propagate {
		$self send-ctrl "prune" $RP_($group) $group
	}
}

Node instproc getReps-group { group } {
	$self instvar replicator_
	set reps ""
	foreach key [array names replicator_ "*:$group"] { 
		lappend reps $replicator_($key)
	}
	return $reps
}

BST instproc recv-graft { from to group iface } {
	$self instvar node_ ns_ id_
	BST instvar RP_
	
	$self dbg "received a shared graft from: $from, to: $to, if: $iface"
	set rep [$node_ getReps "*" $group]
	if {$rep == ""} {
		set iif [$self from-node-iface $RP_($group)] ;#will be receiving from this iface
		$self dbg "********* graft: adding <*, $group, $iif>"
		$node_ add-mfc "*" $group $iif ""
	}
	set rep [$node_ getReps-group $group]
	set propagate 0
	set tmpoif [[$ns_ link $id_ $from] head]

	foreach r $rep {
#		$self dbg "GRAFT: replicator([$r set srcID_]), active([$r is-active])"
		if {![$r is-active]} {
			set propagate 1
		}
		# graft on the interface unless the interface we received a graft from is
		# the one the classifier classifies by...
		#   (should be graft.iif != r.iif)
		if {[$r set srcID_] == "*" || \
				$iface != [$r set iif_]} {
			#WHAT IF there's assymetrical routing???
			$r insert $tmpoif
#			$self dbg "GRAFT: insert in [$r set srcID_]"
		}
	}
	if $propagate {
		$self send-ctrl "graft" $RP_($group) $group
	}
}

#
# send a graft/prune for src/group up the RPF tree towards dst
#
BST instproc send-ctrl { which dst group } {
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
				warn {BST: send-ctrl: unknown control message}
				return
			}
		}
		$mcast_ctr_ send $which $id_ [$dst id] $group
	}
}

################ Helpers

BST instproc dbg arg {
	$self instvar ns_ node_ id_
	puts [format "At %.4f : node $id_ $arg" [$ns_ now]]
}





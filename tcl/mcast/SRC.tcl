#
# tcl/mcast/SRC.tcl
#
# Source-specific joins/leaves.
#

Class SRC -superclass McastProtocol

SRC instproc init { sim node } {
	$self next $sim $node

	$self instvar mcast_ctr_  ;# graft/prune agent
	set mcast_ctr_ [new Agent/Mcast/Control $self]
	$node attach $mcast_ctr_
}

SRC instproc join-group { group src } { 
	$self instvar node_ ns_

	set r [$node_ getReps $src $group]
		
	if {[llength $r] > 1} {
		$self dbg "****** several replicators <$src, $group> found: $r"
		exit 1
	}
	if {$r == ""} {
		set iif [$self from-node-iface $src]
		$self dbg "********* join: adding <$src, $group, $iif>"
		$node_ add-mfc $src $group $iif ""
		set r [$node_ getReps $src $group]
	}
	if {![$r is-active]} {
		$r set ignore_ 0
		$self send-ctrl "graft" [$ns_ set Node_($src)] $group
	}
	$self next [list $group $src]
}

SRC instproc leave-group { group src} { 
	$self instvar ns_
	$self send-ctrl "prune" [$ns_ set Node_($src)] $group
	$self next [list $group $src]
}

SRC instproc recv-graft { from src group } {
	$self instvar node_ ns_ id_

	$self dbg "received a graft  from: $from, src: $src"
	
	set oif_obj [[$ns_ link $id_ $from] head]
	
	set r [$node_ getReps $src $group]
	if {[llength $r] > 1} {
		$self dbg "****** several replicators <$src $group> found: $r"
		exit 1
	}
	if {$r == ""} {
		# it's a cache miss
		set iif [$self from-node-iface $src]
		$node_ add-mfc $src $group $iif ""
		set r [$node_ getReps $src $group]
	}
	if {![$r is-active]} {
		# if this node isn't on the source tree already,
		# propagate the graft
		$self send-ctrl "graft" [$ns_ set Node_($src)] $group
	} 
	# graft on the interface
	$r set ignore_ 0
	$r insert $oif_obj
}

SRC instproc recv-prune { from src group } {
	$self instvar node_ ns_ id_

	$self dbg "received a prune  from: $from, src: $src"
	
	set r [$node_ getReps $src $group]
	if {[llength $r] > 1} {
		$self dbg "****** several replicators <$src $group> found: $r"
		exit 1
	}

	if {$r == ""} return

	set oif_obj [[$ns_ link $id_ $from] head]
	if ![$r exists $oif_obj] {
		warn "node $id_, got a prune from $from, trying to prune a non-existing interface?"
	} else {
		$r instvar active_
		if $active_($oif_obj) {
			$r disable $oif_obj
		}
		# If there are no remaining active output links
		# then send a prune upstream.
		if {![$r is-active]} {
			$self send-ctrl "prune" [$ns_ set Node_($src)] $group
		}
	}
}

SRC instproc drop { replicator src dst iface} {
	$self instvar node_ ns_ 
	# No downstream listeners
	# Send a prune back towards src
	$self dbg "drops src: $src, dst: $dst, replicator: [$replicator set srcID_]"
	
	if {$iface != "?"} {
		# so, this packet came from outside of the node
		$self send-ctrl "prune" [$ns_ set Node_($src)] $dst
	} else {
		$replicator set ignore_ 1
	}
}

SRC instproc send-ctrl { which dst group } {
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
				warn {SRC: send-ctrl: unknown control message}
				return
			}
		}
		$mcast_ctr_ send $which $id_ [$dst id] $group
	}
}

SRC instproc handle-wrong-iif { srcID group iface } {
	$self dbg "wrong iif $iface, src $srcID, grp $group"
}

SRC instproc handle-cache-miss { srcID group iface } {
	$self instvar node_
	$self dbg "cache miss, src: $srcID, group: $group, iface: $iface"
	$self dbg "   adding <$srcID, $group, $iface>"
	$node_ add-mfc $srcID $group $iface ""
}
SRC instproc dbg arg {
	$self instvar ns_ node_ id_
	puts [format "At %.4f : SRC: node $id_ $arg" [$ns_ now]]
}

#####



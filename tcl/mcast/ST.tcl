#
# tcl/mcast/ST.tcl
#
# Implementation of a simple shared tree protocol.  No timers.  Nodes send grafts/prunes 
# toward the RP to join/leave the group.  The user needs to set two protocol variables:
# "ST set Groups_ <list of multicast groups>"
# "ST set RP_($group) $node" - indicates that $node acts as an RP for the $group
#

#the following is protocol specific
#Agent/Decapsulator instproc announce-arrival {src grp format} {
    #debug:
#    puts "received one $format packet from $src to $grp"
#}


Class ST -superclass McastProtocol

ST set decaps_() ""
#ST set star_value_ 0 ;# value of '*' as in (*, G), should be a non-existent address

ST instproc init { sim node } {
	$self instvar ns_
	ST instvar decaps_    ;# decapsulators by group
	#these two instvars are supposed to be initialized by the user
	ST instvar RP_        ;# nodes which serve as RPs.  it will be an "assoc array" RP_(<group>)
	ST instvar Groups_    ;# list of groups.
	if {![info exists RP_] || ![info exists Groups_]} {
		error "ST: 'ST instvar RP_' and 'ST instvar Groups_' must be set"
		exit 1
	}
	$self instvar encaps_     ;# lists of encapsulators by group for a given node
	$self instvar misses_     ;#mostly for debugging
	$self instvar mcast_ctr_  ;# graft/prune agent
	
	set ns_ $sim
	set id [$node id]

	set mcast_ctr_ [new Agent/Mcast/Control $self]
	$node attach $mcast_ctr_
	
	set misses_ 0         ;# initially there's been no misses
	
	foreach grp $Groups_ {
		foreach agent [$node set agents_] {
			if {[expr $grp] == [expr [$agent set dst_]]}  {
				#found an agent that's sending to a group.
				#need to add Encapsulator
				puts "attaching a Encapsulator for group $grp, node $id"
				set e [new Agent/Encapsulator]
				$e set class_ 32
				$e set status_ 1
				$node attach $e
				$e decap-target [$node entry]
				lappend encaps_($grp) $e
				$agent target $e
			}
		}
		#if the node is an RP, need to attach a Decapsulator
		if {$RP_($grp) == $node} {
			puts "attaching a Decapsulator for group $grp, node [$node id]\n"
			set d [new Agent/Decapsulator]
			$node attach $d
			puts "    with address [$d set addr_]" 
			set decaps_($grp) $d
			$node set decaps_($grp) $d ;# a node should know its decapsulator!
		}
	}
	$self next $sim $node
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
				puts "set encapsulator's dst_= [$e set dst_]"
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
		set iif -1
		if {$RP_($group) != $node_} {
			set rpfnbr [$ns_ upstream-node [$node_ id] [$RP_($group) id]]
			if {$rpfnbr != ""} {
				set rpflink [$ns_ link $rpfnbr $node_]
				set iif [$rpflink if-label?]
			}
		}
		$node_ add-mfc "*" $group $iif ""
		set r [$node_ getReps "*" $group]
	}
	puts "replicators: $r"
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
    puts "ST: wrong iif at node [$node_ id], src $srcID, grp $group"
}

# It's bad that in handle-cache-miss it's srcID and not a whole node!
ST instproc handle-cache-miss { srcID group iface } {
	$self instvar node_
	ST instvar RP_
	$self instvar misses_
	
	#there should be only one cache-miss
	puts "cache miss at [$node_ id], src: $srcID, group: $group, iface: $iface"
	#puts $misses_
	if {$misses_ > 0} {
		puts "ST: warning: repeated cache miss node: [$node_ id], src: $srcID, grp $group, if $iface"
	}
	# there should be only one mfc cache miss: the first time it receives a 
	# packet to the group (RP) or a graft message from a receiver.  
	# Just install a (*,G) entry.
	$node_ add-mfc "*" $group -1 ""
	#    puts "Handled a cache miss..."
	incr misses_
}

ST instproc drop { replicator src dst iface} {
	$self instvar node_ ns_
	ST instvar RP_
	
	set id [$node_ id]
	# No downstream listeners
	# Send a prune back toward the source
	puts "node: $id drops src: $src, dst: $dst, replicator: [$replicator set srcID_]"
	
	if {$iface != -1} {
		$self send-ctrl "prune" $RP_($dst) $dst
	}
#	$self annotate "$id dropping a packet from $src to $dst"
}

ST instproc recv-prune { from src group } {
	$self instvar node_ ns_  
	ST instvar RP_ 
	
	set r [$node_ getReps "*" $group]
	if {$r == ""} {
		# it's a cache miss!
		return
	}

	set id [$node_ id]
	set oifInfo [$node_ RPF-interface $src $id $from]
	set tmpoif  [[$ns_ link $id $from] head]
	if ![$r exists $tmpoif] {
		warn "node $id, got a prune from $from, trying to prune a non-existing interface?"
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
	$self instvar node_ ns_
	ST instvar RP_
	set id [$node_ id]
	
	puts "node [$node_ id] received graft from: $from, to: $to"
	set r [$node_ getReps "*" $group]
	if {$r == ""} {
		# it's a cache miss!
		set iif -1
		if {$RP_($group) != $node_} {
			set rpfnbr [$ns_ upstream-node [$node_ id] [$RP_($group) id]]
			if {$rpfnbr != ""} {
				set rpflink [$ns_ link $rpfnbr $node_]
				set iif [$rpflink if-label?]
			}
		}
		$node_ add-mfc "*" $group $iif ""
		set r [$node_ getReps "*" $group]
	}
	if {![$r is-active]} {
		# if this node isn't on the tree and isn't RP, propagate the graft
		$self send-ctrl "graft" $RP_($group) $group
	}
	# graft on the interface
	set tmpoif [[$ns_ link $id $from] head]
	$r insert $tmpoif
	#    $r set ignore_ 0
}

#
# send a graft/prune for src/group up the RPF tree towards dst
#
ST instproc send-ctrl { which dst group } {
	ST instvar RP_
	$self instvar mcast_ctr_ ns_ node_
	
	if {$node_ != $dst} {
		# send only if current $node_ is different from $dst
		set id [$node_ id]
		set next_hop [$ns_ upstream-node $id [$dst id]]
		#    puts "$id is sending a $which to [$dest id]"
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
		$mcast_ctr_ send $which $id [$dst id] $group
	}
}

ST instproc dump-routes args {
}

ST instproc annotate args {
	$self instvar dynT_
	if [info exists dynT_] {
		foreach tr $dynT_ {
			$tr annotate $args
		}
	}
}



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
	$self instvar encaps_ ;# lists of encapsulators by group for a given node
	$self instvar misses_ ;#mostly for debugging
	$self instvar graft_  ;# graft agent
	$self instvar prune_  ;# prune agent (in fact, one can handle both)
	
	set ns_ $sim
	set id [$node id]

	set graft_ [new Agent/Mcast/Control $self]
	set prune_ [new Agent/Mcast/Control $self]
	$node attach $graft_
	$node attach $prune_
	
	set misses_ 0         ;# initially there's been no misses
	
	$graft_ set class_ 30
	$prune_ set class_ 31

	foreach grp $Groups_ {
		foreach agent [$node set agents_] {
			if {[expr $grp] == [expr [$agent set dst_]]}  {
				#found an agent that's sending to a group.
				#need to add Encapsulator
				puts "attaching a Encapsulator for group $grp, node $id"
				set e [new Agent/Encapsulator]
				$e set class_ 32
				$e set status_ 1
				$e decap-target ""
				$node attach $e
				lappend encaps_($grp) $e

#				set r [new Classifier/Replicator/Demuxer]
#				$r set srcID_ $id
#				$r set grp_ $grp
#				$r set node_ $node
#				$r insert [$node entry]
#				$r insert $e
#				$agent target $r
				$agent target $e

#				$node add-mfc $id $grp -1 ""
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
			#	    $d set star_value_ [ST set star_value_]
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

ST instproc join-group  { group } {
    $self next $group
    $self instvar node_
    ST instvar RP_

    
    set r [$node_ getReps "*" $group]

    if {$r == "" || ![$r is-active]} {
	$self send-ctrl "graft" $RP_($group) $group
    }
}

ST instproc leave-group { group } {
    ST instvar RP_
    $self next $group
    if {$group != $RP_($group)} {
	$self send-ctrl "prune" $RP_($group) $group
    }
}

ST instproc handle-wrong-iif { srcID group iface } {
    $self instvar node_
    puts "ST: wrong iif at node [$node_ id], src $srcID, grp $group"
}
# It's bad that in handle-cache-miss it's srcID and not a full address!
ST instproc handle-cache-miss { srcID group iface } {
    $self instvar node_
    ST instvar RP_
    $self instvar misses_

    #there should be only one cache-miss
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

ST instproc drop { replicator src dst } {
    $self instvar node_ ns_
    ST instvar RP_

    # No downstream listeners
    # Send a prune back toward the source
    puts "drop src: $src, dst: $dst"

    if { $src == [$node_ id] } { 
	#
	# if we are trying to prune ourself (i.e., no
	# receivers anywhere in the network), set the
	# ignore bit in the object (we turn it back on
	# when we get a graft).  This prevents this
	# drop methood from being called on every packet.
	#
#	$replicator set ignore_ 1
    } else {
	if {$node_ != $RP_($dst)} {
	    set ph [$ns_ upstream-node [$node_ id] [$RP_($dst) id]]
	    $self send-ctrl "prune" $ph $dst
	} else {
#	    $replicator set ignore_ 1
	}
    }
    $self instvar dynT_
    if [info exists dynT_] {
	foreach tr $dynT_ {
	    $tr annotate "[$ns_ now] [$node_ id] dropping a packet from $src to $dst"
	}
    }
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
	warn {trying to prune a non-existing interface?}
    } else {
	$r instvar active_
	if $active_($tmpoif) {
	    $r disable $tmpoif
	}
	# If there are no remaining active output links
	# then send a prune upstream.
	$r instvar nactive_
	if {$nactive_ == 0 && $node_ != $RP_($group)} {
	    $self send-ctrl "prune" $RP_($group) $group
 	}
    }
}

Link instproc if-label? {} {
	$self instvar iif_
	$iif_ label
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
    if { ![$r is-active] && $node_ != $RP_($group)} {
	# if this node isn't on the tree and isn't RP, propagate the graft
	$self send-ctrl "graft" $RP_($group) $group
    }
    # graft on the interface
    set tmpoif [[$ns_ link $id $from] head]
    $r insert $tmpoif
#    $r set ignore_ 0
}

#
# send a graft/prune for src/group up the RPF tree
#
ST instproc send-ctrl { which dst group } {
    ST instvar RP_
    $self instvar graft_ prune_ ns_ node_

    set id [$node_ id]
    set next_hop [$ns_ upstream-node $id [$dst id]]
    set dest $next_hop

#    if { $node_ == $RP_($group) } {
#	# RP sends multihop prunes towards the source
#	set dest $dst
#    }
#    puts "[$node_ id] is sending a $which to [$dest id]"
    switch $which {
	graft {
	    $ns_ simplex-connect $graft_ \
		    [[[$dest getArbiter] getType [$self info class]] set graft_]
	    $graft_ send $which $id [$dst id] $group
	} 
	prune {
	    $ns_ simplex-connect $prune_ \
		    [[[$dest getArbiter] getType [$self info class]] set prune_]
	    $prune_ send $which $id [$dst id] $group
	}
	default {
	    puts "ST instproc send-ctrl: unsupported ctrl message type"
	}
    }
}

ST instproc dump-routes args {
	
}

# Agent/Mcast/Prune instproc init { protocol } {
#     $self next
#     $self instvar proto_
#     set proto_ $protocol
# }
# Agent/Mcast/Graft instproc init { protocol } {
#     $self next
#     $self instvar proto_
#     set proto_ $protocol
# }

# Agent/Mcast/Prune instproc handle {type from src group } {
#  	$self instvar proto_ 
# #        puts "Agent/Mcast/Prune instproc handle $type $from $src $group"
#          eval $proto_ recv-$type $from $src $group 
# }
# Agent/Mcast/Graft instproc handle {type from src group } {
#  	$self instvar proto_ 
# #        puts "Agent/Mcast/Graft instproc handle $type $from $src $group"
#          eval $proto_ recv-$type $from $src $group
# }

#####



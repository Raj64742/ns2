 #
 # tcl/mcast/ns-mcast.tcl
 #
 # Copyright (C) 1997 by USC/ISI
 # All rights reserved.                                            
 #                                                                
 # Redistribution and use in source and binary forms are permitted
 # provided that the above copyright notice and this paragraph are
 # duplicated in all such forms and that any documentation, advertising
 # materials, and other materials related to such distribution and use
 # acknowledge that the software was developed by the University of
 # Southern California, Information Sciences Institute.  The name of the
 # University may not be used to endorse or promote products derived from
 # this software without specific prior written permission.
 # 
 # THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 # WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 # MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 # 
 # Ported by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 # 
 #
###############

# The MultiSim stuff below is only for backward compatibility.
Class MultiSim -superclass Simulator

MultiSim instproc init args {
        eval $self next $args
        Simulator set EnableMcast_ 1
}

Simulator instproc run-mcast {} {
        $self instvar Node_
        foreach n [array names Node_] {
                set node $Node_($n)
                        $node init-outLink
                        $node start-mcast
        }
        $self next
}

Simulator instproc clear-mcast {} {
        $self instvar Node_
        foreach n [array names Node_] {
                set node $Node_($n)
                $node stop-mcast
        }
}

Simulator instproc upstream-node { id src } {
	$self instvar routingTable_ Node_
        if [info exists routingTable_] {
		set nbr [$routingTable_ lookup $id $src]
		if [info exists Node_($nbr)] {
			return $Node_($nbr)
		} else {
			return ""
		}
	} else {
	    return ""
	}
}

Simulator instproc RPF-link { src from to } {
	$self instvar routingTable_ link_
	#
	# If this link is on the RPF tree, return the link object.
	#
        if [info exists routingTable_] {
	    set reverse [$routingTable_ lookup $to $src]
	    if { $reverse == $from } {
		return $link_($from:$to)
	    }
	    return ""
	}
	return ""
}

Simulator instproc getNodeIDs {} {
        $self instvar Node_
        return [array names Node_]
}

Simulator instproc setPIMProto { index proto } {
        $self instvar pimProtos
        set pimProtos($index) $proto
}

Simulator instproc getPIMProto { index } {
        $self instvar pimProtos
        if [info exists pimProtos($index)] {
                return $pimProtos($index)
        }
        return -1
}

Simulator instproc mrtproto {mproto nodeList} {
    $self instvar Node_ MrtHandle_

    set MrtHandle_ ""
    if {$mproto == "CtrMcast"} {
	set MrtHandle_ [new CtrMcastComp $self]
	$MrtHandle_ set ctrrpcomp [new CtrRPComp $self]
	if {[llength $nodeList] == 0} {
		foreach n [array names Node_] {
		    new $mproto $self $Node_($n) 0 [list]
		}
	} else {
		foreach node $nodeList {
		    new $mproto $self $node 0 [list]
		}
	}
    } else {
        if {[llength $nodeList] == 0} {
                foreach n [array names Node_] {
                    new $mproto $self $Node_($n)
                }
        } else {
                foreach node $nodeList {
                    new $mproto $self $node
                }
        }
    }
    $self at 0.0 "$self run-mcast"
    return $MrtHandle_
}

###############

Node proc allocaddr {} {
	# return a unique mcast address
	set addr [Simulator set McastAddr_]
	Simulator set McastAddr_ [expr $addr + 1]
	return $addr
}

Node proc expandaddr {} {
	# reset the bit used by mcast/unicast switch to expand
	# number of nodes that can be used
	# Simulator set McastShift_ 30
        
        # calling set-address-format with expanded option (sets nodeid with 21 bits 
        # & sets aside 1 bit for mcast) and sets portid with 8 bits

        set ns [Simulator instance]
        $ns set-address-format expanded
	
	set mcastshift [AddrParams set McastShift_]
	Simulator set McastAddr_ [expr 1 << mcastshift]
	#Simulator set McastAddr_ [expr 1 << 30]

	McastProtoArbiter expandaddr
}

Node instproc notify-mcast changes {
	$self instvar mcastproto_

	# XXX quick hack
	if [info exists mcastproto_] {
		$mcastproto_ notify $changes
	}
}

Node instproc stop-mcast {} {
        $self instvar mcastproto_
        $self clear-caches
        $mcastproto_ stop
}

Node instproc clear-caches {} {
        $self instvar Agents_ repByGroup_ multiclassifier_ replicator_

        $multiclassifier_ clearAll
	$multiclassifier_ set nrep_ 0

        if [info exists repByGroup_] {
                unset repByGroup_
        }
        if [info exists Agents_] {
                unset Agents_
        }
        if [info exists replicator_] {
                unset replicator_
        }
        # XXX watch out for memory leaks
}

Node instproc start-mcast {} {
        $self instvar mcastproto_
        $mcastproto_ start
}

Node instproc getArbiter {} {
        $self instvar mcastproto_
	if [info exists mcastproto_] {
	        return $mcastproto_
	}
	return ""
}

Node instproc check-local { group } {
        $self instvar Agents_
        if [info exists Agents_($group)] {
                return [llength $Agents_($group)]
        }
        return 0
}

Node instproc get-oifs {} {
        $self instvar outLink_
        set oiflist ""
        foreach oif [array names outLink_] {
                lappend oiflist $oif
        }
        return $oiflist
}

Node instproc get-oif { link } {
        set oif ""
        # XXX assume we are connected to interfaces
        lappend oif [[$link set ifacein_] set intf_label_]
        lappend oif [$link set ifacein_]
        return $oif
}

Node instproc get-oifIndex { node_id } {
        $self instvar ns_ id_
        # XXX assume link head is iface, for simplicity
        set link [$ns_ set link_($id_:$node_id)]
        return [[$link set ifacein_] set intf_label_]
}

Node instproc label2iface { label } {
        $self instvar outLink_
        return $outLink_($label)
}

Node instproc RPF-interface { src from to } {
        $self instvar ns_
        set oifInfo ""  
        set link [$ns_ RPF-link $src $from $to]

        if { $link != "" } {
                set oifInfo [$self get-oif $link]
        }
        return $oifInfo
}

Node instproc ifaceGetNode { iface } {
        $self instvar ns_ id_ neighbor_
        foreach node $neighbor_ {
                set link [$ns_ set link_([$node id]:$id_)]
	    if {[[$link set ifaceout_] id] == $iface} {
		return $node
	    }
        }
	return -1
}

Node instproc init-outLink { } {
        $self instvar outLink_ neighbor_ id_ ns_
        foreach node $neighbor_ {
                set link [$ns_ set link_($id_:[$node id])]
                set oifInfo [$self get-oif $link]
                set index [lindex $oifInfo 0]
                set outLink_($index) [lindex $oifInfo 1]
        }
}

Node instproc getRepByGroup { group } {
        $self instvar repByGroup_
        if [info exists repByGroup_($group)] {
                return $repByGroup_($group)   
        }
        return ""
}

Node instproc getRep { src group } {
	$self instvar replicator_
	if [info exists replicator_($src:$group)] {
		return $replicator_($src:$group)
	}
	return ""
}

Node instproc getRepBySource { src } {
        $self instvar replicator_
	  
	  set replist ""
	  foreach n [array names replicator_] {
		set pair [split $n :]
		if {[lindex $pair 0] == $src} {
			lappend replist [lindex $pair 1]:$replicator_($n)
		}
	  }
	  return $replist
}

Node instproc exists-Rep { src group } {
    $self instvar replicator_

    if [info exists replicator_($src:$group)] {
        return 1
    } else {
        return 0
    }
}

Node instproc join-group { agent group } {
    # use expr to get rid of possible leading 0x
    set group [expr $group]
    $self instvar Agents_ repByGroup_ agentSlot_ mcastproto_

    $mcastproto_ join-group $group
    lappend Agents_($group) $agent
    if [info exists repByGroup_($group)] {
	#
	# make sure agent is enabled in each replicator
	# for this group
	#
	foreach r $repByGroup_($group) {
	    $r insert $agent
	}
    }
}

Node instproc leave-group { agent group } {
    # use expr to get rid of possible leading 0x
    set group [expr $group]
    $self instvar repByGroup_ Agents_ mcastproto_

    if [info exists repByGroup_($group)] {
	    foreach r $repByGroup_($group) {
		$r disable $agent
	    }
    }
    if [info exists Agents_($group)] {
	set k [lsearch -exact $Agents_($group) $agent]
	if { $k >= 0 } {
	    set Agents_($group) [lreplace $Agents_($group) $k $k]
	}
	## inform the mcastproto agent
	$mcastproto_ leave-group $group
    } else {
	puts stderr "error: leaving a group without joining it"
	exit 0
    }
}

Node instproc join-group-source { agent group source } {
        set group [expr $group]
        set source [expr $source]

       $self instvar id_

        $self instvar Agents_ mcastproto_ replicator_
        ## send a message for the mcastproto agent to inform the mcast protocols
        $mcastproto_ join-group-source $group $source
        lappend Agents_($source:$group) $agent
        if [info exists replicator_($source:$group)] {
                $replicator_($source:$group) insert $agent
        }
}

Node instproc leave-group-source { agent group source } {
        set group [expr $group]
        set source [expr $source]
        $self instvar replicator_ Agents_ mcastproto_
        if [info exists replicator_($source:$group)] {
                $replicator_($source:$group) disable $agent
        }
        $mcastproto_ leave-group-source $group $source
}

Node instproc new-group { src group iface code } {
    $self instvar mcastproto_
	
    $mcastproto_ upcall [list $code $src $group $iface]
}

Node instproc add-mfc { src group iif oiflist } {
    $self instvar multiclassifier_ \
	    replicator_ Agents_ repByGroup_ 

    if [info exists replicator_($src:$group)] {
	foreach oif $oiflist {
	    $replicator_($src:$group) insert $oif
	}
	return 1
    }

    set r [new Classifier/Replicator/Demuxer]
    $r set srcID_ $src
    $r set grp_ $group
    set replicator_($src:$group) $r

    lappend repByGroup_($group) $r
    $r set node_ $self

    foreach oif $oiflist {
	$r insert $oif
    }

    #
    # install each agent that has previously joined this group
    #
    if [info exists Agents_($group)] {
	foreach a $Agents_($group) {
	    $r insert $a
	}
    }
    # we also need to check Agents($srcID:$group)
    if [info exists Agents_($src:$group)] {
            foreach a $Agents_($src:$group) {
                    $r insert $a
            }
    }
    #
    # Install the replicator.  
    #
    $multiclassifier_ add-rep $r $src $group $iif
}

Node instproc del-mfc { srcID group oiflist } {
        $self instvar replicator_ multiclassifier_
        if [info exists replicator_($srcID:$group)] {
                set r $replicator_($srcID:$group)  
                foreach oif $oiflist {
                        $r disable $oif
                }
                return 1
        } 
        return 0
}

####################
Class Classifier/Multicast/Replicator -superclass Classifier/Multicast

#
# This method called when a new multicast group/source pair
# is seen by the underlying classifier/mcast object.
# We install a hash for the pair mapping it to a slot
# number in the classifier table and point the slot
# at a replicator object that sends each packet along
# the RPF tree.
#
Classifier/Multicast instproc new-group { src group iface code} {
	$self instvar node_
	$node_ new-group $src $group $iface $code
}

Classifier/Multicast instproc no-slot slot {
	#XXX should say something better for routing problem
	#puts stderr "$self: no target for slot $slot"
	#exit 1
}

Classifier/Multicast/Replicator instproc init args {
	$self next
	$self instvar nrep_
	set nrep_ 0
}

Classifier/Multicast/Replicator instproc add-rep { rep src group iif } {
	$self instvar nrep_
	$self set-hash $src $group $nrep_ $iif
	$self install $nrep_ $rep
	incr nrep_
}

###################### Class Classifier/Replicator/Demuxer ##############
Class Classifier/Replicator/Demuxer -superclass Classifier/Replicator


Classifier/Replicator set ignore_ 0

Classifier/Replicator/Demuxer instproc init args {
	eval $self next $args
	$self instvar nslot_ nactive_
	set nslot_ 0
	set nactive_ 0
}

Classifier/Replicator/Demuxer instproc is-active {} {
	$self instvar nactive_
	return [expr $nactive_ > 0]
}

Classifier/Replicator/Demuxer instproc insert target {
	$self instvar nslot_ nactive_ active_ index_ ignore_

        if [info exists active_($target)] {
                # treat like enable.. !   
                if !$active_($target) {    
                        $self install $index_($target) $target
                        incr nactive_
                        set active_($target) 1
                        set ignore_ 0
                        return 1
                }
                return 0
        }

	set n $nslot_
	incr nslot_
	incr nactive_
	$self install $n $target
	set active_($target) 1
	set ignore_ 0
	set index_($target) $n
}

Classifier/Replicator/Demuxer instproc disable target {
	$self instvar nactive_ active_ index_
	if ![info exists active_($target)] {
		return 0
	}
	if $active_($target) {
		$self clear $index_($target)
		incr nactive_ -1
		set active_($target) 0
	}
}

Classifier/Replicator/Demuxer instproc enable target {
	$self instvar nactive_ active_ ignore_ index_
	if !$active_($target) {
		$self install $index_($target) $target
		incr nactive_
		set active_($target) 1
		set ignore_ 0
	}
}

Classifier/Replicator/Demuxer instproc exists target {
	$self instvar active_
	return [info exists active_($target)]
}

Classifier/Replicator/Demuxer instproc drop { src dst } {
	set src [expr $src >> 8]
	$self instvar node_
        if [info exists node_] {
	    [$node_ set mcastproto_] drop $self $src $dst
	}
        return 1
}

Classifier/Replicator/Demuxer instproc change-iface { src dst oldiface newiface} {
	$self instvar node_
        [$node_ set multiclassifier_] change-iface $src $dst $oldiface $newiface
        return 1
}


#----------------------------------------------------------------------
# Generating multicast distribution trees 
#----------------------------------------------------------------------

# 
# get the adjacent node thru a outgoing interface
# NOTE: Node::ifaceGetNode() in ns-mcast.tcl get a node through an 
# incoming interface, which is different from the one below.
#
Node instproc oifGetNode { iface } {
        $self instvar ns_ id_ neighbor_
        foreach node $neighbor_ {
                set link [$ns_ set link_($id_:[$node id])]
		if {[$link set ifacein_] == $iface} {
			return $node
		}
        }
	return -1
}

# given an iif, find oifs in (S,G)
Node instproc getRepByIIF { src group iif } {
	$self instvar multiclassifier_
	return [$multiclassifier_ lookup-iface [$src id] $group $iif]
}

Simulator instproc find-next-child { parent sl } {
	puts "Found $sl ([$sl info class]) for node [$parent id]"
	set cls [$sl info class]

	if {$cls == "networkinterface"} {
		set child [$parent oifGetNode $sl]
	} elseif {$cls == "DuplexNetInterface"} {
		set child [$sl getNode]
	} else {
		set basecls [lindex [split $cls /] 0]
		if {$basecls == "Agent"} {
			return ""
		} else {
			error "Unsupported interface type: [$sl info class]"
		}
	}
	if {$child == -1} {
		puts "Outgoing iface doesn't have corresponding nodes"
		return ""
	}
	return $child
}

#
# XXX
# (1) Cannot start from a source in a RP tree, because there a packet is 
#     unicast to the RP so there isn't any multicast entry at the source.
# (2) src is a Node
# (3) Currently NOT working for detailedDM
#
Simulator instproc get-mcast-tree { src grp } {
	$self instvar link_ treeLinks_

	# iif == -2: from local
	set tmp [$src getRepByIIF $src $grp -2]
	if {$tmp == ""} {
		$self flush-trace
		error "No replicator for $GROUP_ at [$src id]"
	}

	set sid [$src id]
	lappend repList $tmp
	lappend nodeList $src

	while {[llength $repList] > 0} {
		set h [lindex $repList 0]
		set parent [lindex $nodeList 0]
		set pid [$parent id]
		set closeNodes($pid) 1
		set repList [lreplace $repList 0 0]
		set nodeList [lreplace $nodeList 0 0]

		set slots [$h slots]
#		puts "$slots"
		foreach sl $slots {
			set child [$self find-next-child $parent $sl]
			if {$child == ""} {
				puts "No children found for ($parent, $sl)"
				continue
			}
			set cid [$child id]
			if [info exists closeNodes($cid)] {
				error "Loop: node $cid already in the tree"
			}

			# shouldn't go upstream
			if ![info exists link_($pid:$cid)] {
				error "Found non-existent link ($pid:$cid)";
			}

			lappend nodeList $child
			puts "Link ($pid:$cid) found."
			lappend treeLinks $pid:$cid $link_($pid:$cid)
			set treeLinks_($pid:$cid) $link_($pid:$cid)

#			debug 1
			set iif [[$link_($pid:$cid) set ifaceout_] id]
			if {$iif == -1} {
				puts "iif == -1"
				debug 1
			}
			lappend repList [$child getRepByIIF $src $grp $iif]
		}
	}
	return $treeLinks
}

# XXX assume duplex link, also assume this is called after ns starts
Simulator instproc color-tree {} {
	$self instvar treeLinks_ Node_

	# color tree links
	foreach l [array names treeLinks_] {
		set tmp [split $l :]
		set sid [lindex $tmp 0]
		set did [lindex $tmp 1]
		$self duplex-link-op $Node_($sid) $Node_($did) color blue
	}
}

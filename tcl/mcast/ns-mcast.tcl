###############
Class MultiSim -superclass Simulator

MultiSim instproc run-mcast {} {
        $self instvar Node_
        #puts "run mcast... "
        foreach n [array names Node_] {
                set node $Node_($n)
#               if { [$node info class] == "MultiNode" } {
                        $node init-outLink
                        #puts "Node $n init-outLink done"
                        $node start-mcast
#               }
        }
        $self next
}

MultiSim instproc upstream-node { id src } {
	$self instvar routingTable_ Node_
	set nbr [$routingTable_ lookup $id $src]
	return $Node_($nbr)
}

MultiSim instproc RPF-link { src from to } {
	$self instvar routingTable_ link_
	#
	# If this link is on the RPF tree, return the link object.
	#
	set reverse [$routingTable_ lookup $to $src]
	if { $reverse == $from } {
		return $link_($from:$to)
	}
	return ""
}

MultiSim instproc node {} {
	$self instvar Node_
	set node [new MultiNode $self]
	set Node_([$node id]) $node
	return $node
}

MultiSim instproc getNodeIDs {} {
        $self instvar Node_
        return [array names Node_]
}

MultiSim instproc setPIMProto { index proto } {
        $self instvar pimProtos
        set pimProtos($index) $proto
}

MultiSim instproc getPIMProto { index } {
        $self instvar pimProtos
        if [info exists pimProtos($index)] {
                return $pimProtos($index)
        }
        return -1
}


###############
Class MultiNode -superclass Node

MultiNode instproc entry {} {
	$self instvar switch_
	return $switch_
}

MultiNode instproc init sim {
	eval $self next
	$self instvar classifier_ multiclassifier_ switch_ ns_ mcastproto_
	set ns_ $sim

	set switch_ [new Classifier/Addr]
	#
	# set up switch to route unicast packet to slot 0 and
	# multicast packets to slot 1
	#
	$switch_ set mask_ 1
	$switch_ set shift_ 15

	#
	# create a classifier for multicast routing
	#
	set multiclassifier_ [new Classifier/Multicast/Replicator]
	$multiclassifier_ set node_ $self

	$switch_ install 0 $classifier_
	$switch_ install 1 $multiclassifier_

	#
	# Create a prune agent.  Each multicast routing node
	# has a private prune agent that sends and receives
	# prune/graft messages and dispatches them to the
	# appropriate replicator object.
	#

        set mcastproto_ [new McastProtoArbiter ""]
}

MultiNode instproc start-mcast {} {
        $self instvar mcastproto_
        $mcastproto_ start
}

MultiNode instproc getArbiter {} {
        $self instvar mcastproto_
        return $mcastproto_
}

MultiNode instproc check-local { group } {
        $self instvar Agents_
        if [info exists Agents_($group)] {
                return [llength $Agents_($group)]
        }
        return 0
}

MultiNode instproc get-oifs {} {
        $self instvar outLink_
        set oiflist ""
        foreach oif [array names outLink_] {
                lappend oiflist $oif
        }
        return $oiflist
}

MultiNode instproc get-oif { link } {
        $self instvar id_
        set oif ""
        # check if this is a dummy link
         if { [$link info class] == "DummyLink" } {
                # then this is part of a LAN
                set container [$link getContainingObject]
                if { [$container info class] == "NonReflectingMultiLink" } {
                        lappend oif [[lindex [$container set nodes_] 0] id]
                        lappend oif [$container getReplicator $id_]
                }
        } else {
                lappend oif [[$link set toNode_] id]
                lappend oif [$link head]
        }
        return $oif
}

MultiNode instproc get-oifIndex { node_id } {
        $self instvar ns_ id_
        set link [$ns_ set link_($id_:$node_id)]
        if { [$link info class] == "DummyLink" } {
                return [[lindex [[$link getContainingObject] set nodes_] 0] id]
        }
        return [[$link set toNode_] id]
}

MultiNode instproc RPF-interface { src from to } {
        $self instvar ns_
        set oifInfo ""  
        set link [$ns_ RPF-link $src $from $to]
        if { $link != "" } {
                set oifInfo [$self get-oif $link]
        }
        return $oifInfo
}

MultiNode instproc init-outLink { } {
        $self instvar outLink_ neighbor_ id_ ns_
        foreach node $neighbor_ {
                set link [$ns_ set link_($id_:[$node id])]
                set oifInfo [$self get-oif $link]
                set index [lindex $oifInfo 0]
                set outLink_($index) [lindex $oifInfo 1]
        }
}

MultiNode instproc getRepByGroup { group } {
        $self instvar repByGroup_
        if [info exists repByGroup_($group)] {
                return $repByGroup_($group)   
        }
        return ""
}

MultiNode instproc getRep { src group } {
	$self instvar replicator_
	if [info exists replicator_($src:$group)] {
		return $replicator_($src:$group)
	}
	return ""
}

MultiNode instproc exists-Rep { src group } {
    $self instvar replicator_

    if [info exists replicator_($src:$group)] {
        return 1
    } else {
        return 0
    }
}

MultiNode instproc join-group { agent group } {
    # use expr to get rid of possible leading 0x
    set group [expr $group]
    $self instvar Agents_ repByGroup_ agentSlot_ mcastproto_

    $mcastproto_ join-group $group
    lappend Agents_($group) $agent
    if [info exists repByGroup_($group)] {
	#
	# there are active replicators on this group
	# see if any are idle, and if so send a graft
	# up the RPF tree for the respective S,G
	#
	foreach r $repByGroup_($group) {
	    if ![$r exists $agent] {
		$r insert $agent
	    }
	}
	#
	# make sure agent is enabled in each replicator
	# for this group
	#
	foreach r $repByGroup_($group) {
	    $r enable $agent
	}
    }
}

MultiNode instproc leave-group { agent group } {
    # use expr to get rid of possible leading 0x
    set group [expr $group]
    $self instvar repByGroup_ Agents_ mcastproto_

    if ![info exists repByGroup_($group)] {
	return 0
    }
    #XXX info exists 
    foreach r $repByGroup_($group) {
	$r disable $agent
    }
    set k [lsearch -exact $Agents_($group) $agent]
    if { $k >= 0 } {
	set Agents_($group) [lreplace $Agents_($group) $k $k]
    }

    ## inform the mcastproto agent

    $mcastproto_ leave-group $group
    
}

MultiNode instproc new-group { src group iface code } {
    $self instvar mcastproto_
	
    # node addr is in upper 24 bits
    # set srcID [expr $src >> 8]

    # puts "node $self id [$self id] upcall [list $code $src $group $iface]"
    $mcastproto_ upcall [list $code $src $group $iface]
}

MultiNode instproc add-mfc { src group iif oiflist } {
    $self instvar multiclassifier_ \
	    replicator_ Agents_ repByGroup_ 

    #XXX node addr is in upper 24 bits

    if [info exists replicator_($src:$group)] {
	foreach oif $oiflist {
	    $replicator_($src:$group) insert $oif
	}
	return 1
    }

    set r [new Classifier/Replicator/Demuxer]
    $r set srcID_ $src
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
    
    #
    # Install the replicator.  We do this only once and leave
    # it forever.  Since prunes are data driven, we want to
    # leave the replicator in place even when it's empty since
    # the replicator::drop callback triggers the prune.
    #
    $multiclassifier_ add-rep $r $src $group $iif
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


Classifier/Replicator set ignore 0

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
	$self instvar nslot_ nactive_ active_ index_

        if [info exists active_($target)] {
                # treat like enable.. !   
                if !$active_($target) {    
                        $self install $index_($target) $target
                        incr nactive_
		        puts "$self $nactive_ (+1)"
                        set active_($target) 1
                        set ignore 0
                        return 1
                }
                return 0
        }

	set n $nslot_
	incr nslot_
	incr nactive_
	$self install $n $target
	set active_($target) 1
	set index_($target) $n
}

Classifier/Replicator/Demuxer instproc disable target {
	$self instvar nactive_ active_ index_
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
	#
	# No downstream listeners
	# Send a prune back toward the source
	#
	set src [expr $src >> 8]
	$self instvar node_
        [$node_ set mcastproto_] drop $self $src $dst
        return 1
}


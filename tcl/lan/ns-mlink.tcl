Class MultiLink
MultiLink set num 0

MultiLink proc getid {} {
	set id [MultiLink set num]
	MultiLink set num [expr $id + 1]
	return $id
}

MultiLink instproc init { listOfNodes bwidth del tp } { 
	# create a LAN with a list of nodes attached
	$self next
	$self instvar nodes_ bw_ delay_ type_ id_
	set id_ [MultiLink getid]
	set nodes_ $listOfNodes
	set bw_ $bwidth
	set delay_ $del
	set type_ $tp
	$self attachNodes
}

MultiLink instproc id {} {
	$self instvar id_
	return $id_
}

MultiLink instproc attachNodes {} {
	$self connectNodes
	$self createReplicators
}

Class PhysicalMultiLink -superclass MultiLink

PhysicalMultiLink instproc init { lOConnects bwidth del tp } {
        $self instvar lONodes_ listOfConnects_
	set listOfConnects_ $lOConnects
        set lONodes_ ""
        # assume the list is of the same types.. 
        set x [lindex $listOfConnects_ 0]
        set k [lsearch -exact [[$x info class] info heritage] "NetInterface"]
        if { [$x info class] == "NetInterface" || $k >= 0 } {
                foreach n $listOfConnects_ {
                        lappend lONodes_ [$n getNode]
                }
        } else {
                set lONodes_ $listOfConnects_
        }
        $self next $lONodes_ $bwidth $del $tp
}

PhysicalMultiLink instproc connectNodes {} {
	$self instvar listOfConnects_ queues_ delays_ type_ bw_ delay_
	foreach n $listOfConnects_ {
	# create a Q for each node, attach a delay model for the Q and attach
	# the delay to the node's `entry', which may be either an interface,
	# or a switch
		set q [new Queue/$type_] 
		set queues_([[$n getNode] id]) $q
		set delayModel [new DelayLink] 
		set delays_([[$n getNode] id]) $delayModel
		$delayModel set bandwidth_ $bw_
		$delayModel set delay_ $delay_
		$q target $delayModel
		$delayModel target [$n entry]
		$self add-neighbors [$n getNode]
	}
}

PhysicalMultiLink instproc add-neighbors node {
	$self instvar nodes_
	set id [$node id]
	# add all other nodes in the nodes list to the node passed
	foreach n $nodes_ {
		if { [$n id] != $id } {
			$n add-neighbor $node 
		}
	}
}

PhysicalMultiLink instproc createReplicators {} {
	$self instvar replicator_ queues_
	set replicator_ [new Classifier/Replicator/Demuxer]
	foreach index [array names queues_] {
		set q $queues_($index)
		$replicator_ insert $q
	}
}	

PhysicalMultiLink instproc getQueue node {
	$self instvar queues_
	return $queues_([$node id])
}

PhysicalMultiLink instproc getDelay node {
	$self instvar delays_
	return $delays_([$node id])
}

PhysicalMultiLink instproc getReplicator { node_id } {
	$self instvar replicator_
	return $replicator_
}

Class NonReflectingMultiLink -superclass PhysicalMultiLink

NonReflectingMultiLink instproc init { lOConnects bwidth del tp } {
        $self next $lOConnects $bwidth $del $tp
}

NonReflectingMultiLink instproc createReplicators {} {
        $self instvar replicators_ queues_ nodes_
	foreach n $nodes_ {
		set nid [$n id]
		set r [new Classifier/Replicator/Demuxer]
		set replicators_($nid) $r
		foreach n2 $nodes_ {
			set n2id [$n2 id]
			if { $n2id != $nid } {
				$r insert $queues_($n2id)
			}
		}
	}
}	

NonReflectingMultiLink instproc getReplicator { node_id } { 
        $self instvar replicators_
        return $replicators_($node_id)
}



#
#Polly: Dummy Link, lan stuff
#
Class DummyLink -superclass Link
DummyLink instproc init { src dst q del } {
        $self next $src $dst
	$self instvar head_ queue_ link_
#	$self instvar trace_ fromNode_ toNode_


#	set fromNode_ [$src getNode]
#	set toNode_ [$dst getNode]
#	set trace_ ""
	set queue_ $q
	set link_ $del
	set head_ $queue_
}

DummyLink instproc setContainingObject obj {
	$self instvar containingObj_
	set containingObj_ $obj
}

DummyLink instproc getContainingObject { } {
	$self instvar containingObj_
	return $containingObj_
}

DummyLink instproc traceLan { ns f } {
        $self next $ns $f
        $self instvar containingObj_ head_ queue_ fromNode_
        set r [$containingObj_ getReplicator [$fromNode_ id]]
        $r disable $queue_
        $r insert $head_
}

DummyLink instproc trace { ns f } {
        $self instvar queue_ head_ fromNode_ toNode_
        set deqT_ [$ns create-trace Deque $f $fromNode_ $toNode_]
        set drpT_ [$ns create-trace Drop $f $fromNode_ $toNode_]
        $drpT_ target [$ns set nullAgent_]
        $deqT_ target $queue_
        $queue_ drop-target $drpT_
        set head_ $deqT_
        $self traceLan $ns $f
}

 #
 # tcl/interface/ns-mlink.tcl
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
# Dummy Link, lan stuff
#
Class DummyLink -superclass Link

# XXX this only works with ifaces... for simplicity.. !!
DummyLink instproc init { src dst q del mlink } {
        $self next $src $dst
        $self instvar head_ queue_ link_ ifacein_ rep_ fromNode_ ifaceout_
        $self setContainingObject $mlink
        set rep_ [$mlink getReplicator [$fromNode_ id]]
        set queue_ $q
        set link_ $del
        set ifacein_ [$src exitpoint]
	set ifaceout_ $dst
        # XXX need ifacein to mcast
        $ifacein_ target $rep_
        # XXX we need head to be Q for unicast not to loop !!
        # unicast loops if it goes to replicator
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
        
DummyLink instproc trace { ns f } {
        $self instvar queue_ fromNode_ toNode_
        set deqT_ [$ns create-trace Deque $f $fromNode_ $toNode_]
        set drpT_ [$ns create-trace Drop $f $fromNode_ $toNode_]
        $drpT_ target [$ns set nullAgent_]
        $deqT_ target $queue_
        $queue_ drop-target $drpT_
 
        $self instvar rep_ head_
        $rep_ disable $queue_
        $rep_ insert $deqT_
        set head_ $deqT_
}

DummyLink instproc addloss { lossObject } {
        $self instvar lossObj
        set lossObj $lossObject
        $self instvar rep_ head_
        $lossObj target $head_
        $rep_ disable $head_
        set head_ $lossObj
        $rep_ insert $head_
}


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

# XXX Bad hack. Because nam require lan and nodes share the same id space, 
# but ns doesn't support virtual node, we start lan id from 10000000
MultiLink set num 10000000
MultiLink proc getid {} {
	set id [MultiLink set num]
	MultiLink set num [expr $id + 1]
	return $id
}

#
# Lan and Node share the same id space, because nam needs it.
#
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

MultiLink instproc getLanBW {} {
	$self instvar bw_
	return $bw_
}

MultiLink instproc getLanDelay {} {
	$self instvar delay_
	return $delay_
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
	$self instvar listOfConnects_ queues_ delays_ type_ bw_ delay_ \
		drophead_

	foreach n $listOfConnects_ {
	# create a Q for each node, attach a delay model for the Q and attach
	# the delay to the node's `entry', which may be either an interface,
	# or a switch
		set nid [[$n getNode] id]
		set q [new Queue/$type_] 
		set queues_($nid) $q
		set delayModel [new DelayLink] 
		set delays_($nid) $delayModel
		$delayModel set bandwidth_ $bw_
		$delayModel set delay_ $delay_
		$q target $delayModel
		set drophead_($nid) [new Connector]
		$drophead_($nid) target [[Simulator instance] set nullAgent_]
		$q drop-target $drophead_($nid)
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

# Should be called by DummyLink::init because of unicast tracing
PhysicalMultiLink instproc setLink { sid did dumlink } {
	$self instvar links_
	set links_($sid:$did) $dumlink
}

# Insert enqT after each node's exitpoint (an interface)
# Insert deqT after each node's queue
# Insert rcvT after each node's delayModel (XXX assuming no ttl present)
# Insert drpT as the droptarget_ of each queue'
# XXX 
# Must be called after everything is setup. 
PhysicalMultiLink instproc trace { ns f {op ""} } {
	$self instvar enqT_ nodes_ deqT_ listOfConnects_ id_ queues_ delays_ \
		drophead_ links_ drpT_ rcvT_

	if [info exists enqT_] {
		# If already traced, don't do anything
		return
	}
        # assume the list is of the same types.. 
        set x [lindex $listOfConnects_ 0]
        set k [lsearch -exact [[$x info class] info heritage] "NetInterface"]
        if { [$x info class] == "NetInterface" || $k >= 0 } {
		# Multicast, insert enqT_ after each interface
                foreach n $listOfConnects_ {
			set nid [[$n getNode] id]
			set oif [$n exitpoint]
			set enqT_($nid) \
				[$ns create-trace Enque $f $nid $id_ $op]
			$enqT_($nid) target [$oif target]
			$oif target $enqT_($nid)
                }
        }

	# The enqT_ must be inserted into the head of every dummy link
	foreach nl [array names links_] {
		$links_($nl) trace-unicast $ns $f $op
	}

	foreach n $listOfConnects_ {
		set nid_ [[$n getNode] id]
		set deqT_($nid) [$ns create-trace Deque $f $nid $id_ $op]
		$deqT_($nid) target [$queues_($nid) target]
		$queues_($nid) target $deqT_($nid)
		
		set drpT_($nid) [$ns create-trace Drop $f $nid $id_ $op]
		$drpT_($nid) target $drophead_($nid)
		$drophead_($nid) target $drpT_($nid)

		set rcvT_($nid) [$ns create-trace Recv $f $id_ $nid $op]
		$rcvT_($nid) target [$delays_($nid) target]
		$delays_($nid) target $rcvT_($nid)
	}
}

PhysicalMultiLink instproc nam-trace { ns f } {
	$self instvar enqT_ nodes_ deqT_ rcvT_

	if [info exists enqT_] {
		foreach n [array names nodes_] {
			set nid [$n id]
			$enqT_($nid) namattach $f
			$deqT_($nid) namattach $f
			$drpT_($nid) namattach $f
			$rcvT_($nid) namattach $f
		}
		foreach nl [array names links_] {
			$links_($nl) nam-trace-unicast $ns $f
		}
	} else {
		$self trace $ns $f "nam"
	}
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

	# Register this link in the lan to set up unicast tracing
	$mlink setLink [$src id] [$dst id] $self
}

DummyLink instproc setContainingObject obj {
        $self instvar containingObj_
        set containingObj_ $obj
}
        
DummyLink instproc getContainingObject { } {
        $self instvar containingObj_
        return $containingObj_
}

DummyLink instproc trace-unicast { ns f {op ""} } {
	$self instvar head_ enqT_ containingObj_ fromNode_
	set mid [$containingObj_ id]
	set enqT_ [$ns create-trace Enque $f $fromNode_ $mid $op]
        $enqT_ target $head_
	set head_ $enqT_
}

DummyLink instproc nam-trace-unicast { ns f } {
	$self instvar enqT_ 

	if [info exists enqT_] {
		$enqT_ namattach $f
	} else {
		$self trace-unicast $ns $f "nam"
	}
}

DummyLink instproc trace { ns f {op ""} } {
        $self instvar queue_ fromNode_ toNode_ enqT_ deqT_ drpT_
        set enqT_ [$ns create-trace Enque $f $fromNode_ $toNode_ $op]
        set deqT_ [$ns create-trace Deque $f $fromNode_ $toNode_ $op]
        set drpT_ [$ns create-trace Drop $f $fromNode_ $toNode_ $op]
        $drpT_ target [$ns set nullAgent_]
        $deqT_ target $queue_
        $queue_ drop-target $drpT_
        $enqT_ target $deqT_

        $self instvar rep_ head_
        $rep_ disable $queue_
        $rep_ insert $enqT_
        set head_ $enqT_
#        $rep_ insert $deqT_
#        set head_ $deqT_ 
}

DummyLink instproc nam-trace { ns f } {
        $self instvar queue_ fromNode_ toNode_ deqT_ drpT_ enqT_

	# Use deqT_ as a flag of whether tracing has been initialized
	if [info exists deqT_] {
		$deqT_ namattach $f
		if [info exists drpT_] {
			$drpT_ namattach $f
		}
		if [info exists enqT_] {
			$enqT_ namattach $f
		}
	} else {
		$self trace $ns $f "nam"
	}
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

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
###############

# The MultiSim stuff below is only for backward compatibility.
Class MultiSim -superclass Simulator

MultiSim instproc init args {
        eval $self next $args
        $self multicast on
}

Simulator instproc multicast args {
	$self set multiSim_ 1
}

Simulator instproc multicast? {} {
	$self instvar multiSim_
	if { ![info exists multiSim_] } {
		set multiSim_ 0
	}
	set multiSim_
}

Simulator instproc run-mcast {} {
        $self instvar Node_
        foreach n [array names Node_] {
		$Node_($n) start-mcast
        }
        $self next
}

Simulator instproc clear-mcast {} {
        $self instvar Node_
        foreach n [array names Node_] {
                $Node_($n) stop-mcast
        }
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
	return $MrtHandle_
}

Node proc allocaddr {} {
	# return a unique mcast address
	set addr [Simulator set McastAddr_]
	Simulator set McastAddr_ [expr $addr + 1]
	return $addr
}

Node proc expandaddr {} {
	# reset the bit used by mcast/unicast switch to expand
	# number of nodes that can be used
        #Simulator set McastShift_ 30
	#Simulator set McastAddr_ [expr 1 << 30]

        # calling set-address-format with expanded option (sets nodeid with 21 bits 
        # & sets aside 1 bit for mcast) and sets portid with 8 bits
	# if hierarchical address format is set, just expands the McastAddr_
	if ![Simulator set EnableHierRt_] {
		set ns [Simulator instance]
		$ns set-address-format expanded
	}
	set mcastshift [AddrParams set McastShift_]
	Simulator set McastAddr_ [expr 1 << $mcastshift]
	mrtObject expandaddr
}

Node instproc start-mcast {} {
        $self instvar mrtObject_
        $mrtObject_ start
}

Node instproc getArbiter {} {
        $self instvar mrtObject_
	if [info exists mrtObject_] {
	        return $mrtObject_
	}
	return ""
}

Node instproc notify-mcast changes {
	$self instvar mrtObject_
	if [info exists mrtObject_] {
		$mrtObject_ notify $changes
	}
}

Node instproc stop-mcast {} {
        $self instvar mrtObject_
        $self clear-caches
        $mrtObject_ stop
}

Node instproc clear-caches {} {
        $self instvar multiclassifier_

        $multiclassifier_ clearAll
	$multiclassifier_ set nrep_ 0

	foreach var {Agents_ replicator_} {
		$self instvar $var
		if { [info exists $var] } {
			unset $var
		}
	}
        # XXX watch out for memory leaks
}

Node instproc dump-routes args {
	$self instvar mrtObject_
	if { [info exists mrtObject_] } {
		eval $mrtObject_ dump-routes $args
	}
}

Node instproc add-oif {link if} {
	if { ![$link isLan?] } {
		# append to list of outgoing interfaces, returned
		# by get-all-oifs.  Each object receives packets from
		# that node and transmits on the corresponding link.
		$self instvar outLink_
		lappend outLink_ $if
	}
}

Node instproc get-all-oifs {} {
	$self set outLink_
}

Node instproc get-oif { link } {
	if { $link != "" } {
		return [$self link2oif $link]
	} else {
		return ""
	}
}

Node instproc add-iif {ifid link} {
	if { ![$link isLan?] } {
		# array mapping ifnum -> link
		$self set inLink_($ifid) $link
	}
}

Node instproc if2link ifid {
	$self set inLink_($ifid)
}

Node instproc link2oif link {
	if { [$link isLan?] } {
		# NOTHING YET
		error duh?
	} else {
		$link head
	}
}

Node instproc rpf-nbr src {
	$self instvar ns_ id_
	if [catch "$src id" srcID] {	;# assumes $src is a node handle
		set srcID $src
	}
	$ns_ get-node-by-id [[$ns_ get-routelogic] lookup $id_ $srcID]
}

Node instproc getReps { src group } {
	$self instvar replicator_
	set reps ""
	foreach key [array names replicator_ "$src:$group"] { 
		lappend reps $replicator_($key)
	}
	set reps
}

Node instproc getReps-raw { src group } {
	$self array get replicator_ "$src:$group"
}

Node instproc clearReps { src group } {
	$self instvar multiclassifier_
	foreach {key rep} [$self getReps-raw $src $group] {
		$rep reset
		delete $rep

		foreach {slot val} [$multiclassifier_ adjacents] {
			if { $val == $rep } {
				$multiclassifier_ clear $slot
			}
		}

		$self unset replicator_($key)
	}
}

Node instproc new-group { src group iface code } {
	$self instvar mrtObject_
	$mrtObject_ upcall $code $src $group $iface
}

Node instproc join-group { agent group } {
	$self instvar replicator_ Agents_ mrtObject_
	set group [expr $group]
	$mrtObject_ join-group $group
	lappend Agents_($group) $agent
	foreach key [array names replicator_ "*:$group"] {
		#
		# make sure agent is enabled in each replicator for this group
		#
		$replicator_($key) insert $agent
	}
}

Node instproc leave-group { agent group } {
	$self instvar replicator_ Agents_ mrtObject_
	set group [expr $group] ;# use expr to get rid of possible leading 0x

	foreach key [array names replicator_ "*:$group"] {
		$replicator_($key) disable $agent
	}
	if [info exists Agents_($group)] {
		set k [lsearch -exact $Agents_($group) $agent]
		if { $k >= 0 } {
			set Agents_($group) [lreplace $Agents_($group) $k $k]
		}
		$mrtObject_ leave-group $group
	} else {
		warn "cannot leave a group without joining it"
	}
}

Node instproc join-group-source { agent group source } {
        $self instvar Agents_ mrtObject_ replicator_
        set group [expr $group]
        ## send a message for the mcastproto agent to inform the mcast protocols
        $mrtObject_ join-group $group $source
        lappend Agents_($source:$group) $agent
        if [info exists replicator_($source:$group)] {
                $replicator_($source:$group) insert $agent
        }
}

Node instproc leave-group-source { agent group source } {
        $self instvar replicator_ Agents_ mrtObject_
        set group [expr $group]
        if [info exists replicator_($source:$group)] {
                $replicator_($source:$group) disable $agent
        }
        $mrtObject_ leave-group $group $source
}

Node instproc check-local { group } {
        $self instvar Agents_
        if [info exists Agents_($group)] {
                return [llength $Agents_($group)]
        }
        return 0
}

Node instproc add-mfc { src group iif oiflist } {
	$self instvar multiclassifier_ replicator_ Agents_

	if [info exists replicator_($src:$group)] {
		foreach oif $oiflist {
			$replicator_($src:$group) insert $oif
		}
		return 1
	}

	set r [new Classifier/Replicator/Demuxer]
	$r set srcID_ $src
	$r set grp_ $group
	$r set iif_ $iif
	set replicator_($src:$group) $r

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
Classifier/Multicast instproc new-group {src group iface code} {
	$self instvar node_
	$node_ new-group $src $group $iface $code
}

Classifier/Multicast instproc no-slot slot {
	# NOTHING
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
Classifier/Replicator/Demuxer set ignore_ 0
Classifier/Replicator/Demuxer instproc init args {
	eval $self next $args
	$self set nactive_ 0
}

Classifier/Replicator/Demuxer instproc is-active {} {
	$self instvar nactive_
	expr $nactive_ > 0
}

Classifier/Replicator/Demuxer instproc insert target {
	$self instvar nactive_ active_

	if ![info exists active_($target)] {
		set active_($target) -1
	}
	if {$active_($target) < 0} {
		$self enable $target
	}
}

Classifier/Replicator/Demuxer instproc disable target {
	$self instvar nactive_ active_
	if {[info exists active_($target)] && $active_($target) >= 0} {
		$self clear $active_($target)
		set active_($target) -1
		incr nactive_ -1
	}
}

Classifier/Replicator/Demuxer instproc enable target {
	$self instvar nactive_ active_ ignore_
	if {$active_($target) < 0} {
		set active_($target) [$self installNext $target]
		incr nactive_
		set ignore_ 0
	}
}

Classifier/Replicator/Demuxer instproc exists target {
	$self instvar active_
	info exists active_($target)
}

Classifier/Replicator/Demuxer instproc is-active-target target {
	$self instvar active_
	if { [info exists active_($target)] && $active_($target) >= 0 } {
		return 1
	} else {
		return 0
	}
}

Classifier/Replicator/Demuxer instproc drop { src dst {iface -1} } {
	$self instvar node_
	[$node_ getArbiter] drop $self [expr $src >> 8] $dst $iface
}

Classifier/Replicator/Demuxer instproc change-iface { src dst oldiface newiface} {
	$self instvar node_
        [$node_ set multiclassifier_] change-iface $src $dst \
			[$oldiface if-label?] [$newiface if-label?]
        return 1
}

Classifier/Replicator/Demuxer instproc reset {} {
	$self instvar nactive_ active_
	foreach { target slot } [array get active_] {
		$self clear $slot
	}
	set nactive_ 0
	unset active_
}

#
# XXX
#
#
# XXX These are PIM specific?  Why are they here?
# 
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

#
# These routines are being phased out in favour of newer ones defined earlier.
#
Simulator instproc RPF-link { src from to } {
	set rev [[$self get-routelogic] lookup $to $src]
	if { $rev == $from } {
		$self link $from $to
	} else {
		return ""
	}
}

Node instproc RPF-interface { src from to } {
        $self instvar ns_
	$self get-oif [$ns_ RPF-link $src $from $to]
}

Node instproc get-oifIndex { node_id } {
	error $proc
	$self instvar ns_
	[$ns_ link [$self id] $node_id] if-label?
}

Node instproc label2iface { label } {
	error $proc
	$self instvar outLink_
	foreach oif $outLink_ {
		if { [[$oif link?] if-label?] == $label } {
			return $oif
		}
	}
	return ""
}

Node instproc ifaceGetNode { iface } {
	error $proc
	$self instvar ns_
	set id [$self id]
	foreach nbr [$self neighbors] {
		if { [[$ns_ link [$nbr id] $id] if-label?] == $iface } {
			return $node
		}
	}
	return ""
}

Node instproc getRepByGroup { group } {
	error $proc
	getRep * $group
}

Node instproc getRepBySource { src } {
	error $proc
        $self instvar replicator_
	set replist ""
	foreach n [array names replicator_ "$src:*"] {
		set pair [split $n :]
		lappend replist [lindex $pair 1]:$replicator_($n)
	}
	return $replist
}

Node instproc exists-Rep { src group } {
	error $proc
	$self instvar replicator_
	info exists replicator_($src:$group)
}

#----------------------------------------------------------------------
# Generating multicast distribution trees 
#----------------------------------------------------------------------

# THIS CODE IS CURRENTLY BROKEN AND NEEDS TO BE LOOKED AT AND FIXED.
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
Node instproc XXX-getRepByIIF { src group iif } {
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

			set iif [[$link_($pid:$cid) set ifaceout_] id]
			if {$iif == -1} {
				puts "iif == -1"
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


Agent/Mcast/Control instproc init { protocol } {
	$self next
	$self instvar proto_
	set proto_ $protocol
}
 
Agent/Mcast/Control array set messages {}
Agent/Mcast/Control set mcounter 0

Agent/Mcast/Control instproc send {type from src group args} {
	set m [Agent/Mcast/Control set mcounter]
	Agent/Mcast/Control set messages($m) [eval list $from $src $group $args]
	$self cmd send $type $m
	Agent/Mcast/Control set mcounter [incr m]
}

Agent/Mcast/Control instproc recv {type iface m} {
        eval $self recv2 $type $iface [Agent/Mcast/Control set messages($m)]
	Agent/Mcast/Control unset messages($m)
}

Agent/Mcast/Control instproc recv2 {type iface from src group args} {
	$self instvar proto_
	eval $proto_ recv-$type $from $src $group $iface $args
}

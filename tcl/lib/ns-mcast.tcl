#
# Copyright (c) 1996 Regents of the University of California.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
# 	This product includes software developed by the MASH Research
# 	Group at the University of California Berkeley.
# 4. Neither the name of the University nor of the Research Group may be
#    used to endorse or promote products derived from this software without
#    specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/Attic/ns-mcast.tcl,v 1.4 1997/01/27 01:16:25 mccanne Exp $
#

#
# This module implements simple RPF style flooding/pruning.
# The RPF tree is computed statically from the unicast routes
# (not discovered from unicast routes as in version 1).
#
# TODO:
#	- prune timers
#	- reliable grafts
#	- geneneralize to allow agents to join more than one group
#

Class MultiSim -superclass Simulator
Class MultiNode -superclass Node

MultiNode instproc entry {} {
	$self instvar switch_
	return $switch_
}

MultiNode instproc init sim {
	eval $self next
	$self instvar classifier_ multiclassifier_ switch_ ns_ prune_
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
	set prune_ [new Agent/Message/Prune]
	$self attach $prune_
	$prune_ set node $self
}

#
# send a graft/prune for src/group up the RPF tree
#
MultiNode instproc send-ctrl { which src group } {
	$self instvar prune_ ns_ id_
	set nbr [$ns_ upstream-node $id_ $src]
	$ns_ connect $prune_ [$nbr set prune_]
	if { $which == "prune" } {
		$prune_ set class_ 30
	} else {
		$prune_ set class_ 31
	}
	#puts "$prune: send $which/$id_/$src/$group"
	$prune_ send "$which/$id_/$src/$group"
}

MultiNode instproc recv-prune { from src group } {
	$self instvar outLink_ replicator_ ns_
	#
	# If the replicator exists, mark it down.
	# (How can we receive a prune if we haven't
	# forwarded the packet an installed a replicator?)
	#
	if [info exists replicator_($src:$group)] {
		set r $replicator_($src:$group)
		$r disable $outLink_($from)
	}
}

MultiNode instproc recv-graft { from srcList group } {
	#puts "RECV-GRAFT from $from src $srcList group $group"

	#
	# Filter out our own graft messages.
	# This can happen if we joined a group, left it, and then
	# rejoined it.
	#
	$self instvar id_ outLink_ replicator_
	if { $from == $id_ } {
		return
	}
	foreach src $srcList {
		set r $replicator_($src:$group)
		if { ![$r is-active] && $src != $id_ } {
			#
			# the replicator was shut-down and the
			# source is still upstream so propagate
			# the graft up the tree
			#
			$self send-ctrl graft $src $group
		}
		#
		# restore the flow
		#
		$r enable $outLink_($from)
	}
}

MultiNode instproc send-grafts { srcs group } {
	while { [llength $srcs] > 15 } {
		$self send-ctrl graft [lrange $srcs 0 15] $group
		set srcs [lrange $srcs 16 end]
	}
	$self send-ctrl graft $srcs $group
}

MultiNode instproc join-group { agent group } {
	# use expr to get rid of possible leading 0x
	set group [expr $group]
	$self instvar Agents_ repByGroup_ agentSlot_
	lappend Agents_($group) $agent
	if [info exists repByGroup_($group)] {
		#
		# there are active replicators on this group
		# see if any are idle, and if so send a graft
		# up the RPF tree for the respective S,G
		#
		set srcs ""
		foreach r $repByGroup_($group) {
			if ![$r is-active] {
				lappend srcs [$r set srcID_]
			}
			if ![$r exists $agent] {
				$r insert $agent
			}
		}
		if { $srcs != "" } {
			$self send-grafts $srcs $group
		}
	}
	#
	# make sure agent is enabled in each replicator
	# for this group
	#
	if [info exists repByGroup_($group)] {
		foreach r $repByGroup_($group) {
			$r enable $agent
		}
	}
}

MultiNode instproc leave-group { agent group } {
	# use expr to get rid of possible leading 0x
	set group [expr $group]
	$self instvar repByGroup_ Agents_
#XXX info exists
	foreach r $repByGroup_($group) {
		$r disable $agent
	}
	set k [lsearch -exact $Agents_($group) $agent]
	if { $k >= 0 } {
		set Agents_($group) [lreplace $Agents_($group) $k $k]
	}
}

Class Agent/Message/Prune -superclass Agent/Message

Agent/Message/Prune instproc handle msg {
	set L [split $msg /]
	set type [lindex $L 0]
	set from [lindex $L 1]
	set src [lindex $L 2]
	set group [lindex $L 3]
	$self instvar node_
	if { $type == "prune" } {
		$node_ recv-prune $from $src $group
	} else {
		$node_ recv-graft $from $src $group
	}
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

MultiNode instproc new-group { srcID group } {
	$self instvar neighbor_ id_ multiclassifier_ ns_ \
		replicator_ agentSlot_ Agents_ repByGroup_ outLink_

	#XXX node addr is in upper 24 bits
#	set srcID [expr $src >> 8]
	set r [new Classifier/Replicator/Demuxer]
	$r set srcID_ $srcID
	set replicator_($srcID:$group) $r

	lappend repByGroup_($group) $r
	$r set node_ $self
	foreach node $neighbor_ {
		set nbr [$node id]
		set link [$ns_ RPF-link $srcID $id_ $nbr]
		if { $link != "" } {
			# remember where each nbr is in the replicator
			set outLink_($nbr) [$link head]
			$r insert $outLink_($nbr)
			#puts "MRT($k): $id -> [$node id] for $srcID/$group"
		}
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
	$multiclassifier_ add-rep $r $srcID $group
}

MultiSim instproc node {} {
	$self instvar Node_
	set node [new MultiNode $self]
	set Node_([$node id]) $node
	return $node
}

Class Classifier/Multicast/Replicator -superclass Classifier/Multicast

#
# This method called when a new multicast group/source pair
# is seen by the underlying classifier/mcast object.
# We install a hash for the pair mapping it to a slot
# number in the classifier table and point the slot
# at a replicator object that sends each packet along
# the RPF tree.
#
Classifier/Multicast instproc new-group { src group } {
	$self instvar node_
	$node_ new-group $src $group
}

Classifier/Multicast/Replicator instproc init args {
	$self next
	$self instvar nrep_
	set nrep_ 0
}

Classifier/Multicast/Replicator instproc add-rep { rep src group } {
	$self instvar nrep_
	$self set-hash $src $group $nrep_
	$self install $nrep_ $rep
	incr nrep_
}

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
	if { $src == [$node_ id] } {
		#
		# if we are trying to prune ourself (i.e., no
		# receivers anywhere in the network), set the
		# ignore bit in the object (we turn it back on
		# when we get a graft).  This prevents this
		# drop methood from being called on every packet.
		#
		$self instvar ignore_
		set ignore_ 1
	} else {
		$node_ send-ctrl prune $src $dst
	}
}


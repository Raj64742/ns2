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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/Attic/ns-mcast.tcl,v 1.1 1996/12/19 03:22:47 mccanne Exp $
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
	$self instvar switch
	return $switch
}

MultiNode instproc init sim {
	eval $self next
	$self instvar classifier multiclassifier switch ns prune
	set ns $sim

	set switch [new classifier/addr]
	#
	# set up switch to route unicast packet to slot 0 and
	# multicast packets to slot 1
	#
	$switch set mask 1
	$switch set shift 15

	#
	# create a classifier for multicast routing
	#
	set multiclassifier [new classifier/mcast/rep]
	$multiclassifier set node $self

	$switch install 0 $classifier
	$switch install 1 $multiclassifier

	#
	# Create a prune agent.  Each multicast routing node
	# has a private prune agent that sends and receives
	# prune/graft messages and dispatches them to the
	# appropriate replicator object.
	#
	set prune [new agent/message/prune]
	$self attach $prune
	$prune set node $self
}

#
# send a graft/prune for src/group up the RPF tree
#
MultiNode instproc send-ctrl { which src group } {
	$self instvar prune ns id
	set nbr [$ns upstream-node $id $src]
	$ns connect $prune [$nbr set prune]
	if { $which == "prune" } {
		$prune set cls 30
	} else {
		$prune set cls 31
	}
	$prune send "$which/$id/$src/$group"
}

MultiNode instproc recv-prune { from src group } {
	$self instvar outLink replicator ns
	#
	# If the replicator exists, mark it down.
	# (How can we receive a prune if we haven't
	# forwarded the packet an installed a replicator?)
	#
	if [info exists replicator($src:$group)] {
		set r $replicator($src:$group)
		$r disable $outLink($from)
	}
}

MultiNode instproc recv-graft { from srcList group } {
	#puts "RECV-GRAFT from $from src $src group $group"
	$self instvar outLink replicator id
	foreach src $srcList {
		set r $replicator($src:$group)
		if { ![$r is-active] && $src != $id } {
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
		$r enable $outLink($from)
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
	$self instvar Agents repByGroup agentSlot
	lappend Agents($group) $agent
	if [info exists repByGroup($group)] {
		#
		# there are active replicators on this group
		# see if any are idle, and if so send a graft
		# up the RPF tree for the respective S,G
		#
		set srcs ""
		foreach r $repByGroup($group) {
			if ![$r is-active] {
				lappend srcs [$r set srcID]
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
	if [info exists repByGroup($group)] {
		foreach r $repByGroup($group) {
			$r enable $agent
		}
	}
}

MultiNode instproc leave-group { agent group } {
	# use expr to get rid of possible leading 0x
	set group [expr $group]
	$self instvar repByGroup Agents
#XXX info exists
	foreach r $repByGroup($group) {
		$r disable $agent
	}
	set k [lsearch -exact $Agents($group) $agent]
	if { $k >= 0 } {
		set Agents($group) [lreplace $Agents($group) $k $k]
	}
}

Class agent/message/prune -superclass agent/message

agent/message/prune instproc handle msg {
	set L [split $msg /]
	set type [lindex $L 0]
	set from [lindex $L 1]
	set src [lindex $L 2]
	set group [lindex $L 3]
	$self instvar node
	if { $type == "prune" } {
		$node recv-prune $from $src $group
	} else {
		$node recv-graft $from $src $group
	}
}

MultiSim instproc upstream-node { id src } {
	$self instvar routingTable Node
	set nbr [$routingTable lookup $id $src]
	return $Node($nbr)
}

MultiSim instproc RPF-link { src from to } {
	$self instvar routingTable link
	#
	# If this link is on the RPF tree, return the link object.
	#
	set reverse [$routingTable lookup $to $src]
	if { $reverse == $from } {
		return $link($from:$to)
	}
	return ""
}

MultiNode instproc new-group { srcID group } {
	$self instvar neighbor id multiclassifier ns \
		replicator agentSlot Agents repByGroup outLink

	#XXX node addr is in upper 24 bits
#	set srcID [expr $src >> 8]
	set r [new classifier/replicator/mcast]
	$r set srcID $srcID
	set replicator($srcID:$group) $r
	lappend repByGroup($group) $r
	$r set node $self
	foreach node $neighbor {
		set nbr [$node id]
		set link [$ns RPF-link $srcID $id $nbr]
		if { $link != "" } {
			# remember where each nbr is in the replicator
			set outLink($nbr) [$link head]
			$r insert $outLink($nbr)
			#puts "MRT($k): $id -> [$node id] for $srcID/$group"
		}
	}
	#
	# install each agent that has previously joined this group
	#
	if [info exists Agents($group)] {
		foreach a $Agents($group) {
			$r insert $a
		}
	}

	#
	# Install the replicator.  We do this only once and leave
	# it forever.  Since prunes are data driven, we want to
	# leave the replicator in place even when it's empty since
	# the replicator::drop callback triggers the prune.
	#
	$multiclassifier add-rep $r $srcID $group
}

MultiSim instproc node {} {
	$self instvar Node
	set node [new MultiNode $self]
	set Node([$node id]) $node
	return $node
}

Class classifier/mcast/rep -superclass classifier/mcast

#
# This method called when a new multicast group/source pair
# is seen by the underlying classifier/mcast object.
# We install a hash for the pair mapping it to a slot
# number in the classifier table and point the slot
# at a replicator object that sends each packet along
# the RPF tree.
#
classifier/mcast instproc new-group { src group } {
	$self instvar node
	$node new-group $src $group
}

classifier/mcast/rep instproc init args {
	$self next
	$self instvar nrep 
	set nrep 0
}

classifier/mcast/rep instproc add-rep { rep src group } {
	$self instvar nrep
	$self set-hash $src $group $nrep
	$self install $nrep $rep
	incr nrep
}

Class classifier/replicator/mcast -superclass classifier/replicator

classifier/replicator set ignore 0

classifier/replicator/mcast instproc init args {
	eval $self next $args
	$self instvar nslot nactive
	set nslot 0
	set nactive 0
}

classifier/replicator/mcast instproc is-active {} {
	$self instvar nactive
	return [expr $nactive > 0]
}

classifier/replicator/mcast instproc insert target {
	$self instvar nslot nactive active index
	set n $nslot
	incr nslot
	incr nactive
	$self install $n $target
	set active($target) 1
	set index($target) $n
}

classifier/replicator/mcast instproc disable target {
	$self instvar nactive active index
	if $active($target) {
		$self clear $index($target)
		incr nactive -1
		set active($target) 0
	}
}

classifier/replicator/mcast instproc enable target {
	$self instvar nactive active ignore index
	if !$active($target) {
		$self install $index($target) $target
		incr nactive
		set active($target) 1
		set ignore 0
	}
}

classifier/replicator/mcast instproc exists target {
	$self instvar active
	return [info exists arctive($target)]
}

classifier/replicator/mcast instproc drop { src dst } {
	#
	# No downstream listeners
	# Send a prune back toward the source
	#
	set src [expr $src >> 8]
	$self instvar node
	if { $src == [$node id] } {
		#
		# if we are trying to prune ourself (i.e., no
		# receivers anywhere in the network), set the
		# ignore bit in the object (we turn it back on
		# when we get a graft).  This prevents this
		# drop methood from being called on every packet.
		#
		$self instvar ignore
		set ignore 1
	} else {
		$node send-ctrl prune $src $dst
	}
}


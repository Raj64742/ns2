# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
#  Time-stamp: <2000-09-13 18:22:04 haoboy>
# 
#  Copyright (c) 2000 by the University of Southern California
#  All rights reserved.
# 
#  Permission to use, copy, modify, and distribute this software and its
#  documentation in source and binary forms for non-commercial purposes
#  and without fee is hereby granted, provided that the above copyright
#  notice appear in all copies and that both the copyright notice and
#  this permission notice appear in supporting documentation. and that
#  any documentation, advertising materials, and other materials related
#  to such distribution and use acknowledge that the software was
#  developed by the University of Southern California, Information
#  Sciences Institute.  The name of the University may not be used to
#  endorse or promote products derived from this software without
#  specific prior written permission.
# 
#  THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
#  the suitability of this software for any purpose.  THIS SOFTWARE IS
#  PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
#  INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 
#  Other copyrights might apply to parts of this software and are so
#  noted when applicable.
# 
#  $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-rtmodule.tcl,v 1.6 2001/03/06 20:49:05 haldar Exp $
#
# OTcl interface definition for the base routing module. They provide 
# linkage to Node, hence all derived classes should inherit these interfaces
# and fill in their specific handling code.

RtModule instproc register { node } {
	# Attach to node and register routing notifications
	$self attach-node $node
	$node route-notify $self
	$node port-notify $self
}

#RtModule instproc init { node } {
#	$self instvar classifier_
#	$self next 
#	# Attach to node and register routing notifications
#	$self attach-node $node
#	$node route-notify $self
#	$node port-notify $self
#}

# Only called when the default classifier of this module is REPLACED.
RtModule instproc unregister {} {
	$self instvar classifier_
	delete $classifier_
	[$self node] unreg-route-notify $self
	[$self node] unreg-port-notify $self
}

RtModule instproc add-route { dst target } {
	[$self set classifier_] install $dst $target
}

RtModule instproc delete-route { dst nullagent} {
	[$self set classifier_] install $dst $nullagent
}

RtModule instproc attach { agent port } {
	# Send target
	$agent target [[$self node] entry]
	# Recv target
	[[$self node] demux] install $port $agent
}

RtModule instproc detach { agent nullagent } {
	# Empty by default
}

RtModule instproc reset {} {
	# Empty by default
}

#
# Base routing module
#

# Use standard naming although this is a pure OTcl class 
#
# In fact, all functionalities of Base are already implemented in RtModule.
# We add this class only to provide a uniform interface to the RtModule 
# creation process, where a single line [new RtModule/$name] is used.
#
# Defined in ~ns/rtmodule.{h,cc}

RtModule/Base instproc register { node } {
	$self next $node

	$self instvar classifier_
	set classifier_ [new Classifier/Hash/Dest 32]
	$classifier_ set mask_ [AddrParams NodeMask 1]
	$classifier_ set shift_ [AddrParams NodeShift 1]
	# XXX Base should ALWAYS be the first module to be installed.

	$node install-entry $self $classifier_

	#XXX this should go away when classifier_ becomes a 
	#XXX a bound object across C++/OTcl line
	#$self attach-classifier $classifier_
}


#
# Illustrates usage of insert-entry{}. However:
# 
# XXX Ugly hack: We should keep all variables local to this particular
# module. However, given all existing multicast code assumes that 
# switch_ and multiclassifier_ are inside Node, we have to make this 
# compromise if we do not want to modify all existing code. :(
#
RtModule/Mcast instproc register { node } {
	$self next $node
	$self instvar classifier_
	
	# Keep old classifier so we can use RtModule::add-route{}.
	$self set classifier_ [$node entry]
	
	if {[$classifier_ info class] != "Classifier/Virtual"} {
		# donot want to add-route if virtual classifier
		$self attach-classifier $classifier_
	}
	
	$node set switch_ [new Classifier/Addr]

	# Set up switch to route unicast packet to slot 0 and
	# multicast packets to slot 1
	[$node set switch_] set mask_ [AddrParams McastMask]
	[$node set switch_] set shift_ [AddrParams McastShift]

	# Create a classifier for multicast routing
	$node set multiclassifier_ [new Classifier/Multicast/Replicator]
	[$node set multiclassifier_] set node_ $node
	
	$node set mrtObject_ [new mrtObject $node]

	# Install existing classifier at slot 0, new classifier at slot 1
	$node insert-entry $self [$node set switch_] 0
	[$node set switch_] install 1 [$node set multiclassifier_]
}

#
# Hierarchical routing module. 
#
RtModule/Hier instproc register { node } {
	$self next $node
	$self instvar classifier_
	set classifier_ [new Classifier/Hier]
	$node install-entry $self $classifier_
	#$self attach-classifier $classifier_
}

RtModule/Hier instproc delete-route args {
	eval [$self set classifier_] clear $args
}

Classifier/Hier instproc init {} {
	$self next
	for {set n 1} {$n <= [AddrParams hlevel]} {incr n} {
		set classifier [new Classifier/Addr]
		$classifier set mask_ [AddrParams NodeMask $n]
		$classifier set shift_ [AddrParams NodeShift $n]
		$self cmd add-classifier $n $classifier
	}
}

Classifier/Hier instproc destroy {} {
	for {set n 1} {$n <= [AddrParams hlevel]} {incr n} {
		delete [$self cmd classifier $n]
	}
	$self next
}

Classifier/Hier instproc clear args {
	set l [llength $args]
	[$self cmd classifier $l] clear [lindex $args [expr $l-1]] 
}

Classifier/Hier instproc install { dst target } {
	set al [AddrParams split-addrstr $dst]
	set l [llength $al]
	for {set i 1} {$i < $l} {incr i} {
		set d [lindex $al [expr $i-1]]
		[$self cmd classifier $i] install $d \
				[$self cmd classifier [expr $i+1]]
	}
	[$self cmd classifier $l] install [lindex $al [expr $l-1]] $target
}


#
# Manual Routing Nodes:
# like normal nodes, but with a hash classifier.
#
RtModule/Manual instproc register { node } {
	$self next $node
	$self instvar classifier_	
	# Note the very small hash size---
	# you're supposed to resize it if you want more.
	set classifier_ [new Classifier/Hash/Dest 2]
	$classifier_ set mask_ [AddrParams NodeMask 1]
	$classifier_ set shift_ [AddrParams NodeShift 1]
	$node install-entry $self $classifier_
	#$self attach-classifier $classifier_
}

RtModule/Manual instproc add-route {dst_address target} {
	$self instvar classifier_ 
	set slot [$classifier_ installNext $target]
	if {$dst_address == "default"} {
		$classifier_ set default_ $slot
	} else {
		# don't encode the address here, set-hash bypasses that for us
		set encoded_dst_address [expr $dst_address << [AddrParams NodeShift 1]]
		$classifier_ set-hash auto 0 $encoded_dst_address 0 $slot
	}
}

RtModule/Manual instproc add-route-to-adj-node { args } {
	$self instvar classifier_ 

	set dst ""
	if {[lindex $args 0] == "-default"} {
		set dst default
		set args [lrange $args 1 end]
	}
	if {[llength $args] != 1} {
		error "ManualRtNode::add-route-to-adj-node [-default] node"
	}
	set target_node $args
	if {$dst == ""} {
		set dst [$target_node set address_]
	}
	set ns [Simulator instance]
	set link [$ns link [$self node] $target_node]
	set target [$link head]
	return [$self add-route $dst $target]
}


#
# Virtual Classifier Nodes:
# like normal nodes, but with a virtual unicast classifier.
#
RtModule/VC instproc register { node } {
	# We do not do route-notify. Only port-notify will suffice.
	$self instvar classifier_

	$self attach-node $node
	$node port-notify $self

	set classifier_ [new Classifier/Virtual]
	$classifier_ set node_ $node
	$classifier_ set mask_ [AddrParams NodeMask 1]
	$classifier_ set shift_ [AddrParams NodeShift 1]
	$classifier_ nodeaddr [$node node-addr]
	$node install-entry $self $classifier_ 
	#$self attach-classifier $classifier_
	#$self attach-self $node
}

RtModule/VC instproc add-route { dst target } {
}

Classifier/Virtual instproc find dst {
	$self instvar node_
	if {[$node_ id] == $dst} {
		return [$node_ set dmux_]
	} else {
		return [[[Simulator instance] link $node_ \
				[[Simulator instance] set Node_($dst)]] head]
	}
}

Classifier/Virtual instproc install {dst target} {
}

Class RtModule/Nix -superclass RtModule

#Nix-vector routing
RtModule/Nix instproc register { node } {
	$self next $node
	# Create classifier
	$self instvar classifier_
	set classifier_ [new Classifier/Nix]
	$classifier_ set-node-id [$node set id_]
	#puts "RtModule/Nix set node id to [$node set id_]"
	$node install-entry $self $classifier_
        #$self attach-classifier $classifier_
}

RtModule/Nix instproc route-notify { module } { }





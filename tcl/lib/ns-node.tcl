# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
# Time-stamp: <2000-08-30 13:59:20 haoboy>
#
# Copyright (c) 1996-1998 Regents of the University of California.
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-node.tcl,v 1.76 2000/08/30 23:27:51 haoboy Exp $
#

Node set nn_ 0
Node proc getid {} {
	set id [Node set nn_]
	Node set nn_ [expr $id + 1]
	return $id
}

Node instproc init args {
	eval $self next $args

        $self instvar id_ agents_ dmux_ neighbor_ rtsize_ address_ \
			nodetype_ multiPath_ ns_

	set ns_ [Simulator instance]
	set id_ [Node getid]
	$self nodeid $id_	;# Propagate id_ into c++ space
	if {[llength $args] != 0} {
		set address_ [lindex $args 0]
	} else {
		set address_ $id_
	}

        set nodetype_ [$ns_ get-nodetype]
	set neighbor_ ""
	set agents_ ""
	set dmux_ ""
        set rtsize_ 0
	$self mk-default-classifier$nodetype_
	$self cmd addr $address_; # new by tomh
	set multiPath_ [$class set multiPath_]
	if [$ns_ multicast?] {
		$self enable-mcast $ns_
	}
}

Node instproc node-type {} {
	return [$self set nodetype_]
}

# XXX We shall replace these mk-default-classifer* stuff with a more 
# modular approach.
Node instproc mk-default-classifierMIPMH {} {
	$self mk-default-classifier
}

Node instproc mk-default-classifierMIPBS {} {
	$self mk-default-classifier
}

Node instproc mk-default-classifierBase {} {
	$self mk-default-classifier
}

Node instproc mk-default-classifierMobile {} {
	$self mk-default-classifier
}

Node instproc mk-default-classifierHier {} {
	$self mk-default-classifier
}

Node instproc mk-default-classifier {} {
	$self instvar classifier_ 
	# Set up classifer as a router. Hierarchical routing, or multi-path
	# routing, cannot be specified as a plugin routing module, because 
	# its installation is too closely tied to the node initialization 
	# process.  - Aug 30, 2000
	if [Simulator set EnableHierRt_] {
		$self instvar classifiers_
		set levels [AddrParams set hlevel_]
		for {set n 1} {$n <= $levels} {incr n} {
			set classifiers_($n) [new Classifier/Addr]
			$classifiers_($n) set mask_ \
					[AddrParams set NodeMask_($n)]
			$classifiers_($n) set shift_ \
					[AddrParams set NodeShift_($n)]
		}
		$self set classifier_ $classifiers_(1)
	} else {
		set classifier_ [new Classifier/Hash/Dest 32]
		$classifier_ set mask_ [AddrParams set NodeMask_(1)]
		$classifier_ set shift_ [AddrParams set NodeShift_(1)]
	}
}

# We no longer take $sim as argument, since ns_ is set in Node::init{}
Node instproc enable-mcast args {
	$self instvar classifier_ multiclassifier_ ns_ switch_ mcastproto_
	
	$self set switch_ [new Classifier/Addr]
	#
	# set up switch to route unicast packet to slot 0 and
	# multicast packets to slot 1
	#
	[$self set switch_] set mask_ [AddrParams set McastMask_]
	[$self set switch_] set shift_ [AddrParams set McastShift_]
	#
	# create a classifier for multicast routing
	#
	$self set multiclassifier_ [new Classifier/Multicast/Replicator]
	[$self set multiclassifier_] set node_ $self
	
	$self set mrtObject_ [new mrtObject $self]

	$switch_ install 0 $classifier_
	$switch_ install 1 $multiclassifier_
}

#
# Increase the routing table size counter - keeps track of rtg table 
# size for each node. For bookkeeping only.
#
Node instproc incr-rtgtable-size {} {
	$self instvar rtsize_
	incr rtsize_
}

Node instproc decr-rtgtable-size {} {
	$self instvar rtsize_
	incr rtsize_ -1
}

Node instproc entry {} {
	$self instvar ns_
	if [info exists router_supp_] {
		return $router_supp_
	}
	if [$ns_ multicast?] {
		$self instvar switch_
		return $switch_
	}
	$self instvar classifier_
	return $classifier_
}

Node instproc id {} {
	$self instvar id_
	return $id_
}

Node instproc node-addr {} {
	$self instvar address_
	return $address_
}

# XXX np_ (number of ports allocated) is now only used in SessionNode.
Node instproc alloc-port { nullagent } {
	return [[$self set dmux_] alloc-port $nullagent]
}

#
# Attach an agent to a node.  Pick a port and
# bind the agent to the port number.
#
Node instproc attach { agent { port "" } } {
	$self instvar agents_ address_ dmux_ 
	#
	# assign port number (i.e., this agent receives
	# traffic addressed to this host and port)
	#
	lappend agents_ $agent
	#
	# Attach agents to this node (i.e., the classifier inside).
	# We call the entry method on ourself to find the front door
	# (e.g., for multicast the first object is not the unicast
	# classifier)
	# Also, stash the node in the agent and set the
	# local addr of this agent.
	#
	$agent set node_ $self
	if [Simulator set EnableHierRt_] {
		$agent set agent_addr_ [AddrParams set-hieraddr $address_]
	} else {
		$agent set agent_addr_ [expr ($address_ & \
				[AddrParams set NodeMask_(1)]) \
				<< [AddrParams set NodeShift_(1) ]]
	}
	#
	# If a port demuxer doesn't exist, create it.
	#
	if { $dmux_ == "" } {
		set dmux_ [new Classifier/Port]
		$dmux_ set mask_ [AddrParams set ALL_BITS_SET]
		$dmux_ set shift_ 0
		#
		# point the node's routing entry to itself
		# at the port demuxer (if there is one)
		#
		if {[Simulator set EnableHierRt_]} {
			$self add-hroute $address_ $dmux_
		} else {
			$self add-route $address_ $dmux_
		}
	}
	if {$port == ""} {
		set port [$self alloc-port [[Simulator instance] \
				set nullAgent_]]
	}
	$agent set agent_port_ $port
	$self add-target $agent $port
}

#
# add target to agent and add entry for port-id in port-dmux
#
Node instproc add-target {agent port} {
	$self instvar dmux_
	#
	# Send Target
	#
	$agent target [$self entry]
	#
	# Recv Target
	#
	$dmux_ install $port $agent
}
	
#
# Detach an agent from a node.
#
Node instproc detach { agent nullagent } {
	$self instvar agents_ dmux_
	#
	# remove agent from list
	#
	set k [lsearch -exact $agents_ $agent]
	if { $k >= 0 } {
		set agents_ [lreplace $agents_ $k $k]
	}
	#
	# sanity -- clear out any potential linkage
	#
	$agent set node_ ""
	$agent set agent_addr_ 0
	$agent target $nullagent
	
	set port [$agent set agent_port_]
	
	#Install nullagent to sink transit packets   
	$dmux_ install $port $nullagent
}

Node instproc agent port {
	$self instvar agents_
	foreach a $agents_ {
		if { [$a set agent_port_] == $port } {
			return $a
		}
	}
	return ""
}

#
# reset all agents attached to this node
#
Node instproc reset {} {
	$self instvar agents_
	foreach a $agents_ {
		$a reset
	}
}

#
# Some helpers
#
Node instproc neighbors {} {
	$self instvar neighbor_
	return [lsort $neighbor_]
}

Node instproc add-neighbor p {
	$self instvar neighbor_
	lappend neighbor_ $p
}

Node instproc is-neighbor { node } {
	$self instvar neighbor_
	return [expr [lsearch $neighbor_ $node] != -1]
}

#
# Helpers for interface stuff. XXX not used anymore??
#
Node instproc addInterface { iface } {
	$self instvar ifaces_
	lappend ifaces_ $iface
}

#
# Address classifier manipulation
#
Node instproc add-route { dst target } {
	$self instvar classifier_
	$classifier_ install $dst $target
	$self incr-rtgtable-size
}

Node instproc delete-route { dst nullagent } {
	$self instvar classifier_
	$classifier_ install $dst $nullagent
	$self decr-rtgtable-size
}

#
# Node support for detailed dynamic routing
#
Node instproc init-routing rtObject {
	$self instvar multiPath_ routes_ rtObject_
	set multiPath_ [$class set multiPath_]
	set nn [$class set nn_]
	for {set i 0} {$i < $nn} {incr i} {
		set routes_($i) 0
	}
	if ![info exists rtObject_] {
		$self set rtObject_ $rtObject
	}
	$self set rtObject_
}

Node instproc rtObject? {} {
    $self instvar rtObject_
	if ![info exists rtObject_] {
		return ""
	} else {
		return $rtObject_
    }
}

# Splitting up address string
Node instproc split-addrstr addrstr {
	set L [split $addrstr .]
	return $L
}

# method to remove an entry from the hier classifiers
Node/MobileNode instproc delete-hroute args {
	$self instvar classifiers_
	set l [llength $args]
	$classifiers_($l) clear [lindex $args [expr $l-1]] 
}

Node instproc add-hroute { dst target } {
	$self instvar classifiers_ rtsize_
	set al [$self split-addrstr $dst]
	set l [llength $al]
	for {set i 1} {$i < $l} {incr i} {
		set d [lindex $al [expr $i-1]]
		$classifiers_($i) install $d $classifiers_([expr $i + 1]) 
	}
	$classifiers_($l) install [lindex $al [expr $l-1]] $target
	# increase the routing table size counter - keeps track of rtg 
	# table size for each node
	$self incr-rtgtable-size
}

#
# Node support for equal cost multi path routing
#
Node instproc add-routes {id ifs} {
	$self instvar classifier_ multiPath_ routes_ mpathClsfr_
	if !$multiPath_ {
		if {[llength $ifs] > 1} {
			warn "$class::$proc cannot install multiple routes"
			set ifs [lindex $ifs 0]
		}
		$self add-route $id [$ifs head]
		set routes_($id) 1
		return
	}
	if {$routes_($id) <= 0 && [llength $ifs] == 1 && 	\
			![info exists mpathClsfr_($id)]} {
		$self add-route $id [$ifs head]
		set routes_($id) 1
	} else {
		if ![info exists mpathClsfr_($id)] {
			#
			# 1. get new MultiPathClassifier,
			# 2. migrate existing routes to that mclassifier
			# 3. install the mclassifier in the node classifier_
			#
			set mpathClsfr_($id) [new Classifier/MultiPath]
			if {$routes_($id) > 0} {
				assert "$routes_($id) == 1"
				$mpathClsfr_($id) installNext \
						[$classifier_ in-slot? $id]
			}
			$classifier_ install $id $mpathClsfr_($id)
		}
		foreach L $ifs {
			$mpathClsfr_($id) installNext [$L head]
			incr routes_($id)
		}
	}
}

Node instproc delete-routes {id ifs nullagent} {
	$self instvar mpathClsfr_ routes_
	if [info exists mpathClsfr_($id)] {
		foreach L $ifs {
			set nonLink([$L head]) 1
		}
		foreach {slot link} [$mpathClsfr_($id) adjacents] {
			if [info exists nonLink($link)] {
				$mpathClsfr_($id) clear $slot
				incr routes_($id) -1
				# XXX Point to null agent if entries goes to 0?
				# Is this the right procedure??
				if { $routes_($id) == 0 } {
					delete $mpathClsfr_($id)
					unset mpathClsfr_($id)
					$self delete-route $id $nullagent
				}
			}
		}
	} else {
		$self delete-route $id $nullagent
		incr routes_($id) -1
		# Notice that after this operation routes_($id) may not 
		# necessarily be 0, because 
	}
}

Node instproc intf-changed {} {
	$self instvar rtObject_
	if [info exists rtObject_] {	;# i.e. detailed dynamic routing
		$rtObject_ intf-changed
	}
}

#
# Manual Routing Nodes:
# like normal nodes, but with a hash classifier.
#
Class ManualRtNode -superclass Node

ManualRtNode instproc mk-default-classifier {} {
	$self instvar address_ classifier_ id_ dmux_
	# Note the very small hash size---
	# you're supposed to resize it if you want more.
	set classifier_ [new Classifier/Hash/Dest 2]
	$classifier_ set mask_ [AddrParams set NodeMask_(1)]
	$classifier_ set shift_ [AddrParams set NodeShift_(1)]
	set address_ $id_
	#
	# When an agent is created,
	# $self add-route $address_ $dmux_ is called
	# which will do this.
	#
}

ManualRtNode instproc add-route {dst_address target} {
	$self instvar classifier_ 
	set slot [$classifier_ installNext $target]
	if {$dst_address == "default"} {
		$classifier_ set default_ $slot
	} else {
		# don't encode the address here, set-hash bypasses that for us
		set encoded_dst_address [expr $dst_address << [AddrParams set NodeShift_(1)]]
		$classifier_ set-hash auto 0 $encoded_dst_address 0 $slot
	}
}

ManualRtNode instproc add-route-to-adj-node { args } {
	$self instvar classifier_ address_

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
	set link [$ns link $self $target_node]
	set target [$link head]
	return [$self add-route $dst $target]
}

#
# Virtual Classifier Nodes:
# like normal nodes, but with a virtual unicast classifier.
#
Class VirtualClassifierNode -superclass Node

VirtualClassifierNode instproc mk-default-classifier {} {
	$self instvar address_ classifier_ id_

	set classifier_ [new Classifier/Virtual]
	$classifier_ set node_ $self
	$classifier_ set mask_ [AddrParams set NodeMask_(1)]
	$classifier_ set shift_ [AddrParams set NodeShift_(1)]
	set address_ $id_
	$classifier_ nodeaddr $address_
}

VirtualClassifierNode instproc add-route { dst target } {
}

Classifier/Virtual instproc find dst {
	$self instvar node_ ns_ 

	if ![info exist ns_] {
		set ns_ [Simulator instance]
	}
	if {[$node_ id] == $dst} {
		return [$node_ set dmux_]
	} else {
		return [[$ns_ link $node_ [$ns_ set Node_($dst)]] head]
	}
}

Classifier/Virtual instproc install {dst target} {
}

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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-node.tcl,v 1.38.2.4 1998/10/02 18:19:35 kannan Exp $
#

Class Node
Node set nn_ 0
Node proc getid {} {
	set id [Node set nn_]
	Node set nn_ [expr $id + 1]
	return $id
}

Node instproc init args {
	set args [lreplace $args 0 1]
	eval $self next $args
	$self instvar np_ id_ agents_ dmux_ neighbor_
	set neighbor_ ""
	set agents_ ""
	set dmux_ ""
	set np_ 0
	set id_ [Node getid]
	$self mk-default-classifier
}

Node instproc mk-default-classifier {} {
	$self instvar address_ classifier_ id_
	set classifier_ [new Classifier/Addr]
	# set up classifer as a router (default value 8 bit of addr and 8 bit port)
	$classifier_ set mask_ [AddrParams set NodeMask_(1)]
	$classifier_ set shift_ [AddrParams set NodeShift_(1)]
	set address_ $id_
}

Node instproc enable-mcast sim {
	$self instvar classifier_ multiclassifier_ ns_ switch_ mrtObject_

	set switch_ [new Classifier/Addr]
	$switch_ set mask_  [AddrParams set McastMask_]
	$switch_ set shift_ [AddrParams set McastShift_]

	#
	# create a classifier for multicast routing
	#
	set multiclassifier_ [new Classifier/Multicast/Replicator]
	$multiclassifier_ set node_ $self	;# XXX
	
	#
	# Create a prune agent.  Each multicast routing node
	# has a private prune agent that sends and receives
	# prune/graft messages and dispatches them to the
	# appropriate replicator object.
	#
	set mrtObject_ [new mrtObject $self ""] ;# no/default protos
	# XXX $mrtObject_ set Node $self

	#
	# Finally, set up switch to route unicast packets
	# to slot 0 and multicast packets to slot 1
	#
	$switch_ install 0 $classifier_
	$switch_ install 1 $multiclassifier_
}

Node instproc add-out-link l {
	$self instvar outLink_
	lappend outLink_ $l
}

Node instproc add-neighbor p {
	$self instvar neighbor_
	lappend neighbor_ $p
}

Node instproc entry {} {
	if [info exists router_supp_] {
		return $router_supp_
	}
        $self instvar ns_
        if { [$ns_ multicast?] } {
                $self instvar switch_
                return $switch_
        }
        $self instvar classifier_
        return $classifier_
}

Node instproc add-route { dst target } {
	$self instvar classifier_
	$classifier_ install $dst $target
}

Node instproc id {} {
	$self instvar id_
	return $id_
}

Node instproc node-addr {} {
	$self instvar address_
	return $address_
}

Node instproc alloc-port { nullagent } {
	$self instvar dmux_ np_
	set p [$dmux_ alloc-port $nullagent]
	if { $np_ < $p } {
		set np_ $p
	}
	if {$np_ > [$dmux_ set mask_] } {
		puts stderr "No of ports($np_) attached to $self node is greater than allowed"
	}
	
	return $p
}

#
# Attach an agent to a node.  Pick a port and
# bind the agent to the port number.
#
Node instproc attach { agent } {
	$self instvar agents_ address_ dmux_
	#
	# assign port number (i.e., this agent receives
	# traffic addressed to this host and port)
	#
	lappend agents_ $agent
	
	#
	# Check if number of agents exceeds length of port-address-field size
	#
	set mask [AddrParams set PortMask_]
	set shift [AddrParams set PortShift_]
	
	if {[llength $agents_] >= $mask} {
		error "\# of agents attached to node $self exceeds port-field length of $mask bits\n"
	}

	#
	# Attach agents to this node (i.e., the classifier inside).
	# We call the entry method on ourself to find the front door
	# (e.g., for multicast the first object is not the unicast
	#  classifier)
	# Also, stash the node in the agent and set the
	# local addr of this agent.
	#
	$agent target [$self entry]
	$agent set node_ $self
	
	#
	# If a port demuxer doesn't exist, create it.
	#
	if { $dmux_ == "" } {
		set dmux_ [new Classifier/Addr]
		$dmux_ set mask_ $mask
		$dmux_ set shift_ $shift
		
		# $dmux_ set shift_ 0
		#
		# point the node's routing entry to itself
		# at the port demuxer (if there is one)
		#
		if [Simulator set EnableHierRt_] {
			$self add-hroute $address_ $dmux_
		} else {
			$self add-route $address_ $dmux_
		}
	}
	set ns_ [Simulator instance]
	$ns_ instvar nullAgent_
	set port [$self alloc-port $nullAgent_]
	$agent set portID_ $port
	
	if [Simulator set EnableHierRt_] {
		set nodeaddr [AddrParams set-hieraddr $address_]
	} else {
		set nodeaddr [expr ($address_ & [AddrParams set NodeMask_(1)])\
				<< [AddrParams set NodeShift_(1)]]
	}
	$agent set addr_ [expr (($port & $mask) << $shift) |	\
			( ~($mask << $shift) & $nodeaddr ) ]
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
	$agent set addr_ 0
	$agent target $nullagent
	
	set port [$agent set portID_]
	
	#Install nullagent to sink transit packets   
	#    $dmux_ clear $port
	$dmux_ install $port $nullagent
}

Node instproc agent port {
	$self instvar agents_
	foreach a $agents_ {
		if { [$a set portID_] == $port } {
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
	return $neighbor_
}

#
# helpers for PIM stuff
#

Node instproc get-vif {} {
	$self instvar vif_ 
	if [info exists vif_] { return $vif_ }
	set vif_ [new NetInterface]
	$self addInterface $vif_
	return $vif_
}

# List of corresponding peer TCP hosts from this node, used in IntTcp

Node instproc addCorresHost {addr cw mtu maxcw wndopt } {
	$self instvar chaddrs_
	
	if { ![info exists chaddrs_($addr)] } {
		set chost [new Agent/Chost $addr $cw $mtu $maxcw $wndopt]
		set chaddrs_($addr) $chost
	}
	return $chaddrs_($addr)
}

Node instproc createTcpSession {dst} {
	$self instvar tcpSession_
	
	if { ![info exists tcpSession_($dst)] } {
		set tcpsession [new Agent/TCP/Session]
		$self attach $tcpsession
		$tcpsession set dst_ $dst
		set tcpSession_($dst) $tcpsession
	}
	return $tcpSession_($dst)
}

Node instproc getTcpSession {dst} {
	$self instvar tcpSession_
	
	if { [info exists tcpSession_($dst)] } {
		return $tcpSession_($dst)
	} else {
		puts "In getTcpSession(): no session exists for destination [$dst set addr_]"
		return ""
    }
}

Node instproc existsTcpSession {dst} {
	$self instvar tcpSession_
	
	if { [info exists tcpSession_($dst)] } {
		return 1
	} else {
		return 0
	}
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
			# 1. get new MultiPathClassifier,
			# 2. migrate existing routes to that mclassifier
			# 3. install the mclassifier in the node classifier_
			#
            set mpathClsfr_($id) [new Classifier/MultiPath]
            if {$routes_($id) > 0} {
		    assert {$routes_($id) == 1}
		    $mpathClsfr_($id) installNext [$classifier_ in-slot? $id]
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
			}
		}
	} else {
		$self add-route $id $nullagent
		incr routes_($id) -1
	}
}

Node instproc intf-changed { } {
	$self instvar rtObject_
	if [info exists rtObject_] {	;# i.e. detailed dynamic routing
		$rtObject_ intf-changed
	}
}


### PGM Router support ###

# Simulator instproc attach-PgmNE (args) {
#     foreach node $args {
# 	$node attach-PgmNEAgent
#     }

# Node instproc attach-PgmNEAgent {} {
#     $self instvar switch_ router_supp_
#     # if![Simulator set EnableMcast_] {
#     # error "Error :Attaching PGM without Mcast option!"
#     # 	}
#     set router_supp_ [new Agent/NE/Pgm $switch_]
#     [Simulator instance] attach-agent $self $router-supp_
#     $switch_ install 1 $router-supp_
# }



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
		# $classifier_ set-hash auto 0 $dst_address 0 $slot
	}
# 	puts "ManualRtNode::add-route: $dst $target, classifier=$classifier_ slot=$slot"
#	puts "\t*slot=[$classifier_ slot $slot]"
}

ManualRtNode instproc add-route-to-adj-node args {
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
	set link [$ns nodes-to-link $self $target_node]
	set target [$link head]
	# puts "ManualRtNode::add-route-to-adj-node: in $self for addr $dst to target $target"
	return [$self add-route $dst $target]
}








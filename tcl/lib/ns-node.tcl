# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
# Time-stamp: <2000-08-29 12:05:58 haoboy>
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-node.tcl,v 1.73 2000/08/29 19:28:03 haoboy Exp $
#

Node set nn_ 0
Node proc getid {} {
	set id [Node set nn_]
	Node set nn_ [expr $id + 1]
	return $id
}

Node instproc init args {
	eval $self next $args
        $self instvar id_ agents_ dmux_ neighbor_ rtsize_ address_
        $self instvar nodetype_
        $self instvar multiPath_

        set nodetype_ [[Simulator instance] get-nodetype]

	set neighbor_ ""
	set agents_ ""
	set dmux_ ""
	set id_ [Node getid]
        set rtsize_ 0
        $self set-node-address$nodetype_ $args
	$self mk-default-classifier$nodetype_
	$self cmd addr $address_; # new by tomh
	set multiPath_ [$class set multiPath_]
}

Node instproc node-type {} {
	return [$self set nodetype_]
}

Node instproc set-node-address { args } {
}

Node instproc set-node-addressMIPBS { args } {
}

Node instproc set-node-addressMIPMH { args } {
}

Node instproc set-node-addressMobile { args } {
    
    $self instvar nifs_ arptable_
    $self instvar netif_ mac_ ifq_ ll_
    $self instvar X_ Y_ Z_

    set X_ 0.0
    set Y_ 0.0
    set Z_ 0.0
    set arptable_ ""                ;# no ARP table yet
    set nifs_	0		;# number of network interfaces
}

Node instproc set-node-addressHier {args} {
	$self instvar address_
	set address_ $args
}

Node instproc set-node-addressBase {args} {
}

Node instproc mk-default-classifierMIPMH {} {
    $self mk-default-classifierBase
}

Node instproc mk-default-classifierMIPBS {} {
    $self mk-default-classifierBase
}

Node instproc mk-default-classifierBase {} {
    $self mk-default-classifierHier
}

Node instproc mk-default-classifierHier {} {
	$self instvar id_ classifiers_ agents_ dmux_ neighbor_ address_ 
	set levels [AddrParams set hlevel_]
	for {set n 1} {$n <= $levels} {incr n} {
		set classifiers_($n) [new Classifier/Addr]
		$classifiers_($n) set mask_ [AddrParams set NodeMask_($n)]
		$classifiers_($n) set shift_ [AddrParams set NodeShift_($n)]
	}
	$self set classifier_ $classifiers_(1)
}

# mobileNode

Node instproc mk-default-classifierMobile {} {
    $self mk-default-classifier
}

## splitting up address str: used by other non-hier classes
Node instproc split-addrstr addrstr {
	set L [split $addrstr .]
	return $L
}

Node instproc mk-default-classifier {} {
	$self instvar address_ classifier_ id_
	set classifier_ [new Classifier/Hash/Dest 32]
	# set up classifer as a router (default value 8 bit of addr and 
	# 8 bit port)
	$classifier_ set mask_ [AddrParams set NodeMask_(1)]
	$classifier_ set shift_ [AddrParams set NodeShift_(1)]
	if ![info exists address_] {
		set address_ $id_
	}
}

Node instproc enable-mcast sim {
	$self instvar classifier_ multiclassifier_ ns_ switch_ mcastproto_
	$self set ns_ $sim
	
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
        $self instvar nodetype_
        return [$self entry-New$nodetype_]
}

Node instproc entry-NewMIPBS {} {
    return [$self entry-NewBase]
}

Node instproc entry-NewMIPMH {} {
    return [$self entry-NewBase]
}

Node instproc entry-NewBase {} {
    $self instvar ns_
    if ![info exist ns_] {
	set ns_ [Simulator instance]
    }
    if [$ns_ multicast?] {
	$self instvar switch_
	return $switch_
    }
    $self instvar classifiers_
    return $classifiers_(1)
}

Node instproc entry-New {} {
	if [info exists router_supp_] {
		return $router_supp_
	}
	if ![info exist ns_] {
		set ns_ [Simulator instance]
	}
	if [$ns_ multicast?] {
		$self instvar switch_
		return $switch_
	}
	$self instvar classifier_
	return $classifier_
}

Node instproc entry-NewHier {} {
	return [$self entry-New]
}

Node instproc entry-NewMobile {} {
	return [$self entry-New]
}

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

# Stupid hack. Can't you say [Simulator instance] get-nam-traceall{} ?
Node instproc get-nam-traceall {args} {

  $self instvar namtraceFile_
  set file [lindex $args 0]
  set namtraceFile_ $file

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
	# XXX Why do this check at all, since now all ports are 32 bits?
#  	if { $np_ < $p } {
#  		set np_ $p
#  	}
#  	if {$np_ > [$dmux_ set mask_] && [$dmux_ set mask_] > 0 } {
#  		warn "No. of ports($np_) attached to node $id_ is greater than allowed"
#  	}
#  	return $p
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
        $self instvar nodetype_
        $self add-target-New$nodetype_ $agent $port
}
	
Node instproc add-target-New {agent port} {
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
	
Node instproc add-target-NewBase {agent port} {
	$self add-target-NewMobile $agent $port
}

Node instproc add-target-NewHier {agent port} {
	$self add-target-New $agent $port
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
        $self instvar nodetype_
        $self do-reset$nodetype_
}

Node instproc do-reset { } {
	$self instvar agents_
	foreach a $agents_ {
		$a reset
	}
}

Node instproc do-resetMobile {} {
    	$self instvar arptable_ nifs_
	$self instvar netif_ mac_ ifq_ ll_
	$self instvar imep_

        for {set i 0} {$i < $nifs_} {incr i} {
	    $netif_($i) reset
	    $mac_($i) reset
	    $ll_($i) reset
	    $ifq_($i) reset
	    if { [info exists opt(imep)] && $opt(imep) == "ON" } { 
		    $imep_($i) reset 
	    }
	}
	if { $arptable_ != "" } {
		$arptable_ reset 
	}
}

Node instproc do-resetBase {} {
	$self do-resetMobile
}

Node instproc do-resetHier {} {
	$self do-reset
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
# Helpers for interface stuff
#
Node instproc addInterface { iface } {
	$self instvar ifaces_
	lappend ifaces_ $iface
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
	set link [$ns link $self $target_node]
	set target [$link head]
	# puts "ManualRtNode::add-route-to-adj-node: in $self for addr $dst to target $target"
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
	$self instvar node_ ns_ routingTable_

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

# New node structure

Node instproc add-target-NewMobile {agent port} {
    $self instvar dmux_ classifier_
    $self instvar imep_ toraDebug_
 
    set ns_ [Simulator instance]

    set newapi [$ns_ imep-support]
    $agent set sport_ $port

    #special processing for TORA/IMEP node
    set toraonly [string first "TORA" [$agent info class]] 

    if {$toraonly != -1 } {
	$agent if-queue [$self set ifq_(0)]    ;# ifq between LL and MAC
	 #
        # XXX: The routing protocol and the IMEP agents needs handles
        # to each other.
        #
        $agent imep-agent [$self set imep_(0)]
        [$self set imep_(0)] rtagent $agent

    }
    
    if { $port == 255 } {			# non-routing agents
	if { [Simulator set RouterTrace_] == "ON" } {
	    #
	    # Send Target
	    #
	    if {$newapi != ""} {
	        set sndT [$ns_ mobility-trace Send "RTR" $self]
	    } else {
		set sndT [cmu-trace Send "RTR" $self]
	    }
            if [info exists namtraceFile_]  {
               $sndT namattach $namtraceFile_
            }

            if { $newapi == "ON" } {
                 $agent target $imep_(0)
                 $imep_(0) sendtarget $sndT

		 # second tracer to see the actual
                 # types of tora packets before imep packs them
		  if { [info exists toraDebug_] && $toraDebug_ == "ON"} {
                       set sndT2 [$ns_ mobility-trace Send "TRP" $self]
                       $sndT2 target $imep_(0)
                       $agent target $sndT2
                 }
             } else {  ;#  no IMEP
                  $agent target $sndT
             }
	    
	    $sndT target [$self set ll_(0)]
#	    $agent target $sndT
	    
	    #
	    # Recv Target
	    #
	     
	    if {$newapi != ""} {
	         set rcvT [$ns_ mobility-trace Recv "RTR" $self]
	    } else {
		 set rcvT [cmu-trace Recv "RTR" $self]
	    }
            if [info exists namtraceFile_]  {
               $rcvT namattach $namtraceFile_
            }

            if {$newapi == "ON" } {
            #    puts "Hacked for tora20 runs!! No RTR revc trace"
                [$self set ll_(0)] up-target $imep_(0)
                $classifier_ defaulttarget $agent

                # need a second tracer to see the actual
                # types of tora packets after imep unpacks them
		# no need to support any hier node

		if {[info exists toraDebug_] && $toraDebug_ == "ON" } {
		    set rcvT2 [$ns_ mobility-trace Recv "TRP" $self]
		    $rcvT2 target $agent
		    [$self set classifier_] defaulttarget $rcvT2
		}
		
             } else {
                 $rcvT target $agent
		 $self install-defaulttarget $rcvT
                 $dmux_ install $port $rcvT
	     }
	    
	} else {
	    #
	    # Send Target
	    #
	    $agent target [$self set ll_(0)]
	    #
	    # Recv Target
	    #
	    $self install-defaulttarget $agent
	    $dmux_ install $port $agent
	}
	
    } else {
	if { [Simulator set AgentTrace_] == "ON" } {
	    #
	    # Send Target
	    #
	   
	    if {$newapi != ""} {
	        set sndT [$ns_ mobility-trace Send AGT $self]
	    } else {
		set sndT [cmu-trace Send AGT $self]
	    }

	    $sndT target [$self entry]
	    $agent target $sndT
		
	    #
	    # Recv Target
	    #
	    if {$newapi != ""} {
	        set rcvT [$ns_ mobility-trace Recv AGT $self]
	    } else {
		set rcvT [cmu-trace Recv AGT $self]
	    }

	    $rcvT target $agent
	    $dmux_ install $port $rcvT
		
	} else {
	    #
	    # Send Target
	    #
	    $agent target [$self entry]
	    
	    #
	    # Recv Target
	    #
	    $dmux_ install $port $agent
	}
    }

}

Node instproc add-interface { channel pmodel \
		lltype mactype qtype qlen iftype anttype} {

	$self instvar arptable_ nifs_
	$self instvar netif_ mac_ ifq_ ll_
	$self instvar imep_
	
	#global ns_ opt
	#set MacTrace [Simulator set MacTrace_]

	set ns_ [Simulator instance]


	set imepflag [$ns_ imep-support]

	set t $nifs_
	incr nifs_

	set netif_($t)	[new $iftype]		;# interface
	set mac_($t)	[new $mactype]		;# mac layer
	set ifq_($t)	[new $qtype]		;# interface queue
	set ll_($t)	[new $lltype]		;# link layer
        set ant_($t)    [new $anttype]

        if {$imepflag == "ON" } {              ;# IMEP layer
            set imep_($t) [new Agent/IMEP [$self id]]
            set imep $imep_($t)

            set drpT [$ns_ mobility-trace Drop "RTR" $self]
            $imep drop-target $drpT
            if [info exists namtraceFile_]  {
               $drpT namattach $namtraceFile_
            }
            $ns_ at 0.[$self id] "$imep_($t) start"     ;# start beacon timer
        }


	#
	# Local Variables
	#
	set nullAgent_ [$ns_ set nullAgent_]
	set netif $netif_($t)
	set mac $mac_($t)
	set ifq $ifq_($t)
	set ll $ll_($t)

	#
	# Initialize ARP table only once.
	#
	if { $arptable_ == "" } {
            set arptable_ [new ARPTable $self $mac]
            # FOR backward compatibility sake, hack only
	    
	    if {$imepflag != ""} {
                set drpT [$ns_ mobility-trace Drop "IFQ" $self]
	    } else {
		set drpT [cmu-trace Drop "IFQ" $self]
	    }
	    
	    $arptable_ drop-target $drpT
        }

	#
	# Link Layer
	#
	$ll arptable $arptable_
	$ll mac $mac
#	$ll up-target [$self entry]
	$ll down-target $ifq

	if {$imepflag == "ON" } {
	    $imep recvtarget [$self entry]
            $imep sendtarget $ll
            $ll up-target $imep
        } else {
            $ll up-target [$self entry]
	}



	#
	# Interface Queue
	#
	$ifq target $mac
	$ifq set qlim_ $qlen

	if {$imepflag != ""} {
	    set drpT [$ns_ mobility-trace Drop "IFQ" $self]
	} else {
	    set drpT [cmu-trace Drop "IFQ" $self]
        }
	$ifq drop-target $drpT

	#
	# Mac Layer
	#
	$mac netif $netif
	$mac up-target $ll
	$mac down-target $netif
	#$mac nodes $opt(nn)
	set god_ [God instance]
	$mac nodes [$god_ num_nodes]

	#
	# Network Interface
	#
	$netif channel $channel
	$netif up-target $mac
	$netif propagation $pmodel	;# Propagation Model
	$netif node $self		;# Bind node <---> interface
	$netif antenna $ant_($t)

	#
	# Physical Channel`
	#
	$channel addif $netif

	# ============================================================

	if { [Simulator set MacTrace_] == "ON" } {
	    #
	    # Trace RTS/CTS/ACK Packets
	    #

	    if {$imepflag != ""} {
	        set rcvT [$ns_ mobility-trace Recv "MAC" $self]
	    } else {
		set rcvT [cmu-trace Recv "MAC" $self]
	    }
	    $mac log-target $rcvT
            if [info exists namtraceFile_]  {
               $rcvT namattach $namtraceFile_
            }


	    #
	    # Trace Sent Packets
	    #
	    if {$imepflag != ""} {
	        set sndT [$ns_ mobility-trace Send "MAC" $self]
	    } else {
		set sndT [cmu-trace Send "MAC" $self]
	    }
	    $sndT target [$mac down-target]
	    $mac down-target $sndT
            if [info exists namtraceFile_]  {
               $sndT namattach $namtraceFile_
            }

	    #
	    # Trace Received Packets
	    #

	    if {$imepflag != ""} {
		set rcvT [$ns_ mobility-trace Recv "MAC" $self]
	        
	    } else {
	        set rcvT [cmu-trace Recv "MAC" $self]
	    }
	    $rcvT target [$mac up-target]
	    $mac up-target $rcvT
            if [info exists namtraceFile_]  {
               $rcvT namattach $namtraceFile_
            }

	    #
	    # Trace Dropped Packets
	    #
	    if {$imepflag != ""} {
	        set drpT [$ns_ mobility-trace Drop "MAC" $self]
	    } else {
		set drpT [cmu-trace Drop "MAC" $self]`
	    }
	    $mac drop-target $drpT
            if [info exists namtraceFile_]  {
               $drpT namattach $namtraceFile_
            }
	} else {
	    $mac log-target [$ns_ set nullAgent_]
	    $mac drop-target [$ns_ set nullAgent_]
	}

	# ============================================================

	$self addif $netif
}

Node instproc agenttrace {tracefd} {
    set ns_ [Simulator instance]
    set ragent [$self set ragent_]
    #
    # Drop Target (always on regardless of other tracing)
    #
    set drpT [$ns_ mobility-trace Drop "RTR" $self]
    $ragent drop-target $drpT
    #
    # Log Target
    #
    set T [new Trace/Generic]
    $T target [$ns_ set nullAgent_]
    $T attach $tracefd
    $T set src_ [$self id]
    #$ragent log-target $T
    $ragent tracetarget $T
    #
    # XXX: let the IMEP agent use the same log target.
    #
    set imepflag [$ns_ imep-support]
    if {$imepflag == "ON"} {
       [$self set imep_(0)] log-target $T
    }

    if [info exists namtraceFile_]  {
       $drpT namattach $namtraceFile_
    }
}

Node instproc nodetrace { tracefd } {
    set ns_ [Simulator instance]
    #
    # This Trace Target is used to log changes in direction
    # and velocity for the mobile node.
    #
    set T [new Trace/Generic]
    $T target [$ns_ set nullAgent_]
    $T attach $tracefd
    $T set src_ [$self id]
    $self log-target $T    

}

Node instproc install-defaulttarget {rcvT} {
    #set nodetype [[Simulator instance] get-nodetype]
    $self instvar nodetype_
    $self install-defaulttarget-New$nodetype_ $rcvT

}

Node instproc install-defaulttarget-New {rcvT} {
    [$self set classifier_] defaulttarget $rcvT
}

Node instproc install-defaulttarget-NewMobile {rcvT} {
    [$self set classifier_] defaulttarget $rcvT
}

Node instproc install-defaulttarget-NewBase {rcvT} {
    $self instvar classifiers_
    set level [AddrParams set hlevel_]
    for {set i 1} {$i <= $level} {incr i} {
		$classifiers_($i) defaulttarget $rcvT
    }
}

Node instproc install-defaulttarget-NewMIPMH {rcvT} {
    $self install-defaulttarget-NewBase $rcvT
}

Node instproc install-defaulttarget-NewMIPBS {rcvT} {
    $self install-defaulttarget-NewBase $rcvT
}

# set transmission power
Node instproc setPt { val } {
    $self instvar netif_
    $netif_(0) setTxPower $val
}

# set receiving power
Node instproc setPr { val } {
    $self instvar netif_
    $netif_(0) setRxPower $val
}

# set idle power -- Chalermek
Node instproc setPidle { val } {
    $self instvar netif_
    $netif_(0) setIdlePower $val
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

# List of corresponding peer TCP hosts from this node, used in IntTcp

#  Node instproc addCorresHost {addr cw mtu maxcw wndopt } {
#  	$self instvar chaddrs_
	
#  	if { ![info exists chaddrs_($addr)] } {
#  		set chost [new Agent/Chost $addr $cw $mtu $maxcw $wndopt]
#  		set chaddrs_($addr) $chost
#  	}
#  	return $chaddrs_($addr)
#  }

#  Node instproc createTcpSession {dst} {
#  	$self instvar tcpSession_
	
#  	if { ![info exists tcpSession_($dst)] } {
#  		set tcpsession [new Agent/TCP/Session]
#  		$self attach $tcpsession
#  		$tcpsession set dst_ $dst
#  		set tcpSession_($dst) $tcpsession
#  	}
#  	return $tcpSession_($dst)
#  }

#  Node instproc getTcpSession {dst} {
#  	$self instvar tcpSession_
	
#  	if { [info exists tcpSession_($dst)] } {
#  		return $tcpSession_($dst)
#  	} else {
#  		puts "In getTcpSession(): no session exists for destination [$dst set addr_]"
#  		return ""
#      }
#  }

#  Node instproc existsTcpSession {dst} {
#  	$self instvar tcpSession_
	
#  	if { [info exists tcpSession_($dst)] } {
#  		return 1
#  	} else {
#  		return 0
#  	}
#  }

#  #
#  # helpers for PIM stuff
#  #
#  Node instproc get-vif {} {
#  	$self instvar vif_ 
#  	if [info exists vif_] { return $vif_ }
#  	set vif_ [new NetInterface]
#  	$self addInterface $vif_
#  	return $vif_
#  }

#  Node instproc attachInterfaces ifs {
#  	$self instvar ifaces_
#  	set ifaces_ $ifs
#  	foreach ifc $ifaces_ {
#  		$ifc setNode $self
#  	}
#  }

#  Node instproc createInterface { num } {
#  	$self instvar ifaces_
#  	set newInterface [new NetInterface]
#  	if { $num < 0 } { return 0 }
#  	$newInterface set-label $num
#  	return 1
#  }

#  Node instproc getInterfaces {} {
#  	$self instvar ifaces_
#  	return $ifaces_
#  }

#  Node instproc getNode {} {  
#  	return $self
#  }



#
# vlan.tcl
# $Id: vlan.tcl
#
# Copyright (c) 1997 University of Southern California.
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

# LanNode----------------------------------------------------
#
# Lan implementation as a virtual node: LanNode mimics a real
# node and uses an address (id) from Node's address space
#
# problems: won't work with hierarchical routing, because 
# somebody has to assign a hierarchical address to the Lan.
#------------------------------------------------------------
Class LanNode -superclass InitObject
LanNode set ifqType_ Queue/DropTail
LanNode set macType_ Mac/Csma/Cd
LanNode set chanType_ Channel
LanNode set addr_ ""

LanNode instproc bw {val} { $self set bw_ $val }
LanNode instproc addr {val} { $self set addr_ $val }
LanNode instproc delay {val} { $self set delay_ $val }
LanNode instproc ifqType {val} { $self set ifqType_ $val }
LanNode instproc macType {val} { $self set macType_ $val }
LanNode instproc chanType {val} { $self set chanType_ $val }

LanNode instproc rtObject? {} {
	$self instvar ln_
	return [$ln_ rtObject?]
}
LanNode instproc id {} { $self set id_ }
LanNode instproc node-addr {{addr ""}} { 
	eval $self set addr_ $addr
}
LanNode instproc reset {} {
	#NOTHING: needed for node processing by ns routing
}
LanNode instproc dump-namconfig {} {
	$self instvar ns_ bw_ delay_ nodelist_ id_
	$ns_ puts-nam-config \
			"X -t * -n $id_ -r $bw_ -D $delay_ -o left"
	set cnt 0
	set LanOrient(0) "up"
	set LanOrient(1) "down"
	
	foreach n $nodelist_ {
		$ns_ puts-nam-config \
				"L -t * -s $id_ -d [$n id] -o $LanOrient($cnt)"
		set cnt [expr 1 - $cnt]
	}
}

LanNode instproc init {ns args} {
	set args [eval $self init-vars $args]
	$self instvar bw_ delay_ ifqType_ macType_ chanType_
	$self instvar ns_ nodelist_ ln_ defRouter_ switch_
	$self instvar id_ addr_ channel_ mcl_ netIface_ 
	$ns instvar Node_ EnableHierRt_

	set ns_ $ns
	set nodelist_ ""

	eval set ln_ [$ns node $addr_]
	set id_ [$ln_ id]
	set addr_ [$ln_ node-addr]
	set Node_($id_) $self

	set defRouter_ [new lanRouter]
	if {[info exists EnableHierRt_] && $EnableHierRt_} {
		$defRouter_ routing hier
	} else {
		$defRouter_ routing flat
	}
	$defRouter_ lanaddr $addr_
	$defRouter_ routelogic [$ns get-routelogic]
	$ns instvar EnableMcast_
	if [info exists EnableMcast_] {
		if $EnableMcast_ { 
			set switch_ [$ln_ set switch_]
			$defRouter_ switch $switch_
		}
	}

	set channel_ [new $chanType_]
	set mcl_ [new Classifier/Mac]
	$mcl_ set offset_ [PktHdr_offset PacketHeader/Mac macDA_]
	$channel_ target $mcl_
}

LanNode instproc add-route {dest mac} {
	#NOTHING: use defRouter to find routes
}

LanNode instproc assign-mac {ip} {
	return $ip ;# use ip addresses at MAC layer
}
LanNode instproc addNode {nodes bw delay {ifqType ""} {macType ""} } {
	$self instvar ifqType_ macType_ chanType_ 
	$self instvar id_ channel_ mcl_ lanIface_ ln_
	$self instvar ns_ nodelist_
	$ns_ instvar link_ Node_

	if {$ifqType == ""} { set ifqType $ifqType_ }
	if {$macType == ""} { set macType $macType_ }

	foreach src $nodes {
		set nif [new LanIface $src $bw $self -ifqType $ifqType -macType $macType]
		set mac [$nif set mac_]
		$mac set addr_ [$self assign-mac [$src node-addr]] ;# cf LL's arp(int ip)
		$mac channel $channel_
		$mac mcl $mcl_
		$mcl_ install [$mac set addr_] $mac

		set lanIface_([$src node-addr]) $nif

		# connect self (vnode) with lan nodes by virtual links
		set sid [$src id]
		set link_($sid:$id_) [new Vlink $ns_ $self $sid $id_ $bw 0]
		set link_($id_:$sid) [new Vlink $ns_ $self $id_ $sid $bw 0]
		$link_($sid:$id_) queue [$nif set ifq_]
		$link_($id_:$sid) queue [$nif set ifq_]
		# cost of each vlink is .5, so total from a node to a node is 1
		$link_($sid:$id_) cost .5
		$link_($id_:$sid) cost .5
	}
	set nodelist_ [concat $nodelist_ $nodes]
}

# LanIface---------------------------------------------------
#
# node's interface to a LanNode
#------------------------------------------------------------
Class LanIface -superclass Connector
LanIface set ifqType_ Queue/DropTail
LanIface set macType_ Mac/Csma/Cd
LanIface set llType_  LL

LanIface instproc llType {val} { $self set llType_ $val }
LanIface instproc ifqType {val} { $self set ifqType_ $val }
LanIface instproc macType {val} { $self set macType_ $val }

LanIface instproc init {node bw lan args} {
	set args [eval $self init-vars $args]
	eval $self next $args

	$self instvar llType_ ifqType_ macType_
	$self instvar node_ ifq_ mac_ ll_

	set node_ $node
	set ll_ [new $llType_]
	set ifq_ [new $ifqType_]
	set mac_ [new $macType_]

	$ll_ lanrouter [$lan set defRouter_]

	$ll_ sendtarget $mac_
	$ll_ recvtarget [$node entry]
	$ifq_ target $ll_
	$mac_ target $ll_

	$self target $ifq_
}

# Vlink------------------------------------------------------
#
# Virtual link  implementation. Mimics a Simple Link but with
# zero delay and infinite bandwidth
#------------------------------------------------------------
Class Vlink
Vlink instproc dump-nam-queueconfig {} {
	#nothing
}
Vlink instproc up? {} {
	return "up"
}
Vlink instproc queue {{q ""}} {
	eval $self set queue_ $q
}
Vlink instproc init {ns lan src dst b d} {
	$self instvar ns_ lan_ src_ dst_ bw_ delay_

	set ns_ $ns
	set lan_ $lan
	set src_ $src
	set dst_ $dst
	set bw_ $b
	set delay_ $d
}

# if this is a link TO the lan vnode, return the lanIface object
# if this is a link FROM the lan vnode, return the MAC object of the dest.
Vlink instproc head {} {
	$self instvar lan_ dst_ src_
	if {$src_ == [$lan_ set id_]} {
		# from the LAN vnode 
		set dst_lif [$lan_ set lanIface_($dst_)]
		return  [$dst_lif set mac_]
	} else {
		# to the LAN
		set src_lif [$lan_ set lanIface_($src_)]
		return $src_lif
	}
}

Vlink instproc cost c { $self set cost_ $c}	
Vlink instproc cost? {} {
	$self instvar cost_
	if ![info exists cost_] {
		return 1
	}
	return $cost_
}


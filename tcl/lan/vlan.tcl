
#
# vlan.tcl
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
# WARNING: if used with hierarchical routing, one has to assign
# a hierarchical address to the lan itself.  This maybe confusing.
#------------------------------------------------------------
Class LanNode -superclass InitObject
LanNode set ifqType_ Queue/DropTail
LanNode set macType_ Mac/Csma/Cd
LanNode set chanType_ Channel
LanNode set address_ ""

LanNode instproc address {val} { $self set address_ $val }
LanNode instproc bw {val} { $self set bw_ $val }
LanNode instproc delay {val} { $self set delay_ $val }
LanNode instproc ifqType {val} { $self set ifqType_ $val }
LanNode instproc macType {val} { $self set macType_ $val }
LanNode instproc chanType {val} { $self set chanType_ $val }

LanNode instproc init {ns args} {
	set args [eval $self init-vars $args]
	$self instvar bw_ delay_ ifqType_ macType_ chanType_
	$self instvar ns_ nodelist_ defRouter_ cost_
	$self instvar id_ address_ channel_ mcl_ netIface_ 
	$ns instvar Node_

	$self next
	set ns_ $ns
	set nodelist_ ""
	set cost_ 1

	set id_ [Node getid]
	set Node_($id_) $self
	if [Simulator set EnableHierRt_] {
		if {$address_ == ""} {
			error "LanNode: use \"-address\" option \
					with hierarchical routing"
		}
	} else {
		set address_ $id_
	}
	set defRouter_ [new lanRouter $ns $self]
	if [Simulator set EnableMcast_] {
		set switch_ [new Classifier/Addr]
		$switch_ set mask_ [AddrParams set McastMask_]
		$switch_ set shift_ [AddrParams set McastShift_]

		$defRouter_ switch $switch_
	}
	set channel_ [new $chanType_]
	set mcl_ [new Classifier/Mac]
	$mcl_ set offset_ [PktHdr_offset PacketHeader/Mac macDA_]
	$channel_ target $mcl_
}
LanNode instproc addNode {nodes bw delay {ifqType ""} {macType ""} } {
	$self instvar ifqType_ macType_ chanType_ 
	$self instvar id_ channel_ mcl_ lanIface_
	$self instvar ns_ nodelist_ cost_
	$ns_ instvar link_ Node_

	if {$ifqType == ""} { set ifqType $ifqType_ }
	if {$macType == ""} { set macType $macType_ }

	set vlinkcost [expr $cost_ / 2.0]
	foreach src $nodes {
		set nif [new LanIface $src $bw $self -ifqType $ifqType -macType $macType]

		set mac [$nif set mac_]
		set ipAddr [AddrParams set-hieraddr [$src node-addr]]
		set macAddr [$self assign-mac $ipAddr] ;# cf LL's arp(int ip)
		$mac set addr_ $macAddr

		$mac channel $channel_
		$mac mcl $mcl_
		$mcl_ install [$mac set addr_] $mac

		set lanIface_([$src id]) $nif
		$nif instvar iface_ ;#interface object
		$ns_ set interfaces_($src:$self) $iface_
		$ns_ set interfaces_($self:$src) $iface_
		$src add-neighbor $self

		set sid [$src id]
		set link_($sid:$id_) [new Vlink $ns_ $self $sid $id_ $bw 0]
		set link_($id_:$sid) [new Vlink $ns_ $self $id_ $sid $bw 0]
		$link_($sid:$id_) set ifacein_ $nif
		$nif set intf_label_ [[$iface_ entry] set intf_label_]

		$link_($sid:$id_) queue [$nif set ifq_]
		$link_($id_:$sid) queue [$nif set ifq_]

		$link_($sid:$id_) cost $vlinkcost
		$link_($id_:$sid) cost $vlinkcost
	}
	set nodelist_ [concat $nodelist_ $nodes]
}
LanNode instproc assign-mac {ip} {
	return $ip ;# use ip addresses at MAC layer
}
LanNode instproc cost c {
	$self instvar ns_ nodelist_ id_ cost_
	$ns_ instvar link_
	set cost_ $c
	set vlinkcost [expr $c / 2.0]
	foreach node $nodelist_ {
		set nid [$node id]
		$link_($id_:$nid) cost $vlinkcost
		$link_($nid:$id_) cost $vlinkcost
	}
}
LanNode instproc cost? {} {
	$self instvar cost_
	return $cost_
}
LanNode instproc rtObject? {} {
	# NOTHING
}
LanNode instproc id {} { $self set id_ }
LanNode instproc node-addr {{addr ""}} { 
	eval $self set address_ $addr
}
LanNode instproc reset {} {
	# NOTHING: needed for node processing by ns routing
}
LanNode instproc is-lan? {} { return 1 }
LanNode instproc dump-namconfig {} {
	# Redefine this function if needed
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
LanNode instproc init-outLink {} { 
	#NOTHING
}
LanNode instproc start-mcast {} { 
	# NOTHING
}
LanNode instproc getArbiter {} {
	# just give an arbiter to those protocols who want it.
	$self instvar arbiter_
	if ![info exists arbiter_] {
		set arbiter_ [new McastProtoArbiter ""]
	}
	return $arbiter_
}
LanNode instproc attach {agent} {
	# NOTHING
}
LanNode instproc add-route {args} {
	# NOTHING: use defRouter to find routes
}
LanNode instproc add-hroute {args} {
	# NOTHING: use defRouter to find routes
}
LanNode instproc split-addrstr addrstr {
	set L [split $addrstr .]
	return $L
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
	$self instvar node_ ifq_ mac_ ll_ iface_

	set node_ $node
	set ll_ [new $llType_]
	set ifq_ [new $ifqType_]
	set mac_ [new $macType_]
	set iface_ [new NetInterface]

	$ll_ lanrouter [$lan set defRouter_]

	$self target $ifq_
	$ifq_ target $ll_
	$ll_ sendtarget $mac_
	$ll_ recvtarget [$iface_ entry]
	$mac_ target $ll_

	$node addInterface $iface_
}

# Vlink------------------------------------------------------
#
# Virtual link  implementation. Mimics a Simple Link but with
# zero delay and infinite bandwidth
#------------------------------------------------------------
Class Vlink
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
Vlink instproc dump-nam-queueconfig {} {
	#nothing
}
Vlink instproc head {} {
	$self instvar lan_ dst_ src_
	if {$src_ == [$lan_ set id_]} {
		# if this is a link FROM the lan vnode, 
		# return the LL object of the dest.
		set dst_lif [$lan_ set lanIface_($dst_)]
		return  [$dst_lif set ll_]
	} else {
		# if this is a link TO the lan vnode, 
		# return the lanIface object
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

# lanRouter--------------------------------------------------
#
# "Virtual node lan" needs to know which of the lan nodes is
# the next hop towards the packet's (maybe remote) destination.
#------------------------------------------------------------
lanRouter instproc init {ns lan} {
	$self next
	Simulator instvar EnableHierRt_
	if {$EnableHierRt_} {
		$self routing hier
	} else {
		$self routing flat
	}
	$self lanaddr [$lan node-addr]
	$self routelogic [$ns get-routelogic]
}

Node instproc is-lan? {} { return 0 }

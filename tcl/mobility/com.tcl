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

# @(#) $Header: 
#

proc create-base-station-node {address } {
    global topo tracefd opt node node_ ns_
    set ns_ [Simulator instance]
    if [Simulator set mobile_ip_] {
	Simulator set node_factory_ MobileNode/MIPBS
    } else {
	Simulator set node_factory_ Node/MobileNode/BaseStationNode
    }
    #set ns_ $ns
    set node [$ns_ node $address]
    set id [$node id]
    #set node_($id) $node    ;#XXX does wireless code use this global
	                         # array?
	
    $node random-motion 0		;# disable random motion
    $node topography $topo
    #
    # This Trace Target is used to log changes in direction
    # and velocity for the mobile node.
    #
    set T [new Trace/Generic]
    $T target [$ns_ set nullAgent_]
    $T attach $tracefd
    $T set src_ $id
    $node log-target $T
    #$node base-station $node
    $node base-station [AddrParams set-hieraddr [$node node-addr]]
    
    
    create-$opt(rp)-bs-node $node $id
    
    Simulator set node_factory_ Node    ;# default value
    return $node
}


proc create-dsdv-bs-node {node id} {
    global ns_ chan prop opt node_
    $node instvar regagent_ ragent_

    $node add-interface $chan $prop $opt(ll) $opt(mac)	\
	    $opt(ifq) $opt(ifqlen) $opt(netif) \
	    $opt(ant)
    
    create-$opt(rp)-routing-agent $node $id

    if [info exists regagent_] {
	$regagent_ ragent $ragent_
    }
    if { $opt(pos) == "Box" } {
		#
		# Box Configuration
		#
		set spacing 200
		set maxrow 7
		set col [expr ($id - 1) % $maxrow]
		set row [expr ($id - 1) / $maxrow]
		$node set X_ [expr $col * $spacing]
		$node set Y_ [expr $row * $spacing]
		$node set Z_ 0.0
		$node set speed_ 0.0

		$ns_ at 0.0 "$node_($id) start"
    }
}

proc create-dsr-bs-node {node id} {
    global ns_ chan prop opt
    $node instvar regagent_ ragent_
    
    $node add-interface $chan $prop $opt(ll) $opt(mac)	\
	    $opt(ifq) $opt(ifqlen) $opt(netif) \
	    $opt(ant)
    
    create-$opt(rp)-routing-agent $node $id
    $node create-xtra-interface 
    
    if [info exists regagent_] {
	$regagent_ ragent $ragent_
    }
    
    $ns_ at 0.0 "$node start-dsr"
}


proc create-dsr-routing-agent { node id } {
    global ns_ ragent_ tracefd RouterTrace AgentTrace opt

    # 
    # Create routing agent and attach it to port 255
    #
    set ragent_($id) [new Agent/DSRAgent/BS_DSRAgent]
    set ragent $ragent_($id)
    
    # setup address (supports hier-addr) for dsdv agent 
    set address [$node node-addr]
    $ragent addr $address
    $ragent node $node
    if [Simulator set mobile_ip_] {
	$ragent port-dmux [$node set dmux_]
    }
    
    $node addr $address
    $node set ragent_ $ragent
    
    set dmux [$node set dmux_]
    if {$dmux == "" } {
	set dmux [new Classifier/Addr]
	$dmux set mask_ [AddrParams set PortMask_]
	$dmux set shift_ [AddrParams set PortShift_]
	#
	# point the node's routing entry to itself
	# at the port demuxer (if there is one)
	#
	if [Simulator set EnableHierRt_] {
	    $node add-hroute $address $ragent
	} else {
	    $node add-route $address $ragent
	}
	$node set dmux_ $dmux
    }
    set level [AddrParams set hlevel_]
    
    if { $RouterTrace == "ON" } {
	#
	# Recv Target
	#
	set rcvT [cmu-trace Recv "RTR" $node]
	$rcvT target $ragent
	for {set i 1} {$i <= $level} {incr i} {
	    [$node set classifiers_($i)] defaulttarget $rcvT
	    [$node set classifiers_($i)] bcast-receiver $rcvT
	}
    } else {
	
	for {set i 1} {$i <= $level} {incr i} {
	    [$node set classifiers_($i)] defaulttarget $ragent
	    [$node set classifiers_($i)] bcast-receiver $ragent
	}
    }
    #
    # Drop Target (always on regardless of other tracing)
    #
    set drpT [cmu-trace Drop "RTR" $node]
    $ragent drop-target $drpT
    
    #
    # Log Target
    #
    #set T [new Trace/Generic]
    #$T target [$ns_ set nullAgent_]
    #$T attach $tracefd
    #$T set src_ [$node id]
    #$ragent log-target $T

    $ragent target $dmux
    
    # packets to the DSR port should be handed over to ragent_
    # since all pkts now donot go thru ragent
    $dmux install $opt(rt_port) $ragent
}


Node/MobileNode/BaseStationNode instproc create-xtra-interface { } {
    global ns_ opt RouterTrace
    $self instvar ragent_ ll_ mac_ ifq_
    
    $ragent_ mac-addr [$mac_(0) id]

    if { $RouterTrace == "ON" } {
	# Send Target
	set sndT [cmu-trace Send "RTR" $self]
	$sndT target $ll_(0)
	$ragent_ add-ll $sndT $ifq_(0)
    } else {
	# Send Target
	$ragent_ add-ll $ll_(0) $ifq_(0)
    }
    
    # setup promiscuous tap into mac layer
    $ragent_ install-tap $mac_(0)
    
}

Node/MobileNode/BaseStationNode instproc start-dsr {} {
    $self instvar ragent_
    global opt;

    $ragent_ startdsr
    if {$opt(cc) == "on"} {checkcache $dsr_agent_}
}

Node/MobileNode/BaseStationNode instproc reset args {
    $self instvar ragent_
    eval $self next $args

    $ragent_ reset
}


proc create-god { nodes } {
	global ns_ god_ tracefd

	set god_ [new God]
	$god_ num_nodes $nodes
}



proc cmu-trace { ttype atype node } {
	global ns_ tracefd

	if { $tracefd == "" } {
		return ""
	}
	set T [new CMUTrace/$ttype $atype]
	$T target [$ns_ set nullAgent_]
	$T attach $tracefd
        $T set src_ [$node id]

        $T node $node

	return $T
}


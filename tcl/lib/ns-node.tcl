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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-node.tcl,v 1.14 1997/10/21 00:50:32 ahelmy Exp $
#

Class Node
Node set nn_ 0
Node proc getid {} {
	set id [Node set nn_]
	Node set nn_ [expr $id + 1]
	return $id
}

Node instproc init args {
	eval $self next $args
	$self instvar np_ id_ classifier_ agents_ dmux_ neighbor_
	set neighbor_ ""
	set agents_ ""
	set dmux_ ""
	set np_ 0
	set id_ [Node getid]
	set classifier_ [new Classifier/Addr]
	# set up classifer as a router (i.e., 24 bit of addr and 8 bit port)
	$classifier_ set mask_ 0xffffff
	$classifier_ set shift_ 8
}

Node instproc enable-mcast sim {
        $self instvar classifier_ multiclassifier_ ns_ switch_ mcastproto_
	$self set ns_ $sim

	$self set switch_ [new Classifier/Addr]
        #
	# set up switch to route unicast packet to slot 0 and
	# multicast packets to slot 1
	#
	[$self set switch_] set mask_ 1
	[$self set switch_] set shift_ [Simulator set McastShift_]

	#
	# create a classifier for multicast routing
	#
	$self set multiclassifier_ [new Classifier/Multicast/Replicator]
	[$self set multiclassifier_] set node_ $self

	[$self set switch_] install 0 $classifier_
	[$self set switch_] install 1 $multiclassifier_

	#
	# Create a prune agent.  Each multicast routing node
	# has a private prune agent that sends and receives
	# prune/graft messages and dispatches them to the
	# appropriate replicator object.
	#

        $self set mcastproto_ [new McastProtoArbiter ""]
	$mcastproto_ set Node $self
}

Node instproc add-neighbor p {
	$self instvar neighbor_
	lappend neighbor_ $p
}

Node instproc entry {} {
        if [Simulator set EnableMcast_] {
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

Node instproc alloc-port {} {
	$self instvar np_
	set p $np_
	incr np_
	return $p
}

#
# Attach an agent to a node.  Pick a port and
# bind the agent to the port number.
#
Node instproc attach agent {
	$self instvar agents_ classifier_ id_ dmux_
	#
	# assign port number (i.e., this agent receives
	# traffic addressed to this host and port)
	#
	lappend agents_ $agent
	set port [$self alloc-port]
	$agent set portID_ $port

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
	$agent set addr_ [expr $id_ << 8 | $port]

	#
	# If a port demuxer doesn't exist, create it.
	#
	if { $dmux_ == "" } {
		set dmux_ [new Classifier/Addr]
		$dmux_ set mask_ 0xff
		$dmux_ set shift_ 0
		#
		# point the node's routing entry to itself
		# at the port demuxer (if there is one)
		#
		$self add-route $id_ $dmux_
	}
	$dmux_ install $port $agent

	#
	# add trace into attached agent
	#
	$self instvar trace_
	if [info exists trace_] {
		$agent attach-trace $trace_
	}
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
# Helpers for interface stuff
#
Node instproc attachInterfaces ifs {
        $self instvar ifaces_
        set ifaces_ $ifs
        foreach ifc $ifaces_ {
                $ifc setNode $self
        }
}

Node instproc addInterface { iface } {
        $self instvar ifaces_
        lappend ifaces_ $iface
        $iface setNode $self 
}

Node instproc createInterface { num } {
        $self instvar ifaces_
        set newInterface [new NetInterface]
        if { $num < 0 } { return 0 }
        $newInterface set-label $num
        return 1
}
 
Node instproc getInterfaces {} {
        $self instvar ifaces_
        return $ifaces_
}

Node instproc getNode {} {  
        return $self
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
	if {[info exists chaddrs_($addr)]} {
		return $chaddrs_($addr)
	} else {
		set chost [new Agent/Chost $addr $cw $mtu $maxcw $wndopt]
		set chaddrs_($addr) $chost
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
    if {[llength $ifs] > 1 && ! $multiPath_} {
	puts stderr "$class::$proc cannot install multiple routes"
	set ifs [llength $ifs 0]
	set routes_($id) 0
    }
    if { $routes_($id) <= 0 && [llength $ifs] == 1 &&	\
	    ![info exists mpathClsfr_($id)] } {
	# either we really have no route, or
	# only one route that must be replaced.
	$self add-route $id [$ifs head]
	incr routes_($id)
    } else {
	if ![info exists mpathClsfr_($id)] {
	    set mclass [new Classifier/MultiPath]
	    if {$routes_($id) > 0} {
		array set current [$classifier_ adjacents]
		foreach i [array names current] {
		    if {$current($i) == $id} {
			$mclass installNext $i
			break
		    }
		}
	    }
	    $classifier_ install $id $mclass
	    set mpathClsfr_($id) $mclass
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
	array set eqPeers [$mpathClsfr_($id) adjacents]
	foreach L $ifs {
	    set link [$L head]
	    if [info exists eqPeers($link)] {
		$mpathClsfr_($id) clear $eqPeers($link)
		unset eqPeers($link)
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


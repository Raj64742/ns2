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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-node.tcl,v 1.9 1997/05/13 22:27:58 polly Exp $
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

Node instproc add-neighbor p {
	$self instvar neighbor_
	lappend neighbor_ $p
}

Node instproc entry {} {
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

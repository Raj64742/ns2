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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-node.tcl,v 1.1.1.1 1996/12/19 03:22:47 mccanne Exp $
#

Class Node
Node set nn 0
Node proc getid {} {
	set id [Node set nn]
	Node set nn [expr $id + 1]
	return $id
}

Node instproc init args {
	eval $self next $args
	$self instvar np id classifier agents dmux neighbor
	set neighbor ""
	set agents ""
	set dmux ""
	set np 0
	set id [Node getid]
	set classifier [new classifier/addr]
	# set up classifer as a router (i.e., 24 bit of addr and 8 bit port)
	$classifier set mask 0xffffff
	$classifier set shift 8
}

Node instproc add-neighbor p {
	$self instvar neighbor
	lappend neighbor $p
}

Node instproc entry {} {
	$self instvar classifier
	return $classifier
}

Node instproc add-route { dst target } {
	$self instvar classifier
	$classifier install $dst $target
}

Node instproc id {} {
	$self instvar id
	return $id
}

Node instproc alloc-port {} {
	$self instvar np
	set p $np
	incr np
	return $p
}

#
# Attach an agent to a node.  Pick a port and
# bind the agent to the port number.
#
Node instproc attach agent {
	$self instvar agents classifier id dmux
	#
	# assign port number (i.e., this agent receives
	# traffic addressed to this host and port)
	#
	lappend agents $agent
	set port [$self alloc-port]
	$agent set portID $port

	#
	# Attach agents to this node (i.e., the classifier inside).
	# We call the entry method on ourself to find the front door
	# (e.g., for multicast the first object is not the unicast
	#  classifier)
	# Also, stash the node in the agent and set the
	# local addr of this agent.
	#
	$agent target [$self entry]
	$agent set node $self
	$agent set addr [expr $id << 8 | $port]

	#
	# If a port demuxer doesn't exist, create it.
	#
	if { $dmux == "" } {
		set dmux [new classifier/addr]
		$dmux set mask 0xff
		$dmux set shift 0
	}
	$dmux install $port $agent

	#XXX might as well reset agent here.
	#XXX should happen at run time
	$agent reset
}


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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-node.tcl,v 1.3 1997/01/27 01:16:26 mccanne Exp $
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
	}
	$dmux_ install $port $agent

	#XXX might as well reset agent here.
	#XXX should happen at run time
	$agent reset
}

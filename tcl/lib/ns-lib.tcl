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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-lib.tcl,v 1.3 1997/01/27 01:16:23 mccanne Exp $
#

#
# Create the core OTcl class called "Simulator".
# This is the principal interface to the simulation engine.
#
Class Simulator

source ns-node.tcl
source ns-link.tcl
source ns-mcast.tcl
source ns-source.tcl
source ns-default.tcl
source ns-compat.tcl
source ns-nam.tcl
source ../rtp/session-rtp.tcl

Simulator instproc init args {
	eval $self next $args
	$self instvar scheduler_ nullAgent_
	set scheduler_ [new Scheduler/List]
	set nullAgent_ [new Agent/Null]
}

#
# A simple method to wrap any object around
# a trace object that dumps to stdout
#
Simulator instproc dumper obj {
	set t [$self alloc-trace hop stdout]
	$t target $obj
	return $t
}

Simulator instproc node {} {
	$self instvar Node_
	set node [new Node]
	set Node_([$node id]) $node
	return $node
}

Simulator instproc now {} {
	$self instvar scheduler_
	return [$scheduler_ now]
}

Simulator instproc at args {
	$self instvar scheduler_
	return [eval $scheduler_ at $args]
}

Simulator instproc run { } {
	$self compute-routes
	$self instvar scheduler_
	return [$scheduler_ run]
}

Simulator instproc simplex-link { n1 n2 bw delay type } {
	$self instvar link_ queueMap_
	set sid [$n1 id]
	set did [$n2 id]
	if [info exists queueMap_($type)] {
		set type $queueMap_($type)
	}
	set q [new Queue/$type]
	set link_($sid:$did) [new SimpleLink $n1 $n2 $bw $delay $q]
	$n1 add-neighbor $n2
}

Simulator instproc duplex-link { n1 n2 bw delay type } {
	$self simplex-link $n1 $n2 $bw $delay $type
	$self simplex-link $n2 $n1 $bw $delay $type
	$self instvar traceAllFile_
	if [info exists traceAllFile_] {
		$self trace-queue $n1 $n2 $traceAllFile_
		$self trace-queue $n2 $n1 $traceAllFile_
	}
}

Simulator instproc flush-trace {} {
	$self instvar alltrace_
	foreach trace $alltrace_ {
		$trace flush
	}
}

Simulator instproc trace-all file {
	$self instvar traceAllFile_
	set traceAllFile_ $file
}

Simulator instproc create-trace { type file src dst } {
	$self instvar alltrace_
	set p [new Trace/$type]
	$p set src_ [$src id]
	$p set dst_ [$dst id]
	lappend alltrace_ $p
	$p attach $file
	return $p
}

Simulator instproc trace-queue { n1 n2 file } {
	$self instvar link_
	$link_([$n1 id]:[$n2 id]) trace $self $file
}

Simulator instproc attach-agent { node agent } {
	$node attach $agent
}

#XXX need to check that agents are attached to nodes already
Simulator instproc connect { src dst } {
	#
	# connect agents (low eight bits of addr are port,
	# high 24 bits are node number)
	#
	set srcNode [$src set node_]
	set dstNode [$dst set node_]
	$src set dst_ [expr [$dstNode id] << 8 | [$dst port]]
	$dst set dst_ [expr [$srcNode id] << 8 | [$src port]]
	return $src
}

Simulator instproc compute-routes {} {
	$self instvar Node_ routingTable_ link_
	#
	# Compute all the routes using the route-logic helper object.
	#
	set r [new RouteLogic]
	foreach ln [array names link_] {
		set L [split $ln :]
		set srcID [lindex $L 0]
		set dstID [lindex $L 1]
		$r insert $srcID $dstID
	}
	$r compute
	#$r dump $nn

	#
	# Set up each classifer (aka node) to act as a router.
	# Point each classifer table to the link object that
	# in turns points to the right node.
	#
	set i 0
	set n [Node set nn_]
	while { $i < $n } {
		set n1 $Node_($i)
		set j 0
		while { $j < $n } {
			if { $i != $j } {
				set nexthop [$r lookup $i $j]
				$n1 add-route $j [$link_($i:$nexthop) head]
			} 
			incr j
		}
		set dmux [$n1 set dmux_]
		if { $dmux != "" } {
			#
			# point the node's routing entry to itself
			# at the port demuxer (if there is one)
			#
			$n1 add-route $i $dmux
		}
		incr i
	}
#	delete $r
#XXX used by multicast router
	set routingTable_ $r
}

#
# Here are a bunch of helper methods.
#

#
# debugging method to dump table (see route.cc for C++ methods)
#
RouteLogic instproc dump nn {
	set i 0
	while { $i < $nn } {
		set j 0
		while { $j < $nn } {
			puts "$i -> $j via [$self lookup $i $j]"
			incr j
		}
		incr i
	}
}

Classifier instproc no-slot slot {
	#XXX should say something better for routing problem
	puts stderr "$self: no target for slot $slot"
	exit 1
}

Agent instproc port {} {
	$self instvar portID_
	return $portID_
}

Agent/LossMonitor instproc log-loss {} {
}

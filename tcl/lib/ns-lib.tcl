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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-lib.tcl,v 1.21 1997/04/03 22:08:58 padmanab Exp $
#

#
# Create the core OTcl class called "Simulator".
# This is the principal interface to the simulation engine.
#
Class Simulator

source ns-node.tcl
source ns-link.tcl
source ns-shlink.tcl
source ns-mcast.tcl
source ns-source.tcl
source ns-default.tcl
source ns-compat.tcl
source ns-nam.tcl
source ns-packet.tcl
source ns-trace.tcl
source ../rtp/session-rtp.tcl
source ../rtglib/dynamics.tcl
source ../rtglib/route-proto.tcl

Simulator instproc init args {
	eval $self next $args
	$self create_packetformat
	$self instvar scheduler_ nullAgent_
	set scheduler_ [new Scheduler/List]
	set nullAgent_ [new Agent/Null]
}

Simulator instproc use-scheduler type {
	$self instvar scheduler_
	delete $scheduler_
	set scheduler_ [new Scheduler/$type]
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

Simulator instproc cancel args {
	$self instvar scheduler_
	return [eval $scheduler_ cancel $args]
}

Simulator instproc run args {
	#$self compute-routes
	eval RouteLogic configure $args
	$self instvar scheduler_ Node_ link_
	#
	# Reset every node, which resets every agent
	#
	foreach nn [array names Node_] {
		$Node_($nn) reset
	}
        #
        # also reset every queue
        #
        foreach qn [array names link_] {
                set q [$link_($qn) queue]
                $q reset
        }
        return [$scheduler_ run]
}

Simulator instproc simplex-link { n1 n2 bw delay type } {
	$self instvar link_ queueMap_ nullAgent_
	set sid [$n1 id]
	set did [$n2 id]
	if [info exists queueMap_($type)] {
		set type $queueMap_($type)
	}
	set q [new Queue/$type]
	$q drop-target $nullAgent_

	set link_($sid:$did) [new SimpleLink $n1 $n2 $bw $delay $q]
	$n1 add-neighbor $n2

	#XXX yuck
	if { $type == "RED" } {
	 	set bw [[$link_($sid:$did) set link_] set bandwidth_]
		$q set ptc_ [expr $bw / (8. * [$q set mean_pktsize_])]
	}
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
	if [info exists alltrace_] {
		foreach trace $alltrace_ {
			$trace flush
		}
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

#
# arrange for queue length of link between nodes n1 and n2
# to be tracked and return object that can be queried
# to learn average q size etc.  XXX this API still rough
#
Simulator instproc monitor-queue { n1 n2 } {
	$self instvar link_
	return [$link_([$n1 id]:[$n2 id]) init-monitor $self]
}

Simulator instproc queue-limit { n1 n2 limit } {
	$self instvar link_
	[$link_([$n1 id]:[$n2 id]) queue] set limit_ $limit
}

Simulator instproc drop-trace { n1 n2 trace } {
	$self instvar link_
	[$link_([$n1 id]:[$n2 id]) queue] drop-target $trace
}

Simulator instproc attach-agent { node agent } {
	$node attach $agent
}

Simulator instproc detach-agent { node agent } {
	$self instvar nullAgent_
	$node detach $agent $nullAgent_
}

#
#   Helper proc for setting delay on an existing link
#
Simulator instproc delay { n1 n2 delay } {
    $self instvar link_
    set sid [$n1 id]
    set did [$n2 id]
    if [info exists link_($sid:$did)] {
        set d [$link_($sid:$did) link]
        $d set delay_ $delay
    }
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
        set r [$self get-routelogic]
        #set r [new RouteLogic]		;# must not create multiple instances
	foreach ln [array names link_] {
		set L [split $ln :]
		set srcID [lindex $L 0]
		set dstID [lindex $L 1]
	        if { [$link_($ln) up?] == "up" } {
			$r insert $srcID $dstID
		} else {
			$r reset $srcID $dstID
		}
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
			        # shortened nexthop to nh, to fit add-route in
			        # a single line
				set nh [$r lookup $i $j]
			        if { $nh >= 0 } {
					$n1 add-route $j [$link_($i:$nh) head]
				}
			} 
			incr j
		}
		incr i
	}
#	delete $r
#XXX used by multicast router
	#set routingTable_ $r		;# already set by get-routelogic
}

#
# Here are a bunch of helper methods.
#

Simulator proc instance {} {
    set ns [Simulator info instances]
    if { $ns != "" } {
	return $ns
    }
    foreach sim [Simulator info subclass] {
	set ns [$sim info instances]
	if { $ns != "" } {
	    return $ns
	}
    }
    abort "Cannot find instance of simulator"
}
		
Simulator instproc get-node-by-id id {
    $self instvar Node_
    return $Node_($id)
}

Simulator instproc all-nodes-list {} {
    $self instvar Node_
    array names Node_
}

Simulator instproc link { n1 n2 } {
    $self instvar link_
    return $link_([$n1 id]:[$n2 id])
}

# Creates connection. First creates a source agent of type s_type and binds
# it to source.  Next creates a destination agent of type d_type and binds
# it to dest.  Finally creates bindings for the source and destination agents,
# connects them, and  returns the source agent.
Simulator instproc create-connection {s_type source d_type dest pktClass} {
	set s_agent [new Agent/$s_type]
	set d_agent [new Agent/$d_type]
	$s_agent set fid_ $pktClass
	$d_agent set fid_ $pktClass
	$self attach-agent $source $s_agent
	$self attach-agent $dest $d_agent
	$self connect $s_agent $d_agent
	
	return $s_agent
}

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

#
# Lower 8 bits of dst_ are portID_.  this proc supports setting the interval
# for delayed acks
#       
Agent instproc dst-port {} {
	$self instvar dst_
	return [expr $dst_%256]
}

#
# Add source of type s_type to agent and return the source
#
Agent instproc attach-source {s_type} {
	set source [new Source/$s_type]
	$source set agent_ $self
	$self set type_ $s_type
	return $source
}

Agent/LossMonitor instproc log-loss {} {
}

Agent/TCPSimple instproc opencwnd {} {
	$self instvar cwnd_
	set cwnd_ [expr $cwnd_ + 1.0 / $cwnd_]
}

Agent/TCPSimple instproc closecwnd {} {
	$self instvar cwnd_
	set cwnd_ [expr 0.5 * $cwnd_]
}

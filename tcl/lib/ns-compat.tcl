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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-compat.tcl,v 1.3 1997/01/24 18:44:23 mccanne Exp $
#

Class OldSim -superclass Simulator

#
# If the "ns" command is called, set up the simulator
# class to assume backward compat.  This creates an instance
# of a backward-compat simulator API with the name "ns"
# (which in turn overrides this proc)
#
proc ns args {
	OldSim ns
	eval ns $args
}

OldSim instproc init args {
	eval $self next $args
	puts stderr "warning: using backward compatibility mode"
	$self instvar classMap
	#
	# Catch queue-limit variable which is now "$q limit"
	#
	queue/drop-tail instproc set args {
		if { [llength $args] == 2 &&
			[lindex $args 0] == "queue-limit" } {
			# this will recursively call ourself
			$self set limit [lindex $args 1]
			return
		}
		eval $self next $args
	}
	#
	# Support for things like "set ftp [$tcp source ftp]"
	#
	agent/tcp instproc source type {
		set src [new source/$type]
		$src set agent $self
		return $src
	}

	Class traceHelper
	traceHelper instproc attach f {
		$self instvar file
		set file $f
	}
	Class linkHelper
	linkHelper instproc init args {
		$self next
		$self instvar node1 node2 link
		set node1 [lindex $args 0]
		set node2 [lindex $args 1]
		set link [$node1 id]:[$node2 id]	    
	}
	linkHelper instproc trace traceObj {
		$self instvar node1 node2
		ns trace-queue $node1 $node2 [$traceObj set file]
	}

	#
	# backward compat for "[ns link $n1 $n2] set linkVar $value"
	# where we only handle queue-limit, bandwidth and delay
	# (in the new simulator queue-limit is limit)
	#
	linkHelper instproc set { var val } {
		$self instvar link
		if { $var == "queue-limit" } {
			set q [[ns set link($link)] queue]
			$q set limit $val
		} elseif { $var == "bandwidth" || $var == "delay" } {
			set d [[ns set link($link)] queue]
			$d set $var $val
		}
	}

	set classMap(tcp) agent/tcp
	set classMap(tcp-reno) agent/tcp/reno
	set classMap(cbr) agent/cbr
	set classMap(tcp-sink) agent/tcp-sink
	set classMap(tcp-sink-da) agent/tcp-sink-da
	set classMap(tcp-sack1) agent/tcp/sack1
	set classMap(sack1-tcp-sink) agent/sack1-tcp-sink
}

OldSim instproc duplex-link-compat { n1 n2 bw delay type } {
	ns simplex-link $n1 $n2 $bw $delay $type
	ns simplex-link $n2 $n1 $bw $delay $type
}

OldSim instproc get-queues { n1 n2 } {
	$self instvar link
	set n1 [$n1 id]
	set n2 [$n2 id]
	return "[$link($n1:$n2) queue] [$link($n2:$n1) queue]"
}

OldSim instproc create-agent { node type pktClass } {

	$self instvar Agents PortID classMap
	if ![info exists classMap($type)] {
		puts stderr \
		  "backward compat bug: need to update classMap for $type"
		exit 1
	}
	set agent [new $classMap($type)]
	$agent set cls $pktClass
	$self attach-agent $node $agent

	$agent proc get var {
		return [$self set $var]
	}

	return $agent
}

OldSim instproc create-connection \
	{ srcType srcNode sinkType sinkNode pktClass } {

	$self instvar PortID NodeID
	set src [$self create-agent $srcNode $srcType $pktClass]
	set sink [$self create-agent $sinkNode $sinkType $pktClass]
	$self connect $src $sink

	return $src
}

#
# return helper object for backward compat of "ns link" command
#
OldSim instproc link { n1 n2 } {
	$self instvar LH
	if ![info exists LH($n1:$n2)] {
		set LH($n1:$n2) 1
		linkHelper LH:$n1:$n2 $n1 $n2
	}
	return LH:$n1:$n2
}

OldSim instproc trace {} {
	return [new traceHelper]
}

proc ns_simplex { n1 n2 bw delay type } {
	puts stderr "ns_simplex: no backward compat"
	exit 1
}

proc ns_duplex { n1 n2 bw delay type } {
	ns duplex-link-compat $n1 $n2 $bw $delay $type
	return [ns get-queues $n1 $n2]
}

#
# Create a source/sink connection pair and return the source agent.
# 
proc ns_create_connection { srcType srcNode sinkType sinkNode pktClass } {
	ns create-connection $srcType $srcNode $sinkType \
		$sinkNode $pktClass
}

#
# backward compat for agent methods that were replaced
# by OTcl instance variables
#
agent instproc connect d {
	$self set dst $d
}

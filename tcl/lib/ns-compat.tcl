#
# Copyright (c) 1996-1997 Regents of the University of California.
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-compat.tcl,v 1.18 1997/03/27 22:30:20 kfall Exp $
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
	$self instvar classMap_
	#
	# Catch queue-limit variable which is now "$q limit"
	#
	Queue/DropTail instproc set args {
		if { [llength $args] == 2 &&
			[lindex $args 0] == "queue-limit" } {
			# this will recursively call ourself
			$self set limit_ [lindex $args 1]
			return
		}
		eval $self next $args
	}
	#
	# Catch set maxpkts for FTP sources, which is now "produce maxpkts"
	#
	Source/FTP instproc set args {
		if { [llength $args] == 2 &&
			[lindex $args 0] == "maxpkts" } {
			$self produce [lindex $args 1]
			return
		}
		eval $self next $args
	}
	#
	# Support for things like "set ftp [$tcp source ftp]"
	#
	Agent/TCP instproc source type {
		if { $type == "ftp" } {
			set type FTP
		}
		set src [new Source/$type]
		$src set agent_ $self
		return $src
	}
	#
	# Lower 8 bits of dst_ are portID_.  this proc supports backward compat.
	# for setting the interval for delayed acks
	#       
	Agent instproc dst-port {} {
		$self instvar dst_
		return [expr $dst_%256]
	}   
	#
	# support for new variable names
	# it'd be nice to set up mappings on a per-class
	# basis, but this is too painful.  Just do the
	# mapping across all objects and hope this
	# doesn't cause any collisions...
	#
	TclObject instproc set args {
		TclObject instvar varMap_
		set var [lindex $args 0] 
		if [info exists varMap_($var)] {
			set var $varMap_($var)
			set args "$var [lrange $args 1 end]"
		}
		return [eval $self next $args]
	}
	# Agent
	TclObject set varMap_(addr) addr_
	TclObject set varMap_(dst) dst_
## now gone
###TclObject set varMap_(seqno) seqno_
###TclObject set varMap_(cls) class_
## class -> flow id
	TclObject set varMap_(cls) fid_

	# Trace
	TclObject set varMap_(src) src_

	# TCP
	TclObject set varMap_(window) window_
	TclObject set varMap_(window-init) windowInit_
	TclObject set varMap_(window-option) windowOption_
	TclObject set varMap_(window-constant) windowConstant_
	TclObject set varMap_(window-thresh) windowThresh_
	TclObject set varMap_(overhead) overhead_
	TclObject set varMap_(tcp-tick) tcpTick_
	TclObject set varMap_(ecn) ecn_
	TclObject set varMap_(packet-size) packetSize_
	TclObject set varMap_(bug-fix) bugFix_
	TclObject set varMap_(maxburst) maxburst_
	TclObject set varMap_(maxcwnd) maxcwnd_
	TclObject set varMap_(dupacks) dupacks_
	TclObject set varMap_(seqno) seqno_
	TclObject set varMap_(ack) ack_
	TclObject set varMap_(cwnd) cwnd_
	TclObject set varMap_(awnd) awnd_
	TclObject set varMap_(ssthresh) ssthresh_
	TclObject set varMap_(rtt) rtt_
	TclObject set varMap_(srtt) srtt_
	TclObject set varMap_(rttvar) rttvar_
	TclObject set varMap_(backoff) backoff_

	# FTP
	TclObject set varMap_(maxpkts) maxpkts_

	# Queue
	TclObject set varMap_(limit) limit_

	# Queue/SFQ
	TclObject set varMap_(limit) maxqueue_
	TclObject set varMap_(buckets) buckets_

	# Queue/RED
	TclObject set varMap_(bytes) bytes_
	TclObject set varMap_(thresh) thresh_
	TclObject set varMap_(maxthresh) maxthresh_
	TclObject set varMap_(mean_pktsize) meanPacketSize_
	TclObject set varMap_(q_weight) queueWeight_
	TclObject set varMap_(wait) wait_
	TclObject set varMap_(linterm) linterm_
	TclObject set varMap_(setbit) setbit_
	TclObject set varMap_(drop-tail) dropTail_
	TclObject set varMap_(doubleq) doubleq_
	TclObject set varMap_(dqthresh) dqthresh_
	TclObject set varMap_(subclasses) subclasses_

	# Agent/TCPSinnk, Agent/CBR
	TclObject set varMap_(packet-size) packetSize_
	TclObject set varMap_(interval) interval_

	# Agent/CBR
	TclObject set varMap_(random) random_

	Class traceHelper
	traceHelper instproc attach f {
		$self instvar file_
		set file_ $f
	}
	Class linkHelper
	linkHelper instproc init args {
		$self next
		$self instvar node1_ node2_ link_
		set node1_ [lindex $args 0]
		set node2_ [lindex $args 1]
		set link_ [$node1_ id]:[$node2_ id]	    
	}
	linkHelper instproc trace traceObj {
		$self instvar node1_ node2_
		ns trace-queue $node1_ $node2_ [$traceObj set file_]
	}

	#
	# backward compat for "[ns link $n1 $n2] set linkVar $value"
	# where we only handle queue-limit, bandwidth and delay
	# (in the new simulator queue-limit is limit)
	#
	linkHelper instproc set { var val } {
		$self instvar link_
		if { $var == "queue-limit" } {
			set q [[ns set link_($link_)] queue]
			$q set limit_ $val
		} elseif { $var == "bandwidth"  || $var == "bandwidth_" } {
			set d [[ns set link_($link_)] link]
			$d set bandwidth_ $val
		} elseif { $var == "delay"  || $var == "delay_" } {
			set d [[ns set link_($link_)] link]
			$d set delay_ $val
		}
	}

	set classMap_(tcp) Agent/TCP
	set classMap_(tcp-reno) Agent/TCP/Reno
	set classMap_(cbr) Agent/CBR
	set classMap_(tcp-sink) Agent/TCPSink
	set classMap_(tcp-sack1) Agent/TCP/Sack1
	set classMap_(sack1-tcp-sink) Agent/TCPSink/Sack1
	set classMap_(tcp-sink-da) Agent/TCPSink/DelAck
	set classMap_(loss-monitor) Agent/LossMonitor

	$self instvar queueMap_
	set queueMap_(drop-tail) DropTail
	set queueMap_(sfq) SFQ
	set queueMap_(red) RED
}

OldSim instproc duplex-link-compat { n1 n2 bw delay type } {
	ns simplex-link $n1 $n2 $bw $delay $type
	ns simplex-link $n2 $n1 $bw $delay $type
}

OldSim instproc get-queues { n1 n2 } {
	$self instvar link_
	set n1 [$n1 id]
	set n2 [$n2 id]
	return "[$link_($n1:$n2) queue] [$link_($n2:$n1) queue]"
}

OldSim instproc create-agent { node type pktClass } {

	$self instvar classMap_
	if ![info exists classMap_($type)] {
		puts stderr \
		  "backward compat bug: need to update classMap for $type"
		exit 1
	}
	set agent [new $classMap_($type)]
	# new mapping old class -> flowid
	$agent set fid_ $pktClass
	$self attach-agent $node $agent

	$agent proc get var {
		return [$self set $var]
	}

	return $agent
}

OldSim instproc create-connection \
	{ srcType srcNode sinkType sinkNode pktClass } {

	set src [$self create-agent $srcNode $srcType $pktClass]
	set sink [$self create-agent $sinkNode $sinkType $pktClass]
	$self connect $src $sink

	return $src
}

#
# return helper object for backward compat of "ns link" command
#
OldSim instproc link { n1 n2 } {
	$self instvar LH_
	if ![info exists LH_($n1:$n2)] {
		set LH_($n1:$n2) 1
		linkHelper LH_:$n1:$n2 $n1 $n2
	}
	return LH_:$n1:$n2
}

OldSim instproc trace {} {
	return [new traceHelper]
}

OldSim instproc random { seed } {
	return [ns-random $seed]
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
# Create a source/sink CBR pair and return the source agent.
# 
proc ns_create_cbr { srcNode sinkNode pktSize interval fid } {
	set s [ns create-connection cbr $srcNode loss-monitor \
		$sinkNode $fid]
	$s set interval_ $interval
	$s set packetSize_ $pktSize
	return $s
}

#
# backward compat for agent methods that were replaced
# by OTcl instance variables
#
Agent instproc connect d {
	$self set dst_ $d
}

# XXX changed call from "handle" to "recv"
Agent/Message instproc recv msg {
	$self handle $msg
}

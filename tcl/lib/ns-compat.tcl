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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-compat.tcl,v 1.21 1997/04/25 02:15:55 kfall Exp $
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
	# in CBQ, setting the algorithm_ variable becomes invoking
	# the algorithm method
	#
	# also, there really isn't a limit_ for CBQ, as each queue
	# has its own.
	#
	Queue/CBQ instproc set args {
		$self instvar compat_qlim_
		if { [lindex $args 0] == "queue-limit" || \
				[lindex $args 0] == "limit_" } { 
			if { [llength $args] == 2 } {
				set val [lindex $args 1]
				set compat_qlim_ $val
				return $val
			}
			return $compat_qlim_
		} elseif { [lindex $args 0] == "algorithm_" } {
			$self algorithm [lindex $args 1]
			# note: no return here
		}
		eval $self next $args
	}
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
        Queue/RED instproc set args {
                if { [llength $args] == 2 &&
                        [lindex $args 0] == "queue-limit" } {
                        # this will recursively call ourself
                        $self set limit_ [lindex $args 1]
                        return
                }
                eval $self next $args
        }

	#
	# Catch set maxpkts for FTP sources, (needed because Source objects are
	# not derived from TclObject, and hence can't use varMap method below)
	#
	Source/FTP instproc set args {
		if { [llength $args] == 2 &&
			[lindex $args 0] == "maxpkts" } {
			$self set maxpkts_ [lindex $args 1]
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

	#
	# linkHelper
	# backward compat for "[ns link $n1 $n2] set linkVar $value"
	#
	# unfortunately, 'linkVar' in ns-1 can be associated
	# with a link (delay, bandwidth, generic queue requests) or
	# can be specific to a particular queue (e.g. RED) which
	# has a bunch of variables (see above).
	#
	Class linkHelper
	linkHelper instproc init args {
		$self instvar node1_ node2_ linkref_ queue_
		set node1_ [lindex $args 0]
		set node2_ [lindex $args 1]
		set lid [$node1_ id]:[$node2_ id]	    
		set linkref_ [ns set link_($lid)]
		set queue_ [$linkref_ queue]
		# these will be used in support of link stats
		set sqi [new SnoopQueue/In]
		set sqo [new SnoopQueue/Out]
		set sqd [new SnoopQueue/Drop]
		set dsamples [new Samples]
		set qmon [new QueueMonitor/Compat]
		$qmon set-delay-samples $dsamples
		$linkref_ attach-monitors $sqi $sqo $sqd $qmon
	}
	linkHelper instproc trace traceObj {
		$self instvar node1_ node2_
		ns trace-queue $node1_ $node2_ [$traceObj set file_]
	}
	linkHelper instproc set { var val } {

		$self instvar linkref_ queue_
		set qvars [$queue_ info vars]
		set linkvars [$linkref_ info vars]
		set linkdelayvars [[$linkref_ link] info vars]
		#
		# adjust the string to have a trailing '_'
		# because all instvars are constructed that way
		#
		if { [string last _ $var] != ( [string length $var] - 1) } {
			set var ${var}_
		}
		if { $var == "queue-limit_" } {
			set var "limit_"
		}
		if { [lsearch $qvars $var] >= 0 } {
			# set a queue var
			$queue_ set $var $val
		} elseif { [lsearch $linkvars $var] >= 0 } {
			# set a link OTcl var
			$linkref_ set $var $val
		} elseif { [lsearch $linkdelayvars $var] >= 0 } {
			# set a linkdelay object var
			[$linkref_ link] set $var $val
		} else {
			puts stderr "linkHelper warning: couldn't set unknown variable $var"
		}
	}
	#
	# gross, but works:
	#
	# In ns-1 queues were a sublass of link, and this compat
	# code carries around a 'linkHelper' as the returned object
	# when you do a [ns link $r1 $r2] or a [ns link $r1 $r2 $qtype]
	# command.  So, operations on this object could have been
	# either link ops or queue ops in ns-1.  It is possible to see
	# whether an Otcl class or object supports certain commands
	# but it isn't possible to look inside a C++ implemented object
	# (i.e. into it's cmd function) to see what it supports.  Instead,
	# arrange to catch the exception generated while trying into a
	# not-implemented method in a C++ object.
	#
	linkHelper instproc try { obj operation argv } {
		set op [eval list $obj $operation $argv]
		set ocl [$obj info class]
		set iprocs [$ocl info instcommands]
		set oprocs [$obj info commands]
		# if it's a OTcl-implemented method we see it in info
		# and thus don't need to catch it
		if { $operation != "cmd" } {
			if { [lsearch $iprocs $operation] >= 0 } {
				return [eval $op]
			}
			if { [lsearch $oprocs $operation] >= 0 } {
				return [eval $op]
			}
		}
		#catch the c++-implemented method in case it's not there
		#ret will contain error string or return string
		# value of catch operation will be 1 on error
		if [catch $op ret] {
			return -1
		}
		return $ret
	}
	# so, try to invoke the op on a queue and if that causes
	# an exception (a missing function hopefully) try it on
	# the link instead
	#
	# we need to override 'TclObject instproc unknown args'
	# (well, at least we did), because it was coded such that
	# if a command() function didn't exist, an exit 1 happened
	#
	linkHelper instproc unknown { m args } {
		# method could be in: queue, link, linkdelay
		# or any of its command procedures
		# note that if any of those have errors in them
		# we can get a general error by ending up at the end here
		$self instvar linkref_ queue_
		set oldbody [TclObject info instbody unknown]
		TclObject instproc unknown args {
			if { [lindex $args 0] == "cmd" } {
				puts stderr "Can't dispatch $args"
				exit 1
			}
			eval $self cmd $args
		}

		# try an OTcl queue then the underlying queue object
		set rval [$self try $queue_ $m $args]
		if { $rval != -1 } {
			TclObject instproc unknown args $oldbody
			return $rval
		}
		set rval [$self try $queue_ cmd [list $m $args]]
		if { $rval != -1 } {
			TclObject instproc unknown args $oldbody
			return $rval
		}
		set rval [$self try $linkref_ $m $args]
		if { $rval != -1 } {
			TclObject instproc unknown args $oldbody
			return $rval
		}
		set rval [$self try $linkref_ cmd [list $m $args]]
		if { $rval != -1 } {
			TclObject instproc unknown args $oldbody
			return $rval
		}
		set dlink [$linkref_ link]
		set rval [$self try $dlink $m $args]
		if { $rval != -1 } {
			TclObject instproc unknown args $oldbody
			return $rval
		}
		set rval [$self try $dlink cmd [list $m $args]]
		if { $rval != -1 } {
			TclObject instproc unknown args $oldbody
			return $rval
		}
		TclObject instproc unknown args $oldbody
		puts stderr "Unknown operation $m or subbordinate operation failed"
		exit 1
	}
	linkHelper instproc stat { classid item } {
		$self instvar linkref_
		set qmon [$linkref_ set qMonitor_]
		# note: in ns-1 the packets/bytes stats are counts
		# of the number of *departures* at a link/queue
		#
		if { $item == "packets" } {
			return [$qmon pkts $classid]
		} elseif { $item == "bytes" } {
			return [$qmon bytes $classid]
		} elseif { $item == "drops"} {
			return [$qmon drops $classid]
		} elseif { $item == "mean-qdelay" } {
			set dsamp [$qmon get-delay-samples]
			return [$dsamp mean]
		} else {
			puts stderr "linkHelper: unknown stat op $item"
			exit 1
		}
	}

	#
	# end linkHelper
	#

	set classMap_(tcp) Agent/TCP
	set classMap_(tcp-reno) Agent/TCP/Reno
	set classMap_(cbr) Agent/CBR
	set classMap_(tcp-sink) Agent/TCPSink
	set classMap_(tcp-sack1) Agent/TCP/Sack1
	set classMap_(sack1-tcp-sink) Agent/TCPSink/Sack1
	set classMap_(tcp-sink-da) Agent/TCPSink/DelAck
	set classMap_(sack1-tcp-sink-da) Agent/TCPSink/Sack1/DelAck
	set classMap_(loss-monitor) Agent/LossMonitor

	$self instvar queueMap_
	set queueMap_(drop-tail) DropTail
	set queueMap_(sfq) SFQ
	set queueMap_(red) RED
	set queueMap_(cbq) CBQ
	set queueMap_(wrr-cbq) CBQ/WRR
}

#
# links in ns-1 had support for statistics collection...
# $link stat packets/bytes/drops
#
OldSim instproc simplex-link-compat { n1 n2 bw delay qtype } {
	set linkhelp [$self link-threeargs $n1 $n2 $qtype]
	$linkhelp set bandwidth_ $bw
	$linkhelp set delay_ $delay
}

OldSim instproc duplex-link-compat { n1 n2 bw delay type } {
	ns simplex-link-compat $n1 $n2 $bw $delay $type
	ns simplex-link-compat $n2 $n1 $bw $delay $type
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
OldSim instproc link args {
	set nargs [llength $args]
	set arg0 [lindex $args 0]
	set arg1 [lindex $args 1]
	if { $nargs == 2 } {
		return [$self link-twoargs $arg0 $arg1]
	} elseif { $nargs == 3 } {
		return [$self link-threeargs $arg0 $arg1 [lindex $args 2]]
	}
}
OldSim instproc link-twoargs { n1 n2 } {
	$self instvar LH_
	if ![info exists LH_($n1:$n2)] {
		set LH_($n1:$n2) 1
		linkHelper LH_:$n1:$n2 $n1 $n2
	}
	return LH_:$n1:$n2
}

OldSim instproc link-threeargs { n1 n2 qtype } {
	# new link with 0 bandwidth and 0 delay
	$self simplex-link $n1 $n2 0 0 $qtype
        return [$self link-twoargs $n1 $n2]
}
OldSim instproc trace {} {
	return [new traceHelper]
}

OldSim instproc random { seed } {
	return [ns-random $seed]
}

proc ns_simplex { n1 n2 bw delay type } {
        # this was never used in ns-1
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
# compat code for CBQ
#
proc ns_create_class { parent borrow allot maxidle notused prio depth xdelay } {
	set cl [new CBQClass]
	#
	# major hack: if the prio is 8 (the highest in ns-1) it's
	# an internal node, hence no queue disc
	if { $prio < 8 } {
		set qtype [CBQClass set def_qtype_]
		set q [new Queue/$qtype]
		$cl install-queue $q
	}
	set depth [expr $depth + 1]
	$cl setparams $borrow $allot $maxidle $prio $depth $xdelay
	return $cl
}

proc ns_create_class1 { parent borrow allot maxidle notused prio depth xdelay Mb } {
	set cl [ns_create_class $parent $borrow $allot $maxidle $notused $prio $depth $xdelay]
	ns_class_maxIdle $cl $allot $maxidle $prio $Mb
	return $cl
}

proc ns_class_params { cl parent borrow allot maxidle notused prio depth xdelay Mb } {
	set depth [expr $depth + 1]
	$cl setparams $borrow $allot $maxidle $prio $depth $xdelay
	ns_class_maxIdle $cl $allot $maxidle $prio $Mb
	return $cl
}

#
# If $maxIdle is "auto", set maxIdle to Max[t(1/p-1)(1-g^n)/g^n, t(1-g)].
# For p = allotment, t = packet transmission time, g = weight for EWMA.
# The parameter t is calculated for a medium-sized 1000-byte packet.
#
proc ns_class_maxIdle { cl allot maxIdle priority Mbps } {
        if { $maxIdle == "auto" } {
                set g 0.9375
                set n [expr 8 * $priority]
                set gTOn [expr pow($g, $n)]
                set first [expr ((1/$allot) - 1) * (1-$gTOn) / $gTOn ]
                set second [expr (1 - $g)]
                set packetsize 1000
                set t [expr ($packetsize * 8)/($Mbps * 1000000) ]
                if { $first > $second } {
                        $cl set maxidle_ [expr $t * $first]
                } else {
                        $cl set maxidle_ [expr $t * $second]
                }
        } else {
                $cl set maxidle_ $maxIdle
        }
        return $cl
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

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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-link.tcl,v 1.30 1998/01/05 18:56:58 bajaj Exp $
#
Class Link
Link instproc init { src dst } {
	$self next

        #modified for interface code
	$self instvar trace_ fromNode_ toNode_ source_ dest_ color_ oldColor_
	set fromNode_ [$src getNode]
	set toNode_ [$dst getNode]
        set source_ $src
        set dest_ $dst
	set color_ "black"
	set oldColor_ "black"

	set trace_ ""
}

Link instproc head {} {
	$self instvar head_
	return $head_
}

Link instproc queue {} {
	$self instvar queue_
	return $queue_
}

Link instproc link {} {
	$self instvar link_
	return $link_
}

Link instproc src {} {
	$self instvar source_
	return $source_
}

Link instproc dst {} {
	$self instvar dest_
	return $dest_
}

Link instproc fromNode {} {
	$self instvar fromNode_
	return $fromNode_
}

Link instproc toNode {} {
	$self instvar toNode_
	return $toNode_
}

Link instproc cost c {
	$self instvar cost_
	set cost_ $c
}

Link instproc cost? {} {
	$self instvar cost_
	if ![info exists cost_] {
		set cost_ 1
	}
	set cost_
}

Link instproc up { } {
	$self instvar dynamics_ dynT_
	if ![info exists dynamics_] return
	$dynamics_ set status_ 1
	if [info exists dynT_] {
		foreach tr $dynT_ {
			$tr format link-up {$src_} {$dst_}
			set ns [Simulator instance]
			$self instvar fromNode_ toNode_
			$tr ntrace "l -t [$ns now] -s [$fromNode_ id] -d [$toNode_ id] -S UP"
			$tr ntrace "v -t [$ns now] link-up [$ns now] [$fromNode_ id] [$toNode_ id]"
		}
	}
}

Link instproc down { } {
	$self instvar dynamics_ dynT_
	if ![info exists dynamics_] {
		puts stderr "$class::$proc Link $self was not declared dynamic, and cannot be taken down.  ignored"
		return
	}
	$dynamics_ set status_ 0
	$self all-connectors reset
	if [info exists dynT_] {
		foreach tr $dynT_ {
			$tr format link-down {$src_} {$dst_}
			set ns [Simulator instance]
			$self instvar fromNode_ toNode_
			$tr ntrace "l -t [$ns now] -s [$fromNode_ id] -d [$toNode_ id] -S DOWN"
			$tr ntrace "v -t [$ns now] link-down [$ns now] [$fromNode_ id] [$toNode_ id]"
		}
	}
}

Link instproc up? {} {
	$self instvar dynamics_
	if [info exists dynamics_] {
		return [$dynamics_ status?]
	} else {
		return "up"
	}
}

Link instproc all-connectors op {
	foreach c [$self info vars] {
		$self instvar $c
		if ![info exists $c] continue
		if [array size $c] continue
		foreach var [$self set $c] {
			if [catch "$var info class"] {
				continue
			}
			if ![$var info class Node] { ;# $op on most everything
				catch "$var $op";# in case var isn't a connector
			}
		}
	}
}

Class SimpleLink -superclass Link

SimpleLink instproc init { src dst bw delay q {lltype "DelayLink"} } {
	$self next $src $dst
	$self instvar link_ queue_ head_ toNode_ ttl_
	$self instvar drophead_

        #added for interface code
        $self instvar ifacein_ ifaceout_
        set ifacein_ 0
        set ifaceout_ $dst

	set queue_ $q
	set link_ [new $lltype]
	$link_ set bandwidth_ $bw
	$link_ set delay_ $delay

	$queue_ target $link_
	$link_ target [$dst entry]

	#set head_ $queue_ -> replace by the following
	# xxx this is hacky
	if { [[$q info class] info heritage ErrModule] == "ErrorModule" } {
		set head_ [$q classifier]
	} elseif { [$src info class] == "DuplexNetInterface" } {
                set head_ [$src exitpoint]
                set ifacein_ $head_
                $head_ target $queue_
        } else {
                set head_ $queue_
        }

	set drophead_ [new Connector]
	$drophead_ target [[Simulator instance] set nullAgent_]
	$queue_ drop-target $drophead_

	# XXX
	# put the ttl checker after the delay
	# so we don't have to worry about accounting
	# for ttl-drops within the trace and/or monitor
	# fabric
	#
	set ttl_ [new TTLChecker]
	$ttl_ target [$link_ target]
	$self ttl-drop-trace
	$link_ target $ttl_
}

#
# should be called after SimpleLink::trace
#
SimpleLink instproc nam-trace { ns f } {
	$self instvar enqT_ deqT_ drpT_ rcvT_ dynT_

	if [info exists enqT_] {
		$enqT_ namattach $f
		if [info exists deqT_] {
			$deqT_ namattach $f
		}
		if [info exists drpT_] {
			$drpT_ namattach $f
		}
		if [info exists rcvT_] {
			$rcvT_ namattach $f
		}
		if [info exists dynT_] {
			foreach tr $dynT_ {
				$tr namattach $f
			}
		}
	} else {
		#XXX 
		# we use enqT_ as a flag of whether tracing has been
		# initialized
		$self trace $ns $f "nam"
	}
}

#
# Build trace objects for this link and
# update the object linkage
#
# create nam trace files if op == "nam"
#
SimpleLink instproc trace { ns f {op ""} } {

	$self instvar enqT_ deqT_ drpT_ queue_ link_ head_ fromNode_ toNode_
	$self instvar rcvT_ ttl_
	$self instvar drophead_		;# idea stolen from CBQ and Kevin

	set enqT_ [$ns create-trace Enque $f $fromNode_ $toNode_ $op]
	set deqT_ [$ns create-trace Deque $f $fromNode_ $toNode_ $op]
	set drpT_ [$ns create-trace Drop $f $fromNode_ $toNode_ $op]
	set rcvT_ [$ns create-trace Recv $f $fromNode_ $toNode_ $op]

	$self instvar drpT_ drophead_
	set nxt [$drophead_ target]
	$drophead_ target $drpT_
	$drpT_ target $nxt

	$queue_ drop-target $drophead_

#	$drpT_ target [$queue_ drop-target]
#	$queue_ drop-target $drpT_

	$deqT_ target [$queue_ target]
	$queue_ target $deqT_

	#$enqT_ target $head_
	#set head_ $enqT_       -> replaced by the following
        if { [$head_ info class] == "networkinterface" } {
	    $enqT_ target [$head_ target]
	    $head_ target $enqT_
	    # puts "head is i/f"
        } else {
	    $enqT_ target $head_
	    set head_ $enqT_
	    # puts "head is not i/f"
	}

	# put recv trace after ttl checking, so that only actually 
	# received packets are recorded
	$rcvT_ target [$ttl_ target]
	$ttl_ target $rcvT_

	$self instvar dynamics_
	if [info exists dynamics_] {
		$self trace-dynamics $ns $f $op
	}
}

SimpleLink instproc trace-dynamics { ns f {op ""}} {
	$self instvar dynT_ fromNode_ toNode_
	lappend dynT_ [$ns create-trace Generic $f $fromNode_ $toNode_ $op]
	$self transit-drop-trace
	$self linkfail-drop-trace
}

SimpleLink instproc ttl-drop-trace args {
	$self instvar ttl_
	if ![info exists ttl_] return
	if {[llength $args] != 0} {
		$ttl_ drop-target [lindex $args 0]
	} else {
		$self instvar drophead_
		$ttl_ drop-target $drophead_
	}
}

SimpleLink instproc transit-drop-trace args {
	$self instvar link_
	if {[llength $args] != 0} {
		$link_ drop-target [lindex $args 0]
	} else {
		$self instvar drophead_
		$link_ drop-target $drophead_
	}
}

SimpleLink instproc linkfail-drop-trace args {
	$self instvar dynamics_
	if ![info exists dynamics_] return
	if {[llength $args] != 0} {
		$dynamics_ drop-target [lindex $args 0]
	} else {
		$self instvar drophead_
		$dynamics_ drop-target $drophead_
	}
}

#
# Trace to a callback function rather than a file.
#
SimpleLink instproc trace-callback {ns cmd} {
	$self trace $ns {}
	foreach part {enqT_ deqT_ drpT_} {
		$self instvar $part
		set to [$self set $part]
		$to set callback_ 1
		$to proc handle a "$cmd \$a"
	}
}

#
# like init-monitor, but allows for specification of more of the items
# attach-monitors $insnoop $inqm $outsnoop $outqm $dropsnoop $dropqm
#
SimpleLink instproc attach-monitors { insnoop outsnoop dropsnoop qmon } {
	$self instvar drpT_ queue_ head_ snoopIn_ snoopOut_ snoopDrop_
	$self instvar qMonitor_ drophead_

	set snoopIn_ $insnoop
	set snoopOut_ $outsnoop
	set snoopDrop_ $dropsnoop

	$snoopIn_ target $head_
	set head_ $snoopIn_

	$snoopOut_ target [$queue_ target]
	$queue_ target $snoopOut_

	set nxt [$drophead_ target]
	$drophead_ target $snoopDrop_
	$snoopDrop_ target $nxt

#	if [info exists drpT_] {
#		$snoopDrop_ target [$drpT_ target]
#		$drpT_ target $snoopDrop_
#		$queue_ drop-target $drpT_
#	} else {
#		$snoopDrop_ target [[Simulator instance] set nullAgent_]
#		$queue_ drop-target $snoopDrop_
#	}

	$snoopIn_ set-monitor $qmon
	$snoopOut_ set-monitor $qmon
	$snoopDrop_ set-monitor $qmon
	set qMonitor_ $qmon
}

#
# Insert objects that allow us to monitor the queue size
# of this link.  Return the name of the object that
# can be queried to determine the average queue size.
#
SimpleLink instproc init-monitor { ns qtrace sampleInterval} {
	$self instvar qMonitor_ ns_ qtrace_ sampleInterval_

	set ns_ $ns
	set qtrace_ $qtrace
	set sampleInterval_ $sampleInterval
	set qMonitor_ [new QueueMonitor]

	$self attach-monitors [new SnoopQueue/In] \
		[new SnoopQueue/Out] [new SnoopQueue/Drop] $qMonitor_

	set bytesInt_ [new Integrator]
	$qMonitor_ set-bytes-integrator $bytesInt_
	set pktsInt_ [new Integrator]
	$qMonitor_ set-pkts-integrator $pktsInt_
	return $qMonitor_
}

SimpleLink instproc start-tracing { } {
	$self instvar qMonitor_ ns_ qtrace_ sampleInterval_
	$self instvar source_ dest_
	
	$qMonitor_ trace $qtrace_
	$qMonitor_ set-src-dst [$source_ id] [$dest_ id]
} 

SimpleLink instproc queue-sample-timeout { } {
	$self instvar qMonitor_ ns_ qtrace_ sampleInterval_
	$self instvar source_ dest_
	
	set qavg [$self sample-queue-size]
	puts $qtrace_ "[$ns_ now] [$source_ id] [$dest_ id] $qavg"
	$ns_ at [expr [$ns_ now] + $sampleInterval_] "$self queue-sample-timeout"
}

SimpleLink instproc sample-queue-size { } {
	$self instvar qMonitor_ ns_ qtrace_ sampleInterval_ lastSample_

	set now [$ns_ now]
	set qBytesMonitor_ [$qMonitor_ get-bytes-integrator]
	set qPktsMonitor_ [$qMonitor_ get-pkts-integrator]

	$qBytesMonitor_ newpoint $now [$qBytesMonitor_ set lasty_]
	set bsum [$qBytesMonitor_ set sum_]

	$qPktsMonitor_ newpoint $now [$qPktsMonitor_ set lasty_]
	set psum [$qPktsMonitor_ set sum_]

	if ![info exists lastSample_] {
		set lastSample_ 0
	}
	set dur [expr $now - $lastSample_]
	if { $dur != 0 } {
		set meanBytesQ [expr $bsum / $dur]
		set meanPktsQ [expr $psum / $dur]
	} else {
		set meanBytesQ 0
		set meanPktsQ 0
	}
	$qBytesMonitor_ set sum_ 0.0
	$qPktsMonitor_ set sum_ 0.0
	set lastSample_ $now

	return "$meanBytesQ $meanPktsQ"
}	


SimpleLink instproc dynamic {} {
	$self instvar dynamics_ head_

	if [info exists dynamics_] return
	
	set dynamics_ [new DynamicLink]
#	$dynamics_ target head_
#	set head_ $dynamics_
	$dynamics_ target [$head_ target]
	$head_ target $dynamics_

	
	$self transit-drop-trace
	$self all-connectors dynamic
}

#
# insert an "error module" after the queue
# point the em's drop-target to the drophead
#
SimpleLink instproc errormodule args {
	$self instvar errmodule_ queue_ drophead_ head_
	if { $args == "" } {
		return $errmodule_
	}

	set em [lindex $args 0]
	set errmodule_ $em

	#set h [$queue_ target]
	set h $head_
	#$queue_ target $em
	set head_ $em
	$em target $h

	$em drop-target $drophead_
}

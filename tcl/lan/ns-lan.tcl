#
# Copyright (c) 1997 Regents of the University of California.
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
# 	This product includes software developed by the Daedalus Research
#       Group at the University of California, Berkeley.
# 4. Neither the name of the University nor of the research group
#    may be used to endorse or promote products derived from this software 
#    without specific prior written permission.
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
# Contributed by the Daedalus Research Group, http://daedalus.cs.berkeley.edu
#

# Defaults for link-layer
LL set delay_ 0.5ms
LL/Snoop set bandwidth_ 1Mb
LL/Snoop set delay_ 3ms
LL/Snoop set snoopDisable_ 0
LL/Snoop set srtt_ 100ms
LL/Snoop set rttvar_ 100ms

ErrorModel set rate_ 0
ErrorModel set time_ 0
ErrorModel set errorLen_ 0

# TraceIp trace IP packet headers for LAN components
TraceIp set src_ -1
TraceIp set dst_ -1
TraceIp set callback_ 0
TraceIp set mask_ 0xffffffff
TraceIp set shift_ 8

TraceIp instproc init {type} {
	$self next $type
	$self instvar type_
	set type_ $type
}

Class TraceIp/Enque -superclass TraceIp
TraceIp/Enque instproc init {} {
	$self next "+"
}

Class TraceIp/Deque -superclass TraceIp
TraceIp/Deque instproc init {} {
	$self next "-"
}

Class TraceIp/Drop -superclass TraceIp
TraceIp/Drop instproc init {} {
	$self next "d"
}

Class TraceIp/Corrupt -superclass TraceIp
TraceIp/Corrupt instproc init {} {
	$self next "c"
}

Class TraceIp/Mac/Hop -superclass TraceIp/Mac
TraceIp/Mac/Hop instproc init {} {
	$self next "h"
}

# Trace/Recv trace the receive events of a packet
Class Trace/Recv -superclass Trace
Trace/Recv instproc init {} {
	$self next "r"
}


# Make a LAN, given a set of nodes, the LAN bw and delay, and the types
# of the various LAN entities (lltype, mactype, etc.)
# This belongs in ns-lib.tcl -- will move it later
Simulator instproc make-lan {nodelist bw delay \
		{lltype LL} {ifqtype Queue/DropTail} \
		{mactype Mac} {chantype Channel}} {
	$self instvar link_ nullAgent_ traceAllFile_

	set lan [new Link/LanLink $nodelist $bw $delay \
			$lltype $ifqtype $mactype $chantype]

	foreach src $nodelist {
		set sid [$src id]
		foreach dst $nodelist {
			if {$src == $dst} continue
			set did [$dst id]
			# this is a dummy queue for unicast routing to work
			set q($src:$dst) [new Queue/DropTail]
			$q($src:$dst) drop-target $nullAgent_
			set link_($sid:$did) [new Link/LanDuplex \
				$src $dst $bw $delay $q($src:$dst) $lltype]
		}
	}
	foreach src $nodelist {
		set sid [$src id]
		set mclass [new Classifier/Mac]
		[$lan getmac $src] target $mclass
		foreach dst $nodelist {
			if {$src == $dst} continue
			set did [$dst id]
			$link_($sid:$did) setuplinkage \
				$src $dst $link_($did:$sid) $lan
			set peerlabel [[$lan getmac $dst] set label_]
			$mclass install $peerlabel [$link_($sid:$did) link]
#			puts "SRC $src DST $dst peerlabel $peerlabel"
			$self trace-queue $src $dst $traceAllFile_
		}
	}

	return $lan
}

# A peer-to-peer duplex link on a LAN
Class Link/LanDuplex -superclass SimpleLink
Link/LanDuplex instproc init { src dst bw delay qtype lltype } {
	$self next $src $dst $bw $delay $qtype $lltype
}

# Setup linkage for the link-layer
Link/LanDuplex instproc setuplinkage {src dst dstlink lan} {
	$self instvar link_
	$link_ peerLL [$dstlink link]
	$link_ recvtarget [$src entry]
	$link_ sendtarget [$lan getifq $src]
	$link_ mac [$lan getmac $src]
}

Link/LanDuplex instproc trace { ns f } {
	$self instvar link_ fromNode_ toNode_

	set enqT_ [$ns create-trace Enque $f $fromNode_ $toNode_]
	$enqT_ target [$link_ sendtarget]
	$link_ sendtarget $enqT_
	set recvT_ [$ns create-trace Recv $f $toNode_ $fromNode_]
	$recvT_ target [$link_ recvtarget]
	$link_ recvtarget $recvT_
}


Class Link/LanLink
Link/LanLink instproc init {nodelist bw delay lltype ifqtype mactype chantype} {
	$self instvar nodelist_ channel_ mac_ ifq_ numifaces_
	set numifaces_ 0

	set nodelist_ $nodelist
	set channel_ [new $chantype]
	set cclass [new Classifier/Channel]
	$channel_ target $cclass

	foreach src $nodelist {
		set mac [set mac_($src) [new $mactype]]
		set ifq_($src) [new $ifqtype]
		$ifq_($src) target $mac
		$mac set bandwidth_ $bw
		$mac set label_ $numifaces_
		$mac channel $channel_
		$mac cclass $cclass
		$cclass install $numifaces_ $mac
		incr numifaces_
		# List of MACs
		$src addmac $mac
	}
}

Link/LanLink instproc getifq {src} {
	$self instvar ifq_
	return $ifq_($src)
}

Link/LanLink instproc getmac {src} {
	$self instvar mac_
	return $mac_($src)
}

Link/LanLink instproc channel {} {
	$self instvar channel_
	return $channel_
}

Link/LanLink instproc trace {ns f} {
	$self instvar nodelist_ channel_ mac_ ifq_ drpT_

	set drpT_($channel_) [new TraceIp/Corrupt]
	$channel_ drop-target $drpT_($channel_)
	$drpT_($channel_) attach $f
	foreach src $nodelist_ {
		# drpT shared by both IFQ and MAC
		set drpT [set drpT_($src) [new TraceIp/Drop]]
		$drpT set src_ [$src id]
		$drpT attach $f
		$mac_($src) drop-target $drpT
		$ifq_($src) drop-target $drpT

		set deqT_ [new TraceIp/Deque]
		$deqT_ attach $f
		$deqT_ target [$ifq_($src) target]
		$ifq_($src) target $deqT_
	}
}

Link/LanLink instproc init-monitor {ns qfile sampleInt src {dst ""}} {
	$self instvar ns_ qfile_ sampleInterval_
	$self instvar ifq_ qm_ si_ so_ sd_
	$ns instvar link_

	set ns_ $ns
	set qfile_($src) $qfile
	set sampleInterval_($src) $sampleInt

	set qm_($src) [new QueueMonitor]
	set si_($src) [new SnoopQueue/In]
	set so_($src) [new SnoopQueue/Out]
	set sd_($src) [new SnoopQueue/Drop]

	set qMonitor_ $qm_($src)
	set si $si_($src)
	$si set-monitor $qMonitor_
	$so_($src) set-monitor $qMonitor_
	$sd_($src) set-monitor $qMonitor_

	$si target $ifq_($src)
	foreach dst $nodelist_ {
		if {$src == $dst} continue
		[$link_([$src id]:[$dst id]) link] target $si
	}

	$so_($src) target [$ifq_($src) target]
	ifq_($src) target $so_($src)

	if [info exists drpT_] {
		$sd_($src) target [$drpT_($src) target]
		$drpT_($src) target $sd_($src)
	} else {
		$sd_($src) target [ns set nullAgent_]
		ifq_($src) drop-target $sd_($src)
	}

	set bytesInt_ [new BytesIntegrator]
	$qMonitor_ set-bytes-integrator $bytesInt_
	set pktsInt_ [new BytesIntegrator]
	$qMonitor_ set-pkts-integrator $pktsInt_
	return $qMonitor_
}

Link/LanLink instproc queue-sample-timeout {} {
	$self instvar ns_ qfile_ sampleInterval_
	$self instvar qm_

	set nodelist [array names qm_]
	set numnodes [llength $nodelist]
	for {set i 0} {$i < $numnodes} {incr i} {
		set src [lindex $nodelist $i]
		set qavg [$self sample-queue-size $src]
		puts $qfile_($src) "[$ns_ now] n[$src id]:n* $qavg"
	}
	$ns_ at [expr [$ns_ now] + $sampleInterval_($src)] \
			"$self queue-sample-timeout"
}

Link/LanLink instproc sample-queue-size {src} {
	$self instvar ns_ qfile_ sampleInterval_
	$self instvar qm_

	set now [$ns_ now]
	set qBytesMonitor_ [$qm_($src) get-bytes-integrator]
	set qPktsMonitor_ [$qm_($src) get-pkts-integrator]
	
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

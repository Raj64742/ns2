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
# @(#) $Header: /usr/src/mash/repository/vint/ns-2/tcl/lib/ns-shlink.tcl
#

LL/Base set bandwidth_ 1.5Mb
LL/Base set delay_ 1ms

Channel set delay_ 16us
Mac/Base set bandwidth_ 1.5Mb
Mac/Base set hlen_ 0

# WaveLAN settings
Mac/Csma set bandwidth_ 2Mb
Mac/Csma set hlen_ 20
Mac/Csma set ifs_ 16us
Mac/Csma set slotTime_ 16us
Mac/Csma set cwmin_ 16
Mac/Csma set cwmax_ 1024
Mac/Csma set rtxmax_ 16

# Ethernet settings
#Mac/Csma/Cd set bandwidth_ 10Mb
#Mac/Csma/Cd set ifs_ 52us
#Mac/Csma/Cd set slotTime_ 52us
#Mac/Csma/Cd set cwmin_ 1

TraceIp set src_ 0
TraceIp set dst_ 0
TraceIp set callback_ 0


Class Trace/Recv -superclass Trace
Trace/Recv instproc init {} {
	$self next "r"
}

TraceIp instproc init {type} {
	$self next $type
	$self instvar type_
	set type_ $type
}

Class TraceIp/Drop -superclass TraceIp
TraceIp/Drop instproc init {} {
	$self next "d"
}

Class TraceIp/Corrupt -superclass TraceIp
TraceIp/Corrupt instproc init {} {
	$self next "c"
}


Simulator instproc shared-duplex-link {nodelist bw delay {qtype DropTail} \
		{lltype LL/Base} {ifqtype Queue/DropTail} {mactype Mac/Base}} {
	$self instvar link_ queueMap_ nullAgent_ traceAllFile_
	$self instvar shlink_

	set shl [new Link/SharedDuplex $nodelist $bw $delay \
			$lltype $ifqtype $mactype]

	set numnodes_ [llength $nodelist]
	for {set i 0} {$i < $numnodes_} {incr i} {
		set src [lindex $nodelist $i]
		for {set j 0} {$j < $numnodes_} {incr j} {
			set dst [lindex $nodelist $j]
			if {$src == $dst} continue
			set q($src:$dst) [new Queue/$qtype]
			$q($src:$dst) drop-target $nullAgent_
			set ll [new Link/Duplex $src $dst $bw $delay \
					$q($src:$dst) $lltype]
			[$ll link] ifq [$shl ifq $src]
			set link_([$src id]:[$dst id]) $ll
		}
	}
	for {set i 0} {$i < $numnodes_} {incr i} {
		set src [lindex $nodelist $i]
		for {set j 0} {$j < $numnodes_} {incr j} {
			set dst [lindex $nodelist $j]
			if {$src == $dst} continue
			$link_([$src id]:[$dst id]) setuplinkage $src \
				$link_([$dst id]:[$src id])
			$self trace-queue $src $dst $traceAllFile_
		}
	}

	set shlink_($nodelist) $shl
	puts "$shl $bw $delay $lltype $ifqtype $mactype"
	return $shl
}


Class Link/Duplex -superclass SimpleLink

Link/Duplex instproc init { src dst bw delay qtype lltype } {
	$self next $src $dst $bw $delay $qtype $lltype
}

Link/Duplex instproc setuplinkage {src dstlink} {
	$self instvar link_
	$link_ recvtarget [$src entry]
	$link_ sendtarget [$dstlink link]
}

Link/Duplex instproc trace { ns f } {
	$self next $ns $f
	$self instvar link_ fromNode_ toNode_
	set recvT_ [$ns create-trace Recv $f $toNode_ $fromNode_]
	$recvT_ target [$link_ recvtarget]
	$link_ recvtarget $recvT_
}


Class Link/SharedDuplex
Link/SharedDuplex instproc init {nodelist bw delay lltype ifqtype mactype} {
	$self instvar numnodes_ channel_ mac_ ifq_ drop_

	set channel_ [new Channel]
	$channel_ drop-target [new TraceIp/Corrupt]
	set numnodes_ [llength $nodelist]
	for {set i 0} {$i < $numnodes_} {incr i} {
		set src [lindex $nodelist $i] 
		# drop_ share by both IFQ and MAC
		set drop_ [new TraceIp/Drop]
		$drop_ set src [$src id]

		set mac_($src) [new $mactype]
		$mac_($src) set bandwidth_ $bw
		$mac_($src) set delay_ $delay
		$mac_($src) channel $channel_
		$mac_($src) drop-target $drop_

		set ifq_($src) [new $ifqtype]
		$ifq_($src) target $mac_($src)
		$ifq_($src) drop-target $drop_
	}
}

Link/SharedDuplex instproc ifq {src} {
	$self instvar ifq_
	return $ifq_($src)
}

Link/SharedDuplex instproc channel {} {
	$self instvar channel_
	return $channel_
}

Link/SharedDuplex instproc init-monitor {ns qfile sampleInt src {dst ""}} {
	$self instvar ns_ qfile_ sampleInterval_
	$self instvar qMonitor_
	$self instvar ifq_

	set ns_ $ns
	set qfile_($src) $qfile
	set sampleInterval_($src) $sampleInt

	set qMonitor_($src) [new QueueMonitor]
	set bytesInt_ [new BytesIntegrator]
	set pktsInt_ [new BytesIntegrator]
	$qMonitor_($src) set-bytes-integrator $bytesInt_
	$qMonitor_($src) set-pkts-integrator $pktsInt_

	$ifq_($src) set-monitor $qMonitor_($src)
	return $qMonitor_($src)
}

Link/SharedDuplex instproc queue-sample-timeout {} {
	$self instvar ns_ qfile_ sampleInterval_
	$self instvar qMonitor_

	set nodelist [array names qMonitor_]
	set numnodes [llength $nodelist]
	for {set i 0} {$i < $numnodes} {incr i} {
		set src [lindex $nodelist $i]
		set qavg [$self sample-queue-size $src]
		puts $qfile_($src) "[$ns_ now] n[$src id]:n* $qavg"
	}
	$ns_ at [expr [$ns_ now] + $sampleInterval_($src)] \
			"$self queue-sample-timeout"
}

Link/SharedDuplex instproc sample-queue-size {src} {
	$self instvar ns_ qfile_ sampleInterval_
	$self instvar qMonitor_

	set now [$ns_ now]
	set qBytesMonitor_ [$qMonitor_($src) get-bytes-integrator]
	set qPktsMonitor_ [$qMonitor_($src) get-pkts-integrator]
	
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

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

LL/Base set bandwidth_ 10Mb
LL/Base set delay_ 1ms

Mac/Base set bandwidth_ 10Mb
Mac/Base set delay_ 1ms

Mac/Csma set bandwidth_ 10Mb
Mac/Csma set delay_ 1ms
Mac/Csma set ifs_ 16us
Mac/Csma set slotTime_ 16us
Mac/Csma set cwmin_ 1
Mac/Csma set cwmax_ 256
Mac/Csma set rtxmax_ 10

Channel set delay_ 16us


Class Trace/Recv -superclass Trace
Trace/Recv instproc init {} {
	$self next "r"
}


Simulator instproc shared-duplex-link { nodelist bw delay { qtype "DropTail" } { lltype "LL/Base" } { ifqtype "Queue/DropTail" } { mactype "Mac/Base" } } {
	$self instvar link_ queueMap_ nullAgent_ traceAllFile_

	puts "$bw $delay $lltype $mactype"

	if [info exists queueMap_($qtype)] {
		set qtype $queueMap_($qtype)
	}

	set numnodes [llength $nodelist]
	set channel_ [new Channel]
	$channel_ set delay_ $delay

	for {set i 0} {$i < $numnodes} {incr i} {
		set src [lindex $nodelist $i] 

		set mac_($src) [new $mactype]
		$mac_($src) set bandwidth_ $bw
		$mac_($src) set slotTime_ $delay
		$mac_($src) channel $channel_

		set ifq_($src) [new $ifqtype]
		$ifq_($src) target $mac_($src)
	}

	for {set i 0} {$i < $numnodes} {incr i} {
		set src [lindex $nodelist $i] 
		for {set j 0} {$j < $numnodes} {incr j} {
			set dst [lindex $nodelist $j]
			if { $src != $dst } {
				set q($src:$dst) [new Queue/$qtype]
				$q($src:$dst) drop-target $nullAgent_
				set link_([$src id]:[$dst id]) [new Link/SharedDuplex $src $dst $bw $delay $q($src:$dst) $lltype]
			}
		}
	}

	for {set i 0} {$i < $numnodes} {incr i} {
		set src [lindex $nodelist $i] 
		for {set j 0} {$j < $numnodes} {incr j} {
			set dst [lindex $nodelist $j]
			if { $src != $dst } {
				set sid [$src id]
				set did [$dst id]
				$link_($sid:$did) setuplinkage $src $ifq_($src) $link_($did:$sid)
				$self trace-queue $src $dst $traceAllFile_
			}
		}
	}
}

			
Class Link/SharedDuplex -superclass SimpleLink

Link/SharedDuplex instproc init { src dst bw delay qtype lltype } {
	$self next $src $dst $bw $delay $qtype $lltype
}

Link/SharedDuplex instproc setuplinkage { src ifq dstlink } {
	$self instvar link_
	$link_ ifq $ifq
	$link_ recvtarget [$src entry]
	$link_ sendtarget [$dstlink link]
}

Link/SharedDuplex instproc trace { ns f } {
	$self next $ns $f
	$self instvar link_ fromNode_ toNode_
	set recvT_ [$ns create-trace Recv $f $toNode_ $fromNode_]
	$recvT_ target [$link_ recvtarget]
	$link_ recvtarget $recvT_
}

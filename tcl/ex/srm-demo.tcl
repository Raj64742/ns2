#
# Copyright (c) 1994-1997 Regents of the University of California.
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
#	This product includes software developed by the Computer Systems
#	Engineering Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
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
#
# This file contains contrived scenarios and protocol agents
# to illustrate the basic srm suppression algorithms.
# It is not an srm implementation.
#
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/ex/srm-demo.tcl,v 1.9 1997/08/11 05:12:36 mccanne Exp $
#

Simulator set NumberInterfaces_ 1
set ns [new MultiSim]

# cause ACKs to get dropped
Queue set limit_ 6

foreach k "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14" {
	 set node($k) [$ns node]
}

set f [open out.tr w]
$ns trace-all $f

proc makelinks { bw delay pairs } {
	global ns node
	foreach p $pairs {
		set src $node([lindex $p 0])
		set dst $node([lindex $p 1])
		$ns duplex-link $src $dst $bw $delay DropTail
	}
}

makelinks 1.5Mb 10ms {
	{ 9 0 }
	{ 9 1 }
	{ 9 2 }
	{ 10 3 }
	{ 10 4 }
	{ 10 5 }
	{ 11 6 }
	{ 11 7 }
	{ 11 8 }
}

makelinks 1.5Mb 40ms {
	{ 12 9 }
	{ 12 10 }
	{ 12 11 }
}

makelinks 1.5Mb 10ms {
	{ 13 12 } 
}

makelinks 1.5Mb 50ms {
	{ 14 12 }
}

set mproto DM
set mrthandle [$ns mrtproto $mproto {}]

Class Agent/Message/MC_Acker -superclass Agent/Message
Agent/Message/MC_Acker set packetSize_ 800

Agent/Message/MC_Acker instproc recv msg {
	set type [lindex $msg 0]
	set from [lindex $msg 1]
	set seqno [lindex $msg 2]
	$self set dst_ $from
	$self send "ack $from $seqno"
}

Class Agent/Message/MC_Sender -superclass Agent/Message

Agent/Message/MC_Sender instproc recv msg {
	$self instvar addr_ sent_
	set type [lindex $msg 0]
	if { $type == "nack" } {
		set seqno [lindex $msg 2]
		if ![info exists sent_($seqno)] {
			$self send "data $addr_ $seqno"
			set sent_($seqno) 1
		}
	}
}

Agent/Message/MC_Sender instproc init {} {
	$self next
	$self set seqno_ 1
}

Agent/Message/MC_Sender instproc send-pkt {} {
	$self instvar seqno_ addr_
	$self send "data $addr_ $seqno_"
	incr seqno_
}

set sndr [new Agent/Message/MC_Sender]
$sndr set packetSize_ 1400
$sndr set dst_ 0x8002
$sndr set class_ 1

$ns at 1.0 {
	global rcvr node
	foreach k "0 1 2 3 4 5 6 7 8" {
		set rcvr($k) [new Agent/Message/MC_Acker]
		$ns attach-agent $node($k) $rcvr($k)
		$rcvr($k) set class_ 2
		$node($k) join-group $rcvr($k) 0x8002
	}
	$node(14) join-group $sndr 0x8002
}

Class Agent/Message/MC_Nacker -superclass Agent/Message
Agent/Message/MC_Nacker set packetSize_ 800
Agent/Message/MC_Nacker instproc recv msg {
	set type [lindex $msg 0]
	set from [lindex $msg 1]
	set seqno [lindex $msg 2]
	$self instvar dst_ ack_
	if [info exists ack_] {
		set expected [expr $ack_ + 1]
		if { $seqno > $expected } {
			set dst_ $from
			$self send "nack $from $seqno"
		}
	}
	set ack_ $seqno
}

Class Agent/Message/MC_SRM -superclass Agent/Message
Agent/Message/MC_SRM set packetSize_ 800
Agent/Message/MC_SRM instproc recv msg {
	$self instvar dst_ ack_ nacked_ random_
	set type [lindex $msg 0]
	set from [lindex $msg 1]
	set seqno [lindex $msg 2]
	if { $type == "nack" } {
		set nacked_($seqno) 1
		return
	}
	if [info exists ack_] {
		set expected [expr $ack_ + 1]
		if { $seqno > $expected } {
			set dst_ 0x8002
			if [info exists random_] {
				global ns
				set r [expr ([ns-random] / double(0x7fffffff) + 0.1) * $random_]
				set r [expr [$ns now] + $r]
				$ns at $r "$self send-nack $from $seqno"
			} else {
				$self send "nack $from $seqno"
			}
		}
	}
	set ack_ $seqno
}

Agent/Message/MC_SRM instproc send-nack { from seqno } {
	$self instvar nacked_ dst_
	if ![info exists nacked_($seqno)] {
		set dst_ 0x8002
		$self send "nack $from $seqno"
	}
}

$ns at 1.5 {
	global rcvr node
	foreach k "0 1 2 3 4 5 6 7 8" {
		$node($k) leave-group $rcvr($k) 0x8002
		$ns detach-agent $node($k) $rcvr($k)
		delete $rcvr($k)
		set rcvr($k) [new Agent/Message/MC_Nacker]
		$ns attach-agent $node($k) $rcvr($k)
		$rcvr($k) set class_ 3
		$node($k) join-group $rcvr($k) 0x8002
	}
}

$ns at 3.0 {
	global rcvr node
	foreach k "0 1 2 3 4 5 6 7 8" {
		$node($k) leave-group $rcvr($k) 0x8002
		$ns detach-agent $node($k) $rcvr($k)
		delete $rcvr($k)
		set rcvr($k) [new Agent/Message/MC_SRM]
		$ns attach-agent $node($k) $rcvr($k)
		$rcvr($k) set class_ 3
		$node($k) join-group $rcvr($k) 0x8002
	}
}

$ns at 3.6 {
	global rcvr node
	foreach k "0 1 2 3 4 5 6 7 8" {
		$rcvr($k) set random_ 2
	}
}

$ns attach-agent $node(14) $sndr

foreach t {
	1.05
	1.08
	1.11
	1.14

	1.55
	1.58
	1.61
	1.64 

	1.85
	1.88
	1.91
	1.94

	2.35
	2.38
	2.41
	2.44

	3.05
	3.08
	3.11
	3.14

	3.65
	3.68
	3.71
	3.74

} { $ns at $t "$sndr send-pkt" }

proc reset-rcvr {} {
	global rcvr
	foreach k "0 1 2 3 4 5 6 7 8" {
		$rcvr($k) unset ack_
	}
}

$ns at 2.345 "reset-rcvr"

Class Agent/Message/Flooder -superclass Agent/Message

Agent/Message/Flooder instproc flood n {
	while { $n > 0 } {
		$self send junk
		incr n -1
	}
}

set m0 [new Agent/Message/Flooder]
$ns attach-agent $node(10) $m0
set sink0 [new Agent/Null]
$ns attach-agent $node(3) $sink0
$ns connect $m0 $sink0
$m0 set class_ 4
$m0 set packetSize_ 1500

$ns at 1.977 "$m0 flood 10"

set m1 [new Agent/Message/Flooder]
$ns attach-agent $node(14) $m1
set sink1 [new Agent/Null]
$ns attach-agent $node(12) $sink1
$ns connect $m1 $sink1
$m1 set class_ 4
$m1 set packetSize_ 1500
$ns at 2.375 "$m1 flood 10"
$ns at 3.108 "$m1 flood 10"
$ns at 3.705 "$m1 flood 10"

$ns at 2.85 "reset-rcvr"

$ns at 3.6 "reset-rcvr"

$ns at 5.0 "finish"

proc finish {} {
	global ns f
	$ns flush-trace
	close $f

	puts "converting output to nam format..."
#	exec awk -f ../nam-demo/nstonam.awk out.tr > srm-demo-nam.tr 
	exec tclsh ../../../nam-1/ns_to_nam.tcl out.tr srm-demo-nam.tcl > srm-demo-nam.tr
exit 0
	exec rm -f out
	#XXX
	puts "running nam..."
	exec nam srm-demo-nam &
	exit 0
}

$ns run


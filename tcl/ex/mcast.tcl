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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/ex/mcast.tcl,v 1.1 1996/12/19 03:22:46 mccanne Exp $
#

#
# Simple multicast test.  It's easiest to verify the
# output with the animator.
# We create a four node start; start a CBR source in the center
# and then at node 3 and exercise the join/leave code.
#

#
# PLEASE NOTE: The current form of this file does not yet
#	conform with our intended API.
#

set ns [new MultiSim]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out.tr w]
$ns trace-all $f
#$ns trace-all stdout

$ns duplex-link $n0 $n1 1.5Mb 10ms drop-tail
$ns duplex-link $n1 $n2 1.5Mb 10ms drop-tail
$ns duplex-link $n1 $n3 1.5Mb 10ms drop-tail

set cbr0 [new agent/cbr]
$ns attach-agent $n1 $cbr0
$cbr0 set dst 0x8001

set cbr1 [new agent/cbr]
$cbr1 set dst 0x8002
$cbr1 set cls 1
$ns attach-agent $n3 $cbr1

set rcvr [new agent/loss-monitor]
$ns attach-agent $n3 $rcvr
$ns at 1.2 "$n2 join-group $rcvr 0x8002"
$ns at 1.25 "$n2 leave-group $rcvr 0x8002"
$ns at 1.3 "$n2 join-group $rcvr 0x8002"
$ns at 1.35 "$n2 join-group $rcvr 0x8001"

$ns at 1.0 "$cbr0 start"
#$ns at 1.001 "$cbr0 stop"
$ns at 1.1 "$cbr1 start"

#set tcp [new agent/tcp]
#set sink [new agent/tcp-sink]
#$ns attach-agent $n0 $tcp
#$ns attach-agent $n3 $sink
#$ns connect $tcp $sink
#set ftp [new source/ftp]
#$ftp set agent $tcp
#$ns at 1.2 "$ftp start"

#puts [$cbr0 set packet-size]
#puts [$cbr0 set interval]

$ns at 2.0 "finish"

proc finish {} {
	puts "converting output to nam format..."
	global ns
	$ns flush-trace
	exec awk -f ../nam-demo/nstonam.awk out.tr > mcast-nam.tr
	exec rm -f out
	#XXX
	puts "running nam..."
	exec nam mcast-nam &
	exit 0
}

$ns run

#
# Copyright (c) Xerox Corporation 1997. All rights reserved.
#
# License is granted to copy, to use, and to make and to use derivative
# works for research and evaluation purposes, provided that Xerox is
# acknowledged in all documentation pertaining to any such copy or
# derivative work. Xerox grants no other licenses expressed or
# implied. The Xerox trade name should not be used in any advertising
# without its written permission. 
#
# XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
# MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
# FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
# express or implied warranty of any kind.
#
# These notices must be retained in any copies of any part of this
# software. 
#


# This example script demonstrates using the token bucket filter as a
# traffic-shaper. 
# There are 2 identical source models(exponential on/off) connected to a common
# receiver. One of the sources is connected via a tbf whereas the other one is 
# connected directly.The tbf parameters are such that they shape the exponential
# on/off source to look like a cbr-like source.


set ns [new Simulator]
set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set f [open out.tr w]
$ns trace-all $f
set nf [open out.nam w]
$ns namtrace-all $nf

set trace_flow 1

$ns color 0 red
$ns color 1 blue

$ns duplex-link $n2 $n1 0.2Mbps 100ms DropTail
$ns duplex-link $n0 $n1 0.2Mbps 100ms DropTail

$ns duplex-link-op $n2 $n1 orient right-down
$ns duplex-link-op $n0 $n1 orient right-up

set exp1 [new Application/Traffic/Exponential]
$exp1 set packetSize_ 128
$exp1 set burst_time_ [expr 20.0/64]
$exp1 set idle_time_ 325ms
$exp1 set rate_ 65.536k

set a [new Agent/UDP]
$a set fid_ 0
$a set rate_ 32.768k
$a set bucket_ 1024
$exp1 attach-agent $a

set tbf [new TBF]
$tbf set bucket_ [$a set bucket_]
$tbf set rate_ [$a set rate_]
$tbf set qlen_  100

$ns attach-tbf-agent $n0 $a $tbf

set rcvr [new Agent/SAack]
$ns attach-agent $n1 $rcvr

$ns connect $a $rcvr

set exp2 [new Application/Traffic/Exponential]
$exp2 set packetSize_ 128
$exp2 set burst_time_ [expr 20.0/64]
$exp2 set idle_time_ 325ms
$exp2 set rate_ 65.536k

set a2 [new Agent/UDP]
$a2 set fid_ 1
$exp2 attach-agent $a2

$ns attach-agent $n2 $a2

$ns connect $a2 $rcvr

$ns at 0.0 "$exp1 start;$exp2 start"
$ns at 20.0 "$exp1 stop;$exp2 stop;close $f;close $nf;exec nam out.nam &;exit 0"
$ns run




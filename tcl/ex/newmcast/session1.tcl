#
# tcl/ex/newmcast/session1.tcl
#
# Copyright (C) 1997 by USC/ISI
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 
# Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
# 
#
# Session Level Simulator
#
# joining & pruning test(s) 
#          |3|
#           |
#  |0|-----|1|
#           |
#          |2|

set ns [new SessionSim]
#SessionSim set rc_ 1

$ns namtrace-all [open s1.nam w]
set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n3 1.5Mb 10ms DropTail

set cbr1 [new Agent/CBR]
$ns attach-agent $n2 $cbr1
$cbr1 set dst_ 0x8003

#### IMPORTANT: have to do $ns create-session 
####            after creating the source and 
####            before adding members
####  
$ns create-session $n2 $cbr1

set rcvr0 [new Agent/LossMonitor]
$ns attach-agent $n0 $rcvr0
set rcvr1 [new Agent/LossMonitor]
$ns attach-agent $n1 $rcvr1
set rcvr2  [new Agent/LossMonitor]
$ns attach-agent $n2 $rcvr2
set rcvr3 [new Agent/LossMonitor]
$ns attach-agent $n3 $rcvr3

$ns at 0.3 "$cbr1 start"
$ns at 0.3 "$n1 join-group  $rcvr1 0x8003"
$ns at 0.3 "$n0 join-group  $rcvr0 0x8003"
$ns at 0.3 "$n3 join-group  $rcvr3 0x8003"
$ns at 0.3 "$n2 join-group  $rcvr2 0x8003"

$ns at 1.1 "$ns finish"

SessionSim instproc finish {} {
    global rcvr0 rcvr1 rcvr2 rcvr3 ns
	$ns flush-trace
    puts "[$rcvr0 set npkts_] [$rcvr0 set nlost_]"
    puts "[$rcvr1 set npkts_] [$rcvr1 set nlost_]"
    puts "[$rcvr2 set npkts_] [$rcvr2 set nlost_]"
    puts "[$rcvr3 set npkts_] [$rcvr3 set nlost_]"
    exit 0
}

$ns run



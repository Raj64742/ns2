#
# tcl/ex/newmcast/session2.tcl
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
#      0
#    +-+--+
#    1    2
# +--+-+--+--+
# 3    4     5
#
set ns [new SessionSim]
SessionSim set rc_ 1

$ns namtrace-all [open s3.nam w]

$ns color 0 red
$ns color 1 magenta
$ns color 2 orange
$ns color 3 yellow
$ns color 4 purple
$ns color 5 brown
$ns color 6 chocolate
$ns color 7 green
$ns color 8 cyan

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
set n4 [$ns node]
set n5 [$ns node]

$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n0 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n3 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n4 1.5Mb 10ms DropTail
$ns duplex-link $n2 $n4 1.5Mb 10ms DropTail
$ns duplex-link $n2 $n5 1.5Mb 10ms DropTail

set cbr0 [new Agent/CBR]
$cbr0 set ttl_ 3
$ns attach-agent $n0 $cbr0
$cbr0 set dst_ 0x8002
set sessionhelper [$ns create-session $n0 $cbr0]

set rcvr0 [new Agent/LossMonitor]
set rcvr1 [new Agent/LossMonitor]
set rcvr2 [new Agent/LossMonitor] 
set rcvr3 [new Agent/LossMonitor]
set rcvr4 [new Agent/LossMonitor]
set rcvr5 [new Agent/LossMonitor]
$ns attach-agent $n0 $rcvr0
$ns attach-agent $n1 $rcvr1
$ns attach-agent $n2 $rcvr2
$ns attach-agent $n3 $rcvr3
$ns attach-agent $n4 $rcvr4
$ns attach-agent $n5 $rcvr5


$ns at 0.2 "$n0 join-group $rcvr0 0x8002"
$ns at 0.2 "$n1 join-group $rcvr1 0x8002"
$ns at 0.2 "$n2 join-group $rcvr2 0x8002"
$ns at 0.2 "$n3 join-group $rcvr3 0x8002"
$ns at 0.2 "$n4 join-group $rcvr4 0x8002"
$ns at 0.2 "$n5 join-group $rcvr5 0x8002"

# set up a constant loss module; see tcl/ex/ranvar.tcl for more 
# random variables
#set loss_random_variable [new RandomVariable/Constant]
#$loss_random_variable set avg_ 2
#set loss_module [new ErrorModel]
# when ranvar avg_ < erromodel rate_ pkts are dropped
#$loss_module drop-target [new Agent/Null]
#$loss_module set rate_ 3
#$loss_module ranvar $loss_random_variable

set loss_module1 [new SelectErrorModel]
$loss_module1 drop-packet 2 20 1     ;# drop one PT_CBR packet every 20 packets
$loss_module1 drop-target [$ns set nullAgent_]

set loss_module2 [new SelectErrorModel]
$loss_module2 drop-packet 2 10 1     ;# drop one PT_CBR packet every 10 packets
$loss_module2 drop-target [$ns set nullAgent_]

set loss_module3 [new SelectErrorModel]
$loss_module3 drop-packet 2 10 1     ;# drop one PT_CBR packet every 10 packets
$loss_module3 drop-target [$ns set nullAgent_]

# insert the loss module; must be done before receivers join groups
$ns insert-loss  $loss_module1 $n0 $n1
$ns insert-loss  $loss_module2 $n1 $n3
$ns insert-loss  $loss_module3 $n0 $n2
$ns at 0.3 "$sessionhelper show-loss-depobj"  ;# showing loss dependency
$ns at 0.3 "$sessionhelper show-dstobj"       ;# showing receiver spec

$ns at 0.1 "$cbr0 start"
 
$ns at 1.6 "finish"

proc finish {} {
        global rcvr3 rcvr4 rcvr5  n0 rcvr2 rcvr1 rcvr0 ns
	$ns flush-trace
        puts "rcvr 0 lost [$rcvr0 set nlost_] pkt, rcv [$rcvr0 set npkts_]"
        puts "rcvr 1 lost [$rcvr1 set nlost_] pkt, rcv [$rcvr1 set npkts_]"
        puts "rcvr 2 lost [$rcvr2 set nlost_] pkt, rcv [$rcvr2 set npkts_]"
        puts "rcvr 3 lost [$rcvr3 set nlost_] pkt, rcv [$rcvr3 set npkts_]"
        puts "rcvr 4 lost [$rcvr4 set nlost_] pkt, rcv [$rcvr4 set npkts_]"
        puts "rcvr 5 lost [$rcvr5 set nlost_] pkt, rcv [$rcvr5 set npkts_]"
#        puts "Showing Dependency List:"
#        $n0 dump-dependency 0 0x8002
#        puts "[$n0 get-dependency 0 0x8002]"
        exit 0
}

$ns run



#
# tcl/ex/newmcast/dense-mode1.tcl
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
# Ported by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
# 
#
set ns [new Simulator]
Simulator set EnableMcast_ 1
Simulator set NumberInterfaces_ 1

$ns color 1 red
# prune/graft packets
$ns color 30 purple
$ns color 31 green

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out-dm1.tr w]
$ns trace-all $f
set nf [open out-dm1.nam w]
$ns namtrace-all $nf

$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n3 1.5Mb 10ms DropTail

$ns duplex-link-op $n0 $n1 orient right
$ns duplex-link-op $n1 $n2 orient right-up
$ns duplex-link-op $n1 $n3 orient right-down

$ns duplex-link-op $n0 $n1 queuePos 0.5
$ns duplex-link-op $n1 $n0 queuePos 0.5
$ns duplex-link-op $n3 $n1 queuePos 0.5

set mproto DM
set mrthandle [$ns mrtproto $mproto {}]

set cbr0 [new Agent/CBR]
$ns attach-agent $n1 $cbr0
$cbr0 set dst_ 0x8001
 
set cbr1 [new Agent/CBR]
$cbr1 set dst_ 0x8002
$cbr1 set class_ 1
$ns attach-agent $n3 $cbr1

set rcvr [new Agent/LossMonitor]
$ns attach-agent $n2 $rcvr
$ns at 1.2 "$n2 join-group $rcvr 0x8002"
$ns at 1.25 "$n2 leave-group $rcvr 0x8002"
$ns at 1.3 "$n2 join-group $rcvr 0x8002"
$ns at 1.35 "$n2 join-group $rcvr 0x8001"
 
$ns at 1.0 "$cbr0 start"
$ns at 1.1 "$cbr1 start"
 
set tcp [new Agent/TCP]
set sink [new Agent/TCPSink]
$ns attach-agent $n0 $tcp
$ns attach-agent $n3 $sink
$ns connect $tcp $sink
set ftp [new Source/FTP]
$ftp set agent_ $tcp
$ns at 1.2 "$ftp start"


$ns at 1.7 "finish"

proc finish {} {
        puts "converting output to nam format..."
        global ns
        $ns flush-trace
        #XXX
        puts "running nam..."
        exec nam out-dm1 &
        exit 0
}

$ns run


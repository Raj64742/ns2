 #
 # tcl/ex/newmcast/detailedDM2.tcl
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
## testing prune override, asserts, deletion timers ... richer 8 node topology
## testing performance with dynamics

set ns [new Simulator]
Simulator set EnableMcast_ 1

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
set n4 [$ns node]
set n5 [$ns node]
set n6 [$ns node]
set n7 [$ns node]

set f [open out-detailedDM2.tr w]
$ns trace-all $f

# 	|4| - |7|
#     /	      /
#   |6|	   |3|
# --------------
#  |5|     |2|
#   |	    |
#  |0|	   |1|
Simulator set NumberInterfaces_ 1
$ns multi-link-of-interfaces [list $n5 $n2 $n3 $n6] 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n4 $n7 1.5Mb 10ms DropTail
$ns duplex-link $n7 $n3 1.5Mb 10ms DropTail
$ns duplex-link $n5 $n0 1.5Mb 10ms DropTail
$ns duplex-link $n4 $n6 1.5Mb 10ms DropTail

Prune/Iface/Timer set timeout 0.3
Deletion/Iface/Timer set timeout 0.1
set mproto detailedDM
$ns mrtproto $mproto {}
$ns rtproto Session

set rcvr0 [new Agent/LossMonitor]
$ns attach-agent $n0 $rcvr0
set rcvr1 [new Agent/LossMonitor]
$ns attach-agent $n1 $rcvr1
set rcvr2  [new Agent/LossMonitor]
$ns attach-agent $n2 $rcvr2
set rcvr3 [new Agent/LossMonitor]
$ns attach-agent $n3 $rcvr3

set sender0 [new Agent/CBR]
$sender0 set dst_ 0x8003
$sender0 set class_ 2
#$ns attach-agent $n0 $sender0
$ns attach-agent $n4 $sender0

$ns at 1.2 "$n0 join-group $rcvr0 0x8003"
$ns at 1.0 "$sender0 start"
#$ns at 2.3 "$n0 leave-group $rcvr0 0x8003"
#$ns at 2.3 "$n1 leave-group $rcvr1 0x8003"
$ns at 3.2 "finish"

### Link between n4 and n6 down at 1.5, up at 2.0
$ns rtmodel-at 1.6 down $n4 $n6
$ns rtmodel-at 2.0 up $n4 $n6
###

# $ns at 2.01 "finish"

proc finish {} {
        global ns
        $ns flush-trace
        exec awk -f ../../nam-demo/nstonam.awk out-detailedDM2.tr > detailedDM2-nam.tr
        # exec rm -f out
        #XXX
        puts "running nam..."
        exec nam detailedDM2-nam &
        exit 0
}

$ns run

 #
 # tcl/ex/newmcast/mcast2.tcl
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
##
##      0
##    +-+--+
##    1    2
## +--+-+--+--+
## 3    4     5
##

set ns [new MultiSim]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
set n4 [$ns node]
set n5 [$ns node]

set f [open out-mc2.tr w]
$ns trace-all $f

$ns duplex-link-of-interfaces $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n0 $n2 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n1 $n3 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n1 $n4 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n2 $n4 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n2 $n5 1.5Mb 10ms DropTail

### Start multicast configuration
DM set PruneTimeout 0.3
dynamicDM set ReportRouteTimeout 0.15
set mproto dynamicDM
set mrthandle [$ns mrtproto $mproto  {}]
### End of multicast  config

set cbr0 [new Agent/CBR]
$ns attach-agent $n0 $cbr0
$cbr0 set dst_ 0x8002
 
set rcvr [new Agent/LossMonitor]
$ns attach-agent $n3 $rcvr
$ns attach-agent $n4 $rcvr
$ns attach-agent $n5 $rcvr

$ns at 0.2 "$n3 join-group $rcvr 0x8002"
$ns at 0.4 "$n4 join-group $rcvr 0x8002"
$ns at 0.6 "$n3 leave-group $rcvr 0x8002"
$ns at 0.7 "$n5 join-group $rcvr 0x8002"
$ns at 0.8 "$n3 join-group $rcvr 0x8002"

#### Link between n0 & n1 down at 1.0, up at 1.2
$ns rtmodel-at 1.0 down $n0 $n1
$ns rtmodel-at 1.2 up $n0 $n1
####

$ns at 0.1 "$cbr0 start"
 
$ns at 1.6 "finish"

proc finish {} {
        puts "converting output to nam format..."
        global ns
        $ns flush-trace
        global rcvr
        # puts "lost [$rcvr set nlost_] pkt, rcv [$rcvr set npkts_]"
        exec awk -f ../../nam-demo/nstonam.awk out-mc2.tr > mcast2-nam.tr
        exec rm -f out
        #XXX
        puts "running nam..."
        exec nam mcast2-nam &
        exit 0
}

$ns run Session


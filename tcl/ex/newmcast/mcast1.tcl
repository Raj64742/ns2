 #
 # tcl/ex/newmcast/mcast1.tcl
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
## Using Multicast in ns
##
## joining & pruning test(s) 
#          |3|
#           |
#  |0|-----|1|
#           |
#          |2|

set ns [new Simulator -multicast on]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out-cmcast.tr w]
$ns trace-all $f
set nf [open out-cmcast.nam w]
$ns namtrace-all $nf

$ns color 2 black
$ns color 1 blue
$ns color 0 yellow
$ns color 30 purple
$ns color 31 green

$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n3 1.5Mb 10ms DropTail

$ns duplex-link-op $n0 $n1 orient right
$ns duplex-link-op $n1 $n2 orient right-up
$ns duplex-link-op $n1 $n3 orient right-down

$ns duplex-link-op $n0 $n1 queuePos 0.5
$ns duplex-link-op $n1 $n0 queuePos 0.5
$ns duplex-link-op $n3 $n1 queuePos 0.5


### Start multicast configuration: 5 mproto options
### CtrMcast   : centralized multicast
### DM         : static DVMRP (can't adapt to link up/down or node up/down)
### detailedDM : dense mode protocol that adapts to dynamics (recommended)
### dynamicDM  : dynamic DVMRP 
### pimDM      : PIM dense mode

### Uncomment following lines to change default
#DM set PruneTimeout 0.3               ;# default 0.5 (sec)
#dynamicDM set ReportRouteTimeout 0.5  ;# default 1 (sec)

set mproto CtrMcast
set mrthandle [$ns mrtproto $mproto  {}]
if {$mrthandle != ""} {
    $mrthandle set_c_rp [list $n2]
}
### End of multicast configuration

set cbr1 [new Agent/CBR]
$ns attach-agent $n2 $cbr1
$cbr1 set dst_ 0x8003

set rcvr0 [new Agent/LossMonitor]
$ns attach-agent $n0 $rcvr0
set rcvr1 [new Agent/LossMonitor]
$ns attach-agent $n1 $rcvr1
set rcvr2  [new Agent/LossMonitor]
$ns attach-agent $n2 $rcvr2
set rcvr3 [new Agent/LossMonitor]
$ns attach-agent $n3 $rcvr3

$ns at 0.66 "$cbr1 start"
$ns at 0.3 "$n1 join-group  $rcvr1 0x8003"
$ns at 0.4 "$n0 join-group  $rcvr0 0x8003"
#if {$mrthandle != ""} {
#    $ns at 0.45 "$mrthandle switch-treetype 0x8003"
#}

$ns at 0.5 "$n3 join-group  $rcvr3 0x8003"
$ns at 0.65 "$n2 join-group  $rcvr2 0x8003"

$ns at 0.7 "$n0 leave-group $rcvr0 0x8003"
$ns at 0.8 "$n2 leave-group  $rcvr2 0x8003"
$ns at 0.9 "$n3 leave-group  $rcvr3 0x8003"
$ns at 1.0 "$n1 leave-group $rcvr1 0x8003"
#$ns at 1.1 "$n1 join-group $rcvr1 0x8003"

$ns at 1.2 "finish"

proc finish {} {
        global ns
        $ns flush-trace
    global rcvr0 rcvr1 rcvr2 rcvr3
    puts "[$rcvr0 set npkts_] [$rcvr0 set nlost_]"
    puts "[$rcvr1 set npkts_] [$rcvr1 set nlost_]"
    puts "[$rcvr2 set npkts_] [$rcvr2 set nlost_]"
    puts "[$rcvr3 set npkts_] [$rcvr3 set nlost_]"
        #exec awk -f ../../nam-demo/nstonam.awk out-cmcast.tr > cmcast-nam.tr
        #XXX
        puts "running nam..."
        exec nam out-cmcast &
        exit 0
}

$ns run




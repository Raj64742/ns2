#
# tcl/ex/newmcast/cmcast-spt.tcl
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
# Centralized Multicast Module Examples
# o. Use only source specific tree. 
# o. Note that when all receivers leave a group, the group tree type will be
# reset to the default, RP tree.
# o. RP related settings are not required.
#
# joining & pruning test(s)
#          |3|
#           |
#  |0|-----|1|
#           |
#          |2|
set ns [new Simulator]
Simulator set EnableMcast_ 1
Simulator set NumberInterfaces_ 1

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

# mcast set-up
set mproto CtrMcast
set mrthandle [$ns mrtproto $mproto  {}]

# Create a sender and receivers
set cbr1 [new Agent/CBR]
$ns attach-agent $n2 $cbr1
$cbr1 set dst_ 0x8003

set rcvr0 [new Agent/Null]
$ns attach-agent $n0 $rcvr0
set rcvr1 [new Agent/Null]
$ns attach-agent $n1 $rcvr1
set rcvr2  [new Agent/Null]
$ns attach-agent $n2 $rcvr2
set rcvr3 [new Agent/Null]
$ns attach-agent $n3 $rcvr3

# Switch group 0x8003 to source specific tree at the beginning
# Default tree type is RP tree (shared tree)
$ns at 0 "$mrthandle switch-treetype 0x8003"

# Group events
$ns at 0.2 "$cbr1 start"
$ns at 0.3 "$n1 join-group  $rcvr1 0x8003"
$ns at 0.4 "$n0 join-group  $rcvr0 0x8003"
$ns at 0.5 "$n3 join-group  $rcvr3 0x8003"
$ns at 0.65 "$n2 join-group  $rcvr2 0x8003"

$ns at 0.7 "$n0 leave-group $rcvr0 0x8003"
$ns at 0.8 "$n2 leave-group  $rcvr2 0x8003"

# Re-compute the entire mcast routes. 
# In this case, nothing is changed
$ns at 0.85 "$mrthandle compute-mroutes"
$ns at 0.9 "$n3 leave-group  $rcvr3 0x8003"
$ns at 1.0 "$n1 leave-group $rcvr1 0x8003"


$ns at 1.1 "finish"

proc finish {} {
        global ns
        $ns flush-trace
        #exec awk -f ../../nam-demo/nstonam.awk out-cmcast.tr > cmcast-nam.tr
        # exec rm -f out
        #XXX
        puts "running nam..."
        exec nam out-cmcast.nam &
        exit 0
}

$ns run



 #
 # tcl/ex/newmcast/cmcast.tcl
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
## Centralized Multicast Module Examples
## o. Use default RP tree(share tree) at first, then switch
## o. RP related settings are required, such as C_RP, C_BSR, compute-rpset,...
##
## joining & pruning test(s) 
#          |3|
#           |
#  |0|-----|1|
#           |
#          |2|

set ns [new MultiSim]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out-cmcast.tr w]
$ns trace-all $f

$ns duplex-link-of-interfaces $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n1 $n3 1.5Mb 10ms DropTail

set ctrmcastcomp [new CtrMcastComp $ns]
set ctrrpcomp [new CtrRPComp $ns]

# [list C_RP C_BSR priority]
# Currently, the highest id nodes with C_RP set to 1 will be the RP for 
# all groups.
# At least one node's C_BSR has to be set to 1
set ctrmcast0 [new CtrMcast $ns $n0 $ctrmcastcomp [list 1 1]]
set ctrmcast1 [new CtrMcast $ns $n1 $ctrmcastcomp [list 0 1]]
set ctrmcast2 [new CtrMcast $ns $n2 $ctrmcastcomp [list 1 1]]
set ctrmcast3 [new CtrMcast $ns $n3 $ctrmcastcomp [list 1 1]]

$ns compute-routes
$ctrrpcomp compute-rpset

set i 0
set n [Node set nn_]
while { $i < $n } {
    set arbiter [[$ns set Node_($i)] getArbiter]
    set ctrmcast [$arbiter getType "CtrMcast"]
    puts "[[$ns set Node_($i)] id] rpset = [$ctrmcast set rpset]"
    incr i
}

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


$ns at 0.2 "$cbr1 start"
$ns at 0.3 "$n1 join-group  $rcvr1 0x8003"
$ns at 0.4 "$n0 join-group  $rcvr0 0x8003"
$ns at 0.45 "$ctrmcastcomp switch-treetype 0x8003"
$ns at 0.6 "$n3 join-group  $rcvr3 0x8003"
$ns at 0.6.5 "$n2 join-group  $rcvr2 0x8003"

$ns at 0.7 "$n0 leave-group $rcvr0 0x8003"
$ns at 0.8 "$n2 leave-group  $rcvr2 0x8003"
$ns at 0.85 "$ctrmcastcomp compute-mroutes"
$ns at 0.9 "$n3 leave-group  $rcvr3 0x8003"
$ns at 1.0 "$n1 leave-group $rcvr1 0x8003"


$ns at 1.1 "finish"

proc finish {} {
        global ns
        $ns flush-trace
        exec awk -f ../../nam-demo/nstonam.awk out-cmcast.tr > cmcast-nam.tr
        # exec rm -f out
        #XXX
        puts "running nam..."
        exec nam cmcast-nam &
        exit 0
}

$ns run



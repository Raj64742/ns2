#
# tcl/ex/newmcast/dense-mode2.tcl
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

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out-dm2.tr w]
$ns trace-all $f

$ns multi-link [list $n0 $n1 $n2 $n3] 1.5Mb 10ms DropTail

set mproto DM
set mrthandle [$ns mrtproto $mproto {}]

set rcvr0 [new Agent/Null]
$ns attach-agent $n0 $rcvr0
set rcvr1 [new Agent/Null]
$ns attach-agent $n1 $rcvr1
set rcvr2  [new Agent/Null]
$ns attach-agent $n2 $rcvr2
set rcvr3 [new Agent/Null]
$ns attach-agent $n3 $rcvr3

set sender0 [new Agent/CBR]
$sender0 set dst_ 0x8003
$sender0 set class_ 2
$ns attach-agent $n0 $sender0

#$ns at 1.95 "$n0 join-group $rcvr0 0x8003"
#$ns at 1.96 "$n1 join-group $rcvr1 0x8003"
$ns at 2.05 "$n1 join-group $rcvr1 0x8003"
$ns at 2.2 "$n2 join-group $rcvr2 0x8003"
# $ns at 1.98 "$n3 join-group $rcvr3 0x8003"
# $ns at 2.05 "$n3 join-group $rcvr3 0x8003"

$ns at 2.0 "$sender0 start"

$ns at 2.7 "finish"

proc finish {} {
        global ns
        $ns flush-trace
        exec awk -f ../../nam-demo/nstonam.awk out-dm2.tr > dense-mode2-nam.tr
        # exec rm -f out
        #XXX
        puts "running nam..."
        exec nam dense-mode2-nam &
        exit 0
}

$ns run

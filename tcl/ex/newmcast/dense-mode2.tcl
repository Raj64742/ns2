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

# Note: nam trace currently broken because DummyLink only generates two 
# types of events ('-' and 'd'), but nam needs two more: ('+', 'h'). This 
# will be fixed in the next release

set ns [new Simulator]
Simulator set EnableMcast_ 1

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

$ns color 1 red
$ns color 2 black

$ns color 30 red
$ns color 31 green
$ns color 104 blue

set f [open out-dm2.tr w]
$ns trace-all $f
#set nf [open out-dm2.nam w]
#$ns namtrace-all $nf

$ns multi-link-of-interfaces [list $n0 $n1 $n2 $n3] 1.5Mb 10ms DropTail

#$ns duplex-link-op $n0 $n1 orient right
#$ns duplex-link-op $n1 $n2 orient down
#$ns duplex-link-op $n1 $n3 orient left-down
#$ns duplex-link-op $n0 $n3 orient down
#$ns duplex-link-op $n3 $n2 orient left
#$ns duplex-link-op $n0 $n2 orient right-down

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
        #XXX
#        puts "running nam..."
#        exec nam out-dm2 &
        exit 0
}

$ns run

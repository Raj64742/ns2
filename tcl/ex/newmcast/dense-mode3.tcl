#
# tcl/ex/newmcast/dense-mode3.tcl
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
set n4 [$ns node]
set n5 [$ns node]

set f [open out-dm3.tr w]
$ns trace-all $f

# 	|4|
#	 |
#	|3|
# --------------
#  |5|     |2|
#   |	    |
#  |0|	   |1|
Simulator set NumberInterfaces_ 1
$ns multi-link-of-interfaces [list $n5 $n2 $n3] 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n4 $n3 1.5Mb 10ms DropTail
$ns duplex-link $n5 $n0 1.5Mb 10ms DropTail

set mproto DM
set mrthandle [$ns mrtproto $mproto {}]

Agent/Message instproc handle msg {
        $self instvar node_
        puts "@@@@@@@@@node [$node_ id] agent $self rxvd msg $msg. @@@@@@@@"
}

set rcvr0 [new Agent/Message]
$ns attach-agent $n0 $rcvr0
set rcvr1 [new Agent/Message]
$ns attach-agent $n1 $rcvr1
set rcvr2  [new Agent/Message]
$ns attach-agent $n2 $rcvr2
set rcvr3 [new Agent/Message]
$ns attach-agent $n3 $rcvr3

set sender0 [new Agent/Message]
$sender0 set dst_ 0x8003
$sender0 set class_ 2
$ns attach-agent $n0 $sender0

$ns at 1.95 "$n0 join-group $rcvr0 0x8003"
#$ns at 1.96 "$n1 join-group $rcvr1 0x8003"
$ns at 2.05 "$n1 join-group $rcvr1 0x8003"
# $ns at 1.97 "$n2 join-group $rcvr2 0x8003"
# $ns at 1.98 "$n3 join-group $rcvr3 0x8003"
# $ns at 2.05 "$n3 join-group $rcvr3 0x8003"

$ns at 2.0 "$sender0 send \"pkt1\""
$ns at 2.1 "$sender0 send \"pkt2\""
$ns at 2.13 "$sender0 send \"pkt3\""
$ns at 2.16 "$sender0 send \"pkt4\""
$ns at 2.2 "$sender0 send \"pkt5\""
$ns at 2.24 "$sender0 send \"pkt6\""
$ns at 2.28 "$sender0 send \"pkt7\""
$ns at 3.12 "$sender0 send \"pkt8\""
$ns at 3.16 "$sender0 send \"pkt9\""
$ns at 4.0 "$sender0 send \"pkt10\""


$ns at 5.0 "finish"
# $ns at 2.01 "finish"

# Note: nam trace currently broken because DummyLink only generates two 
# types of events ('-' and 'd'), but nam needs two more: ('+', 'h'). This 
# will be fixed in the next release
proc finish {} {
        global ns
        $ns flush-trace
        #XXX
        #puts "running nam..."
        #exec nam dense-mode3-nam &
        exit 0
}

$ns run

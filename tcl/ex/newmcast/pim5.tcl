 #
 # tcl/ex/newmcast/pim5.tcl
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
## testing joins and prunes.. richer 7 node topology

set ns [new Simulator]
Simulator set EnableMcast_ 1

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
set n4 [$ns node]
set n5 [$ns node]
set n6 [$ns node]

set f [open out-pim5.tr w]
$ns trace-all $f

# 	|4|
#     /	   \
#   |6|	   |3|
# --------------
#  |5|     |2|
#   |	    |
#  |0|	   |1|
Simulator set NumberInterfaces_ 1
$ns multi-link-of-interfaces [list $n5 $n2 $n3 $n6] 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n4 $n3 1.5Mb 10ms DropTail
$ns duplex-link $n5 $n0 1.5Mb 10ms DropTail
$ns duplex-link $n4 $n6 1.5Mb 10ms DropTail

set pim0 [new PIM $ns $n0 [list 1 1 0]]
set pim1 [new PIM $ns $n1 [list 0 1 0]]
set pim2 [new PIM $ns $n2 [list 1]]
set pim3 [new PIM $ns $n3 [list 0 1 0]]
set pim4 [new PIM $ns $n4 [list 1]]
set pim5 [new PIM $ns $n5 [list 0]]
set pim6 [new PIM $ns $n6 [list 0]]

PIM config $ns

$ns at 0.0 "$ns run-mcast"

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
$ns at 2.16 "$sender0 send \"pkt3\""
$ns at 2.2 "$sender0 send \"pkt3\""

$ns at 2.5 "finish"
# $ns at 2.01 "finish"

proc finish {} {
        global ns
        $ns flush-trace
        exec awk -f ../../nam-demo/nstonam.awk out-pim5.tr > pim5-nam.tr
        # exec rm -f out
        #XXX
        puts "running nam..."
        exec nam pim5-nam &
        exit 0
}

$ns run

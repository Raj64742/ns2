## testing joins and prunes.. richer 10 node topology

set ns [new MultiSim]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
set n4 [$ns node]
set n5 [$ns node]
set n6 [$ns node]
set n7 [$ns node]
set n8 [$ns node]
set n9 [$ns node]

set f [open out-pim6.tr w]
$ns trace-all $f

#       |0|
#        |
# 	|4|
#     /	   \
#   |7|    |8|
# -------------
#   |6|	   |3|
# --------------
#  |5|     |2|
#   |	    |
#  |9|	   |1|
$ns multi-link-of-interfaces [list $n5 $n2 $n3 $n6] 1.5Mb 10ms DropTail
$ns multi-link-of-interfaces [list $n7 $n8 $n3 $n6] 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n4 $n8 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n5 $n9 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n4 $n7 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n4 $n0 1.5Mb 10ms DropTail

set pim0 [new PIM $ns $n0 [list 1 1 0]]
set pim1 [new PIM $ns $n1 [list 0 1 0]]
set pim2 [new PIM $ns $n2 [list 1]]
set pim3 [new PIM $ns $n3 [list 0 1 0]]
set pim4 [new PIM $ns $n4 [list 1]]
set pim5 [new PIM $ns $n5 [list 0]]
set pim6 [new PIM $ns $n6 [list 0]]
set pim7 [new PIM $ns $n7 [list 0]]
set pim8 [new PIM $ns $n8 [list 0]]
set pim9 [new PIM $ns $n9 [list 0]]

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
set rcvr9 [new Agent/Message]
$ns attach-agent $n9 $rcvr9

set sender0 [new Agent/Message]
$sender0 set dst_ 0x8003
$sender0 set class_ 2
$ns attach-agent $n0 $sender0

$ns at 1.0 "$n0 join-group $rcvr0 0x8003"
#$ns at 1.96 "$n1 join-group $rcvr1 0x8003"
$ns at 1.2 "$n1 join-group $rcvr1 0x8003"
# $ns at 1.97 "$n2 join-group $rcvr2 0x8003"
# $ns at 1.98 "$n3 join-group $rcvr3 0x8003"
# $ns at 2.05 "$n3 join-group $rcvr3 0x8003"
$ns at 1.4 "$n9 join-group $rcvr9 0x8003"

$ns at 1.1 "$sender0 send \"pkt1\""
$ns at 1.3 "$sender0 send \"pkt2\""
$ns at 1.35 "$sender0 send \"pkt3\""
$ns at 1.44 "$sender0 send \"pkt4\""
$ns at 1.48 "$sender0 send \"pkt5\""
$ns at 1.52 "$sender0 send \"pkt6\""

$ns at 2.5 "finish"
# $ns at 2.01 "finish"

proc finish {} {
        global ns
        $ns flush-trace
        exec awk -f ../nam-demo/nstonam.awk out-pim6.tr > pim6-nam.tr
        # exec rm -f out
        #XXX
        puts "running nam..."
        exec ./nam pim6-nam &
        exit 0
}

$ns run

set ns [new MultiSim]

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
$ns multi-link-of-interfaces [list $n5 $n2 $n3] 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n4 $n3 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n5 $n0 1.5Mb 10ms DropTail

set dm0 [new DM $ns $n0]
set dm1 [new DM $ns $n1]
set dm2 [new DM $ns $n2]
set dm3 [new DM $ns $n3]
set dm4 [new DM $ns $n4]
set dm5 [new DM $ns $n5]

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
$ns at 2.16 "$sender0 send \"pkt4\""
$ns at 2.2 "$sender0 send \"pkt5\""
$ns at 2.24 "$sender0 send \"pkt6\""
$ns at 2.28 "$sender0 send \"pkt7\""
$ns at 3.12 "$sender0 send \"pkt8\""
$ns at 3.16 "$sender0 send \"pkt9\""
$ns at 4.0 "$sender0 send \"pkt10\""


$ns at 5.0 "finish"
# $ns at 2.01 "finish"

proc finish {} {
        global ns
        $ns flush-trace
        exec awk -f ../../nam-demo/nstonam.awk out-dm3.tr > dense-mode3-nam.tr
        # exec rm -f out
        #XXX
        puts "running nam..."
        exec nam dense-mode3-nam &
        exit 0
}

$ns run

set ns [new MultiSim]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out-dm2.tr w]
$ns trace-all $f

$ns multi-link [list $n0 $n1 $n2 $n3] 1.5Mb 10ms DropTail

set dm0 [new DM $ns $n0]
set dm1 [new DM $ns $n1]
set dm2 [new DM $ns $n2]
set dm3 [new DM $ns $n3]

$ns at 0.0 "$ns run-mcast"

Agent/Message instproc handle msg {
	$self instvar node_
	puts "Node [$node_ id] agent $self rxvd msg $msg"
}

set rcvr0 [new Agent/Message]
$ns attach-agent $n0 $rcvr0
set rcvr1 [new Agent/Message]
$ns attach-agent $n1 $rcvr1
set rcvr2  [new Agent/Message]
$ns attach-agent $n2 $rcvr2
set rcvr3 [new Agent/Message]
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
#$ns at 2.01 "finish"

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

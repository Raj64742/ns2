
set ns [new Simulator]
$ns use-scheduler RealTime

set net1 [new Network/IP]
$net1 open 10.0.0.2 6623 16

set net2 [new Network/IP]
$net2 open 10.0.0.3 6628 16

set a [new Agent/Tap]
$a network $net1

set b [new Agent/Tap]
$b network $net2

set n0 [$ns node]
set n1 [$ns node]

$ns duplex-link $n0 $n1 8Mb 5ms DropTail

$ns attach-agent $n0 $a
$ns attach-agent $n1 $b
$ns connect $a $b

$ns run

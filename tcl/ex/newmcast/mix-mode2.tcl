set ns [new SessionSim]
$ns multicast

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
set n4 [$ns node]
set n5 [$ns node]

$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n0 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n3 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n4 1.5Mb 10ms DropTail
$ns duplex-link $n2 $n4 1.5Mb 10ms DropTail
$ns duplex-link $n2 $n5 1.5Mb 10ms DropTail

#$ns detailed-duplex-link $n0 $n1
$ns detailed-duplex-link $n0 $n2
$ns detailed-duplex-link $n1 $n3
#$ns detailed-duplex-link $n1 $n4
$ns detailed-duplex-link $n2 $n4
#$ns detailed-duplex-link $n2 $n5

set mproto CtrMcast
set mrthandle [$ns mrtproto $mproto  {}]

set udp0 [new Agent/UDP]
$udp0 set dst_ 0x8002
$udp0 set class_ 1
$ns attach-agent $n0 $udp0
set cbr0 [new Application/Traffic/CBR]
$cbr0 attach-agent $udp0
$ns create-session $n0 $udp0

set rcvr3 [new Agent/LossMonitor]
set rcvr4 [new Agent/LossMonitor]
set rcvr5 [new Agent/LossMonitor]
$ns attach-agent $n3 $rcvr3
$ns attach-agent $n4 $rcvr4
$ns attach-agent $n5 $rcvr5

if {$mrthandle != ""} {
    $ns at 0.01 "$mrthandle switch-treetype 0x8002"
}
$ns at 0.2 "$n3 join-group $rcvr3 0x8002"
$ns at 0.4 "$n4 join-group $rcvr4 0x8002"
$ns at 0.7 "$n5 join-group $rcvr5 0x8002"


$ns at 0.1 "$cbr0 start"
 
$ns at 1.6 "finish"

proc finish {} {
        global ns
        global rcvr3 rcvr4 rcvr5
        $ns flush-trace
        puts "lost [$rcvr3 set nlost_] pkt, rcv [$rcvr3 set npkts_]"
        puts "lost [$rcvr4 set nlost_] pkt, rcv [$rcvr4 set npkts_]"
        puts "lost [$rcvr5 set nlost_] pkt, rcv [$rcvr5 set npkts_]"

        exit 0
}

$ns run

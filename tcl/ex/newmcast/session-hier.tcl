# A simple example for hierarchical routing, by generating
# topology by hand (of 10 nodes)

set ns [new SessionSim]
$ns set-address-format hierarchical

$ns namtrace-all [open session-hier.nam w]

AddrParams set domain_num_ 2
lappend cluster_num 2 2
AddrParams set cluster_num_ $cluster_num
lappend eilastlevel 2 3 2 3
AddrParams set nodes_num_ $eilastlevel

set naddr {0.0.0 0.0.1 0.1.0 0.1.1 0.1.2 1.0.0 1.0.1 1.1.0 1.1.1 1.1.2}

for {set i 0} {$i < 10} {incr i} {
     set n($i) [$ns node [lindex $naddr $i]]
}

$ns duplex-link $n(0) $n(1) 5Mb 2ms DropTail
$ns duplex-link $n(1) $n(2) 5Mb 2ms DropTail
$ns duplex-link $n(2) $n(3) 5Mb 2ms DropTail
$ns duplex-link $n(2) $n(4) 5Mb 2ms DropTail
$ns duplex-link $n(1) $n(5) 5Mb 2ms DropTail
$ns duplex-link $n(5) $n(6) 5Mb 2ms DropTail
$ns duplex-link $n(6) $n(7) 5Mb 2ms DropTail
$ns duplex-link $n(7) $n(8) 5Mb 2ms DropTail
$ns duplex-link $n(8) $n(9) 5Mb 2ms DropTail

set cbr0 [new Agent/CBR]
$ns attach-agent $n(0) $cbr0
$cbr0 set dst_ 0x8002
$ns create-session $n(0) $cbr0

set rcvr0 [new Agent/LossMonitor]
set rcvr1 [new Agent/LossMonitor]
set rcvr2 [new Agent/LossMonitor]
set rcvr3 [new Agent/LossMonitor]
set rcvr4 [new Agent/LossMonitor]
set rcvr5 [new Agent/LossMonitor]
set rcvr6 [new Agent/LossMonitor]
set rcvr7 [new Agent/LossMonitor]
set rcvr8 [new Agent/LossMonitor]
set rcvr9 [new Agent/LossMonitor]
$ns attach-agent $n(0) $rcvr0
$ns attach-agent $n(1) $rcvr1
$ns attach-agent $n(2) $rcvr2
$ns attach-agent $n(3) $rcvr3
$ns attach-agent $n(4) $rcvr4
$ns attach-agent $n(5) $rcvr5
$ns attach-agent $n(6) $rcvr6
$ns attach-agent $n(7) $rcvr7
$ns attach-agent $n(8) $rcvr8
$ns attach-agent $n(9) $rcvr9

$ns at 0.2 "$n(0) join-group $rcvr0 0x8002"
$ns at 0.2 "$n(1) join-group $rcvr1 0x8002"
$ns at 0.2 "$n(2) join-group $rcvr2 0x8002"
$ns at 0.2 "$n(3) join-group $rcvr3 0x8002"
$ns at 0.2 "$n(4) join-group $rcvr4 0x8002"
$ns at 0.2 "$n(5) join-group $rcvr5 0x8002" 
$ns at 0.2 "$n(6) join-group $rcvr6 0x8002"
$ns at 0.2 "$n(7) join-group $rcvr7 0x8002"
$ns at 0.2 "$n(8) join-group $rcvr8 0x8002"
$ns at 0.2 "$n(9) join-group $rcvr9 0x8002" 
$ns at 1.0 "$cbr0 start"

puts [$cbr0 set packetSize_]
puts [$cbr0 set interval_]

$ns at 3.0 "finish"

proc finish {} {
        global rcvr3 rcvr4 rcvr5 rcvr0 rcvr1 rcvr2 rcvr6 rcvr7 rcvr8 rcvr9
        puts "lost [$rcvr0 set nlost_] pkt, rcv [$rcvr0 set npkts_]"
        puts "lost [$rcvr1 set nlost_] pkt, rcv [$rcvr1 set npkts_]"
        puts "lost [$rcvr2 set nlost_] pkt, rcv [$rcvr2 set npkts_]"
        puts "lost [$rcvr3 set nlost_] pkt, rcv [$rcvr3 set npkts_]"
        puts "lost [$rcvr4 set nlost_] pkt, rcv [$rcvr4 set npkts_]"
        puts "lost [$rcvr5 set nlost_] pkt, rcv [$rcvr5 set npkts_]"
        puts "lost [$rcvr6 set nlost_] pkt, rcv [$rcvr6 set npkts_]"
        puts "lost [$rcvr7 set nlost_] pkt, rcv [$rcvr7 set npkts_]"
        puts "lost [$rcvr8 set nlost_] pkt, rcv [$rcvr8 set npkts_]"
        puts "lost [$rcvr9 set nlost_] pkt, rcv [$rcvr9 set npkts_]"
        exit 0
}

$ns run



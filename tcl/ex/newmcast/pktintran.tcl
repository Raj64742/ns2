set ns [new Simulator]
Simulator set EnableMcast_ 1

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
set n4 [$ns node]
set n5 [$ns node]

$ns color 2 black
$ns color 1 blue
$ns color 0 yellow
# prune/graft packets
$ns color 30 purple
$ns color 31 green

set f [open out-mc2.tr w]
$ns trace-all $f
set nf [open out-mc2.nam w]
$ns namtrace-all $nf

Simulator set NumberInterfaces_ 1
$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n0 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n3 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n4 1.5Mb 10ms DropTail
$ns duplex-link $n2 $n4 1.5Mb 10ms DropTail
$ns duplex-link $n2 $n5 1.5Mb 10ms DropTail

$ns duplex-link-op $n0 $n1 orient left-down
$ns duplex-link-op $n0 $n2 orient down-right
$ns duplex-link-op $n1 $n3 orient left-down
$ns duplex-link-op $n1 $n4 orient right-down
$ns duplex-link-op $n2 $n4 orient left-down
$ns duplex-link-op $n2 $n5 orient right-down

$ns rtproto Session
### Start multicast configuration
DM set PruneTimeout 0.3
dynamicDM set ReportRouteTimeout 0.15

set mproto CtrMcast
set mrthandle [$ns mrtproto $mproto  {}]
### End of multicast  config

set cbr0 [new Agent/CBR]
$ns attach-agent $n0 $cbr0
$cbr0 set dst_ 0x8002
 
set rcvr [new Agent/LossMonitor]
$ns attach-agent $n3 $rcvr
$ns attach-agent $n4 $rcvr
$ns attach-agent $n5 $rcvr

$ns at 0.2 "$n3 join-group $rcvr 0x8002"
$ns at 0.4 "$n4 join-group $rcvr 0x8002"
$ns at 0.6 "$n3 leave-group $rcvr 0x8002"
$ns at 0.7 "$n5 join-group $rcvr 0x8002"
$ns at 0.8 "$n3 join-group $rcvr 0x8002"

#### Link between n0 & n1 down at 1.0, up at 1.2
$ns rtmodel-at 1.0 down $n0 $n1
$ns rtmodel-at 1.2 up $n0 $n1
####
set mmonitor [$ns McastMonitor]
$ns at 0.0 "$mmonitor trace-topo 0 [expr 0x8002]"

$ns at 0.1 "$cbr0 start"
 
$ns at 1.6 "finish"

proc finish {} {
        #puts "converting output to nam format..."
        global ns
        $ns flush-trace
        global rcvr
        # puts "lost [$rcvr set nlost_] pkt, rcv [$rcvr set npkts_]"

        puts "running nam..."
        exec nam out-mc2 &
        exit 0
}

$ns run

## Simple Shared Tree multicast test
##
## joining & pruning.
#  |0|     |4|
#   |       |
#  |1|-----|2|
#   |       |
#  |3|     |5|

set ns [new Simulator -multicast on]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
set n4 [$ns node]
set n5 [$ns node]

$ns color 0 blue      ;#cbrs
$ns color 30 purple   ;#grafts
$ns color 31 green    ;#prunes
$ns color 32 orange   ;#encapsulated packets
$ns color 35 red      ;#native for the other group

$n2 color blue        ;#RP
$n3 color red

set f [open out-mc4.tr w]
$ns trace-all $f
set nf [open out-mc4.nam w]
$ns namtrace-all $nf


$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n3 1.5Mb 10ms DropTail
$ns duplex-link $n2 $n4 1.5Mb 10ms DropTail
$ns duplex-link $n2 $n5 1.5Mb 10ms DropTail


$ns duplex-link-op $n0 $n1 orient right-down
$ns duplex-link-op $n3 $n1 orient right-up
$ns duplex-link-op $n1 $n2 orient right
$ns duplex-link-op $n2 $n4 orient right-up
$ns duplex-link-op $n2 $n5 orient right-down

$ns duplex-link-op $n1 $n2 queuePos 0.5
$ns duplex-link-op $n2 $n1 queuePos 0.5
$ns duplex-link-op $n3 $n1 queuePos 0.5
$ns duplex-link-op $n5 $n2 queuePos 0.5

set cbr0 [new Agent/CBR]
set cbr3 [new Agent/CBR]
set cbr4 [new Agent/CBR]

$cbr0 set dst_ 0x8003
$cbr3 set dst_ 0x8003
$cbr4 set dst_ 0x8004
$cbr4 set class_ 35
#
$cbr0 set interval_ 20ms
$cbr3 set interval_ 30ms
$cbr4 set interval_ 40ms

$ns attach-agent $n0 $cbr0
$ns attach-agent $n3 $cbr3
$ns attach-agent $n4 $cbr4
 
set rcvr0 [new Agent/Null]
$ns attach-agent $n0 $rcvr0
set rcvr3 [new Agent/Null]
$ns attach-agent $n3 $rcvr3
set rcvr4  [new Agent/Null]
$ns attach-agent $n4 $rcvr4
set rcvr5 [new Agent/Null]
$ns attach-agent $n5 $rcvr5


#$ns simplex-connect $encap0 $decap2

### Start multicast configuration: 
source ~/vint/ns-main/tcl/mcast/ST.tcl

ST set RP_(0x8003) $n2
ST set RP_(0x8004) $n3

set mproto ST
set mrthandle [$ns mrtproto $mproto {}]

### End of multicast configuration


$ns at 0    "$cbr0 start"
$ns at 0    "$cbr4 start"
$ns at 0.05 "$cbr3 start"
$ns at 0.1  "$n3 join-group-source   $rcvr3 0x8003 *"
$ns at 0.15 "$n4 join-group-source   $rcvr4 0x8004 *"
$ns at 0.2  "$n4 join-group-source   $rcvr4 0x8003 *"
$ns at 0.3  "$n0 join-group-source   $rcvr0 0x8003 *"
$ns at 0.4  "$n3 leave-group-source  $rcvr3 0x8003 *"
$ns at 0.42 "$n0 leave-group-source  $rcvr0 0x8003 *"
$ns at 0.45 "$n5 join-group-source   $rcvr5 0x8003 *"
$ns at 0.5  "$n5 leave-group-source  $rcvr5 0x8003 *"
$ns at 0.52 "$n4 leave-group-source  $rcvr4 0x8003 *"
$ns at 0.55 "finish"

proc finish {} {
        global ns
        $ns flush-trace

        puts "running nam..."
        exec nam out-mc4 &
        exit 0
}

$ns run






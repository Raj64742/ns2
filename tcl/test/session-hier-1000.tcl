# test for memory usage for 1000 node hier topology using session sim

set ns [new SessionSim]
$ns set-address-format hierarchical
source ./hts1000.tcl
set linkBW 5Mb
global n ns

$ns namtrace-all [open session-hier-1000.nam w]

create-hier-topology $linkBW

set cbr0 [new Agent/CBR]
$ns attach-agent $n(0) $cbr0

$ns at 1.0 "$cbr0 start"

$ns at 3.0 "finish"
proc finish {} {
    global ns
    $ns flush-trace
    puts "running nam..."
    exec nam out.nam &
    exit 0
}

$ns run

# test for memory usage for 1000 node hier topology using session sim

set ns [new SessionSim]
$ns set-address-format hierarchical
source ./hts1000.tcl
set linkBW 5Mb
global n ns

$ns namtrace-all [open session-hier-1000.nam w]

create-hier-topology $linkBW

$ns at 3.0 "finish"
proc finish {} {
    global ns
    $ns flush-trace
    puts "running nam..."
    exec nam out.nam &
    exit 0
}

$ns run

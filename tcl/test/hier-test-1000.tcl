# hier-test-1000.tcl
# Test for hier routing using topology generated by GaTech topology generator
# (1000 node transit-stub graph converted to hierarchical topology


set ns [new Simulator]
$ns set-address-format hierarchical
source ./hts1000.tcl
set linkBW 5Mb
global n ns

#$ns namtrace-all [open hier-out-1000.nam w]
#$ns trace-all [open hier-out-1000.tr w]

create-hier-topology $linkBW


set udp0 [new Agent/UDP]
$ns attach-agent $n(0) $udp0
set cbr0 [new Application/Traffic/CBR]
$cbr0 attach-agent $udp0

set udp1 [new Agent/UDP]
$ns attach-agent $n(500) $udp1
$udp1 set class_ 1
set cbr1 [new Application/Traffic/CBR]
$cbr1 attach-agent $udp1

set null0 [new Agent/Null]
$ns attach-agent $n(10) $null0

set null1 [new Agent/Null]
$ns attach-agent $n(600) $null1

$ns connect $udp0 $null0
$ns connect $udp1 $null1

$ns at 1.0 "$cbr0 start"
$ns at 1.1 "$cbr1 start"

set tcp [new Agent/TCP]
$tcp set class_ 2
set sink [new Agent/TCPSink]
$ns attach-agent $n(0) $tcp
$ns attach-agent $n(5) $sink
$ns connect $tcp $sink
set ftp [new Source/FTP]
$ftp set agent_ $tcp
$ns at 1.2 "$ftp start"
$ns at 3.0 "finish"

puts [$cbr0 set packet_size_]
puts [$cbr0 set interval_]

$ns at 3.0 "finish"

proc finish {} {
    global ns 
    $ns flush-trace
    puts "running nam..."
    exec nam out.nam &
    exit 0
}

$ns run





set ns [new Simulator]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]

set f [open out.tr w]
$ns trace-all $f

$ns duplex-link $n0 $n2 5Mb 2ms DropTail
$ns duplex-link $n1 $n2 5Mb 2ms DropTail

$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns queue-limit $n0 $n1 2

set cbr0 [new Agent/CBR]
$cbr0 set interval_ 1ms
$ns attach-agent $n0 $cbr0

set null1 [new Agent/Null]
$ns attach-agent $n1 $null1

$ns connect $cbr0 $null1

$ns rtmodel Deterministic {} $n0 $n1

proc finish {} {
	puts "converting output to nam format..."
	exec awk -f ../nam-demo/nstonam.awk out.tr > dynamic-nam.tr 
	exec rm -f out
	#XXX
	puts "running nam..."
	exec nam dynamic-nam &
	exit 0
}

$ns at 1.0 "$cbr0 start"
$ns at 8.0 "finish"
puts [$cbr0 set packetSize_]
puts [$cbr0 set interval_]
$ns run Session


set ns [new Simulator]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

puts n0=[$n0 id]
puts n1=[$n1 id]
puts n2=[$n2 id]
puts n3=[$n3 id]

set f [open out.tr w]
$ns trace-all $f

#$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n0 $n2 10Mb 2ms DropTail
$ns duplex-link $n1 $n2 10Mb 2ms DropTail
$ns duplex-link $n2 $n3 1.5Mb 10ms DropTail

set cbr0 [new Agent/CBR]
$ns attach-agent $n0 $cbr0

set cbr1 [new Agent/CBR]
$ns attach-agent $n3 $cbr1
$cbr1 set cls 1

set null0 [new Agent/Null]
$ns attach-agent $n3 $null0

set null1 [new Agent/Null]
$ns attach-agent $n1 $null1

$ns connect $cbr0 $null0
$ns connect $cbr1 $null1

$ns at 1.0 "$cbr0 start"
$ns at 1.1 "$cbr1 start"

set tcp [new Agent/TCP]
$tcp set cls 2
set sink [new Agent/TCPSink]
$ns attach-agent $n0 $tcp
$ns attach-agent $n3 $sink
$ns connect $tcp $sink
set ftp [new Source/FTP]
$ftp set agent $tcp
$ns at 1.2 "$ftp start"

puts [$cbr0 set packetSize_]
puts [$cbr0 set interval_]

$ns at 2.0 "finish"

proc finish {} {
	puts "converting output to nam format..."
	exec awk -f ../nam-demo/nstonam.awk out.tr > simple-nam.tr 
	exec rm -f out
	#XXX
	puts "running nam..."
	exec nam simple-nam &
	exit 0
}

$ns run


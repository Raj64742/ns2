
set ns [new Simulator]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out.tr w]
$ns trace-all $f

$ns duplex-link $n0 $n1 1.5Mb 10ms drop-tail
$ns duplex-link $n0 $n2 1.5Mb 10ms drop-tail
$ns duplex-link $n1 $n2 1.5Mb 10ms drop-tail
$ns duplex-link $n2 $n3 1.5Mb 10ms drop-tail

set cbr0 [new agent/cbr]
$ns attach-agent $n0 $cbr0

set cbr1 [new agent/cbr]
$ns attach-agent $n3 $cbr1
$cbr1 set cls 1

set null0 [new agent/null]
$ns attach-agent $n3 $null0

set null1 [new agent/null]
$ns attach-agent $n1 $null1

$ns connect $cbr0 $null0
$ns connect $cbr1 $null1

$ns at 1.0 "$cbr0 start"
$ns at 1.1 "$cbr1 start"

set tcp [new agent/tcp]
$tcp set cls 2
set sink [new agent/tcp-sink]
$ns attach-agent $n0 $tcp
$ns attach-agent $n3 $sink
$ns connect $tcp $sink
set ftp [new source/ftp]
$ftp set agent $tcp
$ns at 1.2 "$ftp start"

puts [$cbr0 set packet-size]
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


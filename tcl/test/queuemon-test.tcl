source ns-queue.tcl
set ns [new Simulator]

set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out.tr w]
$ns trace-all $f

$ns duplex-link $n1 $n2 500kb 2ms DropTail
$ns duplex-link $n2 $n3 1Mb 10ms DropTail

set cbr1 [new Agent/CBR]
$cbr1 set packetSize 1024
$ns attach-agent $n1 $cbr1

set null1 [new Agent/Null]
$ns attach-agent $n3 $null1

$ns connect $cbr1 $null1

$ns at 0.0 "$cbr1 start"

set tcp [new Agent/TCP]
$tcp set class_ 1
$tcp set packetSize_ 1024
set sink [new Agent/TCPSink]
$ns attach-agent $n1 $tcp
$ns attach-agent $n3 $sink
$ns connect $tcp $sink
set ftp [new Source/FTP]
$ftp set agent_ $tcp
$ns at 2.0 "$ftp start"

$ns at 20.0 "finish"

proc do_nam {} {
	puts "converting output to nam format..."
	exec awk -f ../nam-demo/nstonam.awk out.tr > simple-nam.tr 
	exec rm -f out.tr
	puts "running nam..."
	exec nam simple-nam &
}

proc finish {} {
	global ns f
	$ns flush-trace
	close $f
	do_nam
	exit 0
}

$ns monitor-queue $n1 $n2
set l12 [$n1 get-link $n2]
$l12 set qBytesEstimate_ 0
$l12 set qPktsEstimate_ 0
set queueSampleInterval 0.5
$ns at [$ns now] "$l12 queue-sample-timeout"

$ns run

# This is a example file for C++ implementation of srm
# This is a example file for C++ implementation of srm

set ns [new MultiSim]

set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
set n4 [$ns node]
set n5 [$ns node]
set n6 [$ns node]

set dm1 [new DM $ns $n1]
set dm2 [new DM $ns $n2]
set dm3 [new DM $ns $n3]
set dm4 [new DM $ns $n4]
set dm5 [new DM $ns $n5]
set dm6 [new DM $ns $n6]


$ns at 0.0 "$ns run-mcast"



set f [open srm-out.tr w]
$ns trace-all $f

$ns duplex-link $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n2 $n3 1.5Mb 10ms DropTail
$ns duplex-link $n3 $n4 1.5Mb 10ms DropTail
$ns duplex-link $n4 $n5 1.5Mb 10ms DropTail
$ns duplex-link $n4 $n6 1.5Mb 10ms DropTail

set s1 [new Agent/SRM]
set s2 [new Agent/SRM]
set s3 [new Agent/SRM]
set s4 [new Agent/SRM]
set s5 [new Agent/SRM]
set s6 [new Agent/SRM]

$s1 attach-to $n1
$s2 attach-to $n2
$s3 attach-to $n3
$s4 attach-to $n4
$s5 attach-to $n5
$s6 attach-to $n6


$ns at 1.0 "$s1 join-group 0x8000"
$ns at 1.0 "$s1 start"
$ns at 6.0 "$s1 sender"

$ns at 1.0 "$s2 join-group 0x8000"
$ns at 1.0 "$s2 start"

$ns at 1.0 "$s3 join-group 0x8000"
$ns at 1.0 "$s3 start"

$ns at 1.0 "$s4 join-group 0x8000"
$ns at 1.0 "$s4 start"

$ns at 1.0 "$s5 join-group 0x8000"
$ns at 1.0 "$s5 start"

$ns at 1.0 "$s6 join-group 0x8000"
$ns at 1.0 "$s6 start"


$ns at 20.0 "finish"

proc finish {} {
#	puts "converting output to nam format..."
	global ns
	$ns flush-trace
	exec awk -f ../nam-demo/nstonam.awk srm-out.tr > srm2-nam.tr
#	exec rm -f out
	puts "running nam..."
	exec nam srm2-nam &
	exit 0
}

$ns run

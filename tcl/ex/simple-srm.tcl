# This is a example file for C++ implementation of srm
# This is a example file for C++ implementation of srm

set ns [new MultiSim]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
 
set f [open srm-out.tr w]
set f1 [open srm-local.tr w]
$ns trace-all $f

$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n3 1.5Mb 10ms DropTail

set dm0 [new DM $ns $n0]
set dm1 [new DM $ns $n1]
set dm2 [new DM $ns $n2]
set dm3 [new DM $ns $n3]


$ns at 0.0 "$ns run-mcast"

set s0 [new Agent/SRM]
set s2 [new Agent/SRM]
set s3 [new Agent/SRM]


$s0 attach-to $n0
$s2 attach-to $n2
$s3 attach-to $n3


#set exp1 [new Traffic/Expoo]
#$exp1 set packet-size 1024
#$exp1 set burst-time 500ms
#$exp1 set idle-time 500ms
#$exp1 set rate 100k

#$s0 attach-traffic $exp1


$ns at 1.4 "$s0 join-group 0x8000"
$ns at 1.5 "$s0 start"
#$ns at 1.6 "$s0 sender"
#$ns at 6.0 "$s0 set seqno 2"
#

$ns at 1.5 "$s2 join-group 0x8000"
$ns at 1.6 "$s2 start"


$ns at 1.6 "$s3 join-group 0x8000"
$ns at 1.7 "$s3 start"

$ns at 20.0 "finish"

proc finish {} {
	puts "converting output to nam format..."
	global ns
	$ns flush-trace
	exec awk -f ../nam-demo/nstonam.awk srm-out.tr > srm-nam.tr
#	exec rm -f out
        #XXX
	puts "running nam..."
	exec nam srm-nam &
	exit 0
}

$ns run






set ns [new MultiSim]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open rtp-out.tr w]
$ns trace-all $f

$ns duplex-link $n0 $n1 1.5Mb 10ms drop-tail
$ns duplex-link $n1 $n2 1.5Mb 10ms drop-tail
$ns duplex-link $n1 $n3 1.5Mb 10ms drop-tail

set s0 [new Session/RTP]
set s1 [new Session/RTP]
set s2 [new Session/RTP]
set s3 [new Session/RTP]

$s0 tx-bw 400kb/s
$s1 tx-bw 400kb/s
$s2 tx-bw 400kb/s
$s3 tx-bw 400kb/s

$s0 attach-to $n0
$s1 attach-to $n1
$s2 attach-to $n2
$s3 attach-to $n3

$ns at 1.4 "$s0 join-group 0x8000"
$ns at 1.5 "$s0 start"
$ns at 1.6 "$s0 transmit"

$ns at 1.7 "$s1 join-group 0x8000"
$ns at 1.8 "$s1 start"
$ns at 1.9 "$s1 transmit"

$ns at 2.0 "$s2 join-group 0x8000"
$ns at 2.1 "$s2 start"
$ns at 2.2 "$s2 transmit"

$ns at 2.3 "$s3 join-group 0x8000"
$ns at 2.4 "$s3 start"
$ns at 2.5 "$s3 transmit"

$ns at 4.0 "finish"

proc finish {} {
	puts "converting output to nam format..."
	global ns
	$ns flush-trace
	exec awk -f ../nam-demo/nstonam.awk rtp-out.tr > rtp-nam.tr
	exec rm -f out
        #XXX
	puts "running nam..."
	exec /usr/src/mash/nam/nam rtp-nam &
	exit 0
}

$ns run

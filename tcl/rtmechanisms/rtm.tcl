#
# simulator for router mechanisms
#

source mechanisms.tcl

#
# create:
#
# S1		 S3
#   \		 /
#    \		/
#    R1========R2
#    /		\
#   /		 \
# S2		 S4
#
# - 10Mb/s, 3ms, drop-tail
# = 1.5Mb/s, 20ms, CBQ
#
proc create_topology tf {
	global ns
	global s1 s2 s3 s4
	global r1 r2

	set s1 [$ns node]
	set s2 [$ns node]
	set s3 [$ns node]
	set s4 [$ns node]

	set r1 [$ns node]
	set r2 [$ns node]

	$ns duplex-link $s1 $r1 10Mb 2ms DropTail
	$ns duplex-link $s2 $r1 10Mb 3ms DropTail
	set cl [new Classifier/Hash/SrcDestFid 33]
	$ns simplex-link $r1 $r2 1.5Mb 20ms "CBQ $cl"
	set cbqlink [$ns link $r1 $r2]
	$ns simplex-link $r2 $r1 1.5Mb 20ms RED
	[[$ns link $r2 $r1] queue] set limit_ 25
	$ns duplex-link $s3 $r2 10Mb 4ms DropTail
	$ns duplex-link $s4 $r2 10Mb 5ms DropTail

#	$ns trace-queue $n2 $n3 $tf
	return $cbqlink
}

proc create_source { src sink fid } {
	global ns

	set cbr1s [new Agent/CBR]
	$cbr1s set fid_ $fid
	$ns attach-agent $src $cbr1s

	set cbr1r [new Agent/LossMonitor]
	$ns attach-agent $sink $cbr1r
	$ns connect $cbr1s $cbr1r
	return $cbr1s
}

proc create_tcp_source { src sink fid win pktsize } {
	global ns

	set srctype TCP/Reno
	set sinktype TCPSink
	set tcp [$ns create-connection $srctype $src $sinktype $sink $fid]
	if {$pktsize > 0} {
		$tcp set packetSize_ $pktsize
	}
	if {$win > 0} {
		$tcp set window_ $win
	}
	return [$tcp attach-source FTP]
}
proc printflow f {
	puts "flow $f: epdrops: [$f set epdrops_]; ebdrops: [$f set ebdrops_]; pdrops: [$f set pdrops_]; bdrops: [$f set bdrops_]"
}

proc finish { tf ff } {
	close $tf
	close $ff
	puts "simulation complete"
	exit 0
}

proc sim1 {} {
	global ns

	global s1 s2 s3 s4
	global r1 r2

	puts "starting sim1"

	set start1 1.0
	set start2 1.2
	set start3 1.4
	set endsim 300.0

	set rtt 0.06
	set mtu 512

	set ns [new Simulator]

	set tracef [open out.tr w]
	set cbqlink [create_topology $tracef]

	set rtm [new RTMechanisms $ns $cbqlink $rtt $mtu]

	set gfm [$rtm makeflowmon]
	set gflowf [open gflow.tr w]
	$gfm set enable_in_ false	; # no per-flow arrival state
	$gfm set enable_out_ false	; # no per-flow departure state
	$gfm attach $gflowf

	set bfm [$rtm makeflowmon]
	set bflowf [open bflow.tr w]
	$bfm attach $bflowf

	$rtm makeboxes $gfm $bfm 100 1000
	$rtm bindboxes

	set src1 [create_tcp_source $s1 $s3 1 100 1000]
	set src2 [create_tcp_source $s2 $s4 2 100 50]
	set src3 [create_source $s1 $s4 3]
		$src3 set packetSize_ 190
		$src3 set interval_ 0.003

	$ns at $start1 "$src1 start"
	$ns at $start2 "$src2 start"
	$ns at $start3 "$src3 start"
	$ns at $endsim "finish $gflowf $bflowf"
	$ns run
}
sim1

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
proc create_topology { tf af queuetype }  {
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
	$ns simplex-link $r1 $r2 1.5Mb 20ms $queuetype
	$ns simplex-link $r2 $r1 1.5Mb 20ms DropTail
	set redlink [$ns link $r1 $r2]
	[[$ns link $r2 $r1] queue] set limit_ 25
	$ns duplex-link $s3 $r2 10Mb 4ms DropTail
	$ns duplex-link $s4 $r2 10Mb 5ms DropTail

	$redlink trace $ns $tf	; # trace pkts
	[$ns link $r2 $r1] trace $ns $af ; # trace acks
	return $redlink
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
	global ns
	close $tf
	close $ff
	$ns instvar scheduler_
	$scheduler_ halt
	puts "simulation complete"
}

proc tcpDumpAll { tcpSrc interval label } {
	global ns
        proc dump { src interval label } {
                ns at [expr [ns now] + $interval] "dump $src $interval $label"
                puts time=[$ns now]/class=$label/ack=[$src set ack_]
        }
        puts $label:window=[$tcpSrc set window_]/packet-size=[$tcpSrc set packetSize_]
        $ns at 0.0 "dump $tcpSrc $interval $label"
}


proc new_tcp { startTime source dest window flowid dump size } {
	global ns
	set src [create_tcp_source $source $dest $flowid $window $size]
	$ns at $startTime "$src start"
	if {$dump == 1 } {tcpDumpAll $src 20.0 $flowid }
}

proc makeflowmon {} {
	global ns
        set flowmon [new QueueMonitor/ED/Flowmon]
        set cl [new Classifier/Hash/SrcDestFid 33]

        $cl proc unknown-flow { src dst fid hashbucket } {
                set fdesc [new QueueMonitor/ED/Flow]
                set slot [$self installNext $fdesc]
                $self set-hash $hashbucket $src $dst $fid $slot
        }
        $cl proc no-slot slotnum {
        }
        $flowmon classifier $cl
        return $flowmon
}

proc attach-fmon { lnk fm } {
	global ns
	set isnoop [new SnoopQueue/In]
	set osnoop [new SnoopQueue/Out]
	set dsnoop [new SnoopQueue/Drop]
	$lnk attach-monitors $isnoop $osnoop $dsnoop $fm
        set edsnoop [new SnoopQueue/EDrop]
        $edsnoop set-monitor $fm
        [$lnk queue] early-drop-target $edsnoop
	$edsnoop target [$ns set nullAgent_]
}

proc sim1 {} {
	global ns

	global s1 s2 s3 s4
	global r1 r2
	set queuetype RED

	puts "starting sim1"

	set endsim 30.0

	set ns [new Simulator]

	set tracef [open out.tr w]
	set ackf [open ack.tr w]
	set redlink [create_topology $tracef $ackf $queuetype]
	if {$queuetype == "RED"} {
		[$redlink queue] set limit_ 100
		new_tcp 10.2 $s1 $s3 100 20 0 1500
		new_tcp 5.2 $s1 $s3 100 21 0 1500
	}
	
	set gfm [makeflowmon]
	set gflowf [open gflow.tr w]
	$gfm attach $gflowf
	attach-fmon $redlink $gfm

	$ns at $endsim "$gfm dump; finish $gflowf $tracef"
	$ns run
}
sim1

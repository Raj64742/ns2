#!../../../../ns
source ../../../lan/ns-lan.tcl

proc createTcpSource { type tcptrace { maxburst 0 } { tcpTick 0.1 } { window 100 } } {
	set tcp0 [new Agent/$type]
	$tcp0 set class_ 1
	$tcp0 set maxburst_ $maxburst
	$tcp0 set tcpTick_ $tcpTick
	$tcp0 set window_ $window
	$tcp0 trace $tcptrace
	return $tcp0
} 

proc createTcpSink { type sinktrace { ackSize 40 } { maxdelack 25 } } {
	set sink0 [new Agent/$type]
	$sink0 set packetSize_ $ackSize
	if {[string first "Asym" $type] != -1} { 
		$sink0 set maxdelack_ $maxdelack
	}
	$sink0 trace $sinktrace
	return $sink0
}

proc createFtp { ns n0 tcp0 n1 sink0 } {
	$ns attach-agent $n0 $tcp0
	$ns attach-agent $n1 $sink0
	$ns connect $tcp0 $sink0
	set ftp0 [new Source/FTP]
	$ftp0 set agent_ $tcp0
	return $ftp0
}

proc configQueue { ns n0 n1 type trace { size 50 } { acksfirst false } { filteracks false } { replace_head false } { acksfromfront false } { interleave false } } {
	$ns queue-limit $n0 $n1 $size
	set id0 [$n0 id]
	set id1 [$n1 id]
	set l01 [$ns set link_($id0:$id1)]
	set q01 [$l01 set queue_]
	if {[string first "NonFifo" $type] != -1} {
		$q01 set acksfirst_ $acksfirst
		$q01 set filteracks_ $filteracks
		$q01 set replace_head_ $replace_head
		$q01 set acksfromfront_ $acksfromfront
		$q01 set interleave_ $interleave
	}
	$q01 trace $trace
	$q01 reset
}


# set up simulation

set ns [new Simulator]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out.tr w]
$ns trace-all $f

set tcptrace [open tcp.tr w] 
set sinktrace [open sink.tr w]
set redtrace [open red.tr w]
set graph 0
set maxdelack 25
set maxburst 3
set upwin 0
set tcpTick 0.1
set ackSize 40
set rbw 28.8Kb
set rqsize 10
set qsize 10
set rgw "DropTail" 
set acksfirst false
set filteracks false
set replace_head false
set acksfromfront false
set interleave false
set midtime 0

# configure RED parameters
Queue/RED set setbit_ true
Queue/RED set drop-tail_ true
Queue/RED set fracthresh_ true
Queue/RED set fracminthresh_ 0.4
Queue/RED set fracmaxthresh_ 0.7
Queue/RED set q_weight_ 0.25
Queue/RED set wait_ false
Queue/RED set linterm_ 1

Queue set interleave_ false
Queue set acksfirst_ false
Queue set filteracks_ false
Queue set replace_head_ false
Queue set ackfromfront_ false

Agent/TCP set disable_ecn_ 0

proc printUsage {} {
	puts "Usage: "
	exit
}

proc finish {ns traceall tcptrace graph midtime} {
	$ns flush-trace
	close $traceall
	flush $tcptrace
	close $tcptrace
	plotgraph $graph $midtime
	exit 0
}	

proc plotgraph {graph midtime} {
	exec gawk --lint -f ../../../ex/asym/seq.awk out.tr
	exec gawk --lint -v mid=$midtime -f ../../../ex/asym/tcp.awk tcp.tr
	if {$graph == 0} {
		return
	}

        set if [open index.out r]
	while {[gets $if i] >= 0} {
		set seqfile [format "seq-%s.out" $i]
		set ackfile [format "ack-%s.out" $i]
		set cwndfile [format "cwnd-%s.out" $i]
		set ssthreshfile [format "ssthresh-%s.out" $i]
		set srttfile [format "srtt-%s.out" $i]
		set rttvarfile [format "rttvar-%s.out" $i]
		exec xgraph -display joyride:0.1 -bb -tk -m -x time -y seqno $seqfile $ackfile &
		exec xgraph -display joyride:0.1 -bb -tk -m -x time -y window $cwndfile &
	}
	close $if
}

# read user options and act accordingly
set count 0
while {$count < $argc} {
	set arg [lindex $argv $count]
	incr count 2
	switch -exact '$arg' {
		'-graph' {
			set graph 1
			incr count -1
			continue
		}
		'-mda' {
			set maxdelack [lindex $argv [expr $count-1]]
			continue
		}
		'-acksz' {
			set ackSize [lindex $argv [expr $count-1]]
			continue
		}
		'-mb' {
			set maxburst [lindex $argv [expr $count-1]]
			continue
		}
		'-upwin' {
			set upwin [lindex $argv [expr $count-1]]
			continue
		}
		'-tk' {
			set tcpTick [lindex $argv [expr $count-1]]
			continue
		}
		'-rbw' {
			set rbw [lindex $argv [expr $count-1]]
			continue
		}
		'-rq' {
			set rqsize [lindex $argv [expr $count-1]]
			continue
		}
		'-q' {
			set qsize [lindex $argv [expr $count-1]]
			Queue set limit_ $qsize 
			continue
		}
		'-nonfifo' {
			set rgw "DropTail/NonFifo"
			incr count -1
			continue
		}
		'-noecn' {
			Agent/TCP set disable_ecn_ 1
			incr count -1
			continue
		}
		'-red' {
			set rgw "RED"
			incr count -1
			continue
		}
		'-red_nonfifo' {
			set rgw "RED/NonFifo"
			incr count -1
			continue
		}
		'-acksfirst' {
			set acksfirst true
			incr count -1
			continue
		}
		'-filteracks' {
			set filteracks true
			incr count -1
			continue
		}
		'-replace_head' {
			set replace_head true
			incr count -1
			continue
		}
		'-mid' {
			set midtime [lindex $argv [expr $count-1]]
			continue
		}
		default {
			incr count -2
		} 
	}
	switch -exact '$arg' {
		'-dn' {
			set direction "down"
		}
		'-up' {
			set direction "up"
		}
		default {
			puts "arg $arg"
			printUsage
			exit
		}
	}
	incr count 1
	set arg [lindex $argv $count]
	switch -exact '$arg' {
		'-asym' {
			set type "asym"
		}
		'-asymsrc' {
			set type "asymsrc"
		}
		'-reno' {
			set type "reno"
		}
		default {
			puts "arg $arg"
			printUsage
			exit
		}
	}
	incr count 1
	set arg [lindex $argv $count]
	set startTime $arg
	incr count 1

	if { $direction == "down" } {
		set src $n0
		set dst $n3
	} elseif { $direction == "up" } {
		set src $n3
		set dst $n0
	} else {
	}
	
	if { $type == "asym" } {
		set tcp [createTcpSource "TCP/Reno/Asym" $tcptrace $maxburst $tcpTick]
		set sink [createTcpSink "TCPSink/Asym" $sinktrace $ackSize $maxdelack]
	} elseif { $type == "asymsrc" } {
		set tcp [createTcpSource "TCP/Reno/Asym" $tcptrace $maxburst $tcpTick]
		set sink [createTcpSink "TCPSink" $sinktrace $ackSize]
	} elseif { $type == "reno" } {
		set tcp [createTcpSource "TCP/Reno" $tcptrace $tcpTick]
		set sink [createTcpSink "TCPSink" $sinktrace $ackSize]
	}
	
	if { $direction == "up" && $upwin != 0 } {
		$tcp set window_ $upwin
	}
	set ftp [createFtp $ns $src $tcp $dst $sink]
	$ns at $startTime "$ftp start" 
}

# topology
#
#      10Mb, 1ms       10Mb, 5ms       10Mb, 1ms
#  n0 ------------ n1 ------------ n2 ------------ n3
#                     28.8Kb, 50ms 
#
$ns duplex-link $n0 $n1 10Mb 1ms DropTail
$ns simplex-link $n1 $n2 10Mb 5ms DropTail
$ns simplex-link $n2 $n1 $rbw 50ms $rgw
$ns duplex-link $n2 $n3 10Mb 1ms DropTail

# configure reverse bottleneck queue
configQueue $ns $n2 $n1 $rgw $redtrace $rqsize $acksfirst $filteracks $replace_head

# end simulation
$ns at 30.0 "finish $ns $f $tcptrace $graph $midtime"

$ns run
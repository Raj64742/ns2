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
Queue set priority_drop_ false
Queue set random_drop_ false

Agent/TCP set disable_ecn_ 0
Agent/TCP/Fack set ss-div4 0
Agent/TCP/Fack set rampdown 0

proc plotgraph {graph midtime { qtraceflag false } } {
	global env

	exec gawk --lint -f ../../../ex/asym/seq.awk out.tr
	exec gawk --lint -v mid=$midtime -f ../../../ex/asym/tcp.awk tcp.tr
	exec gawk --lint -f ../../../ex/asym/tcp-burst.awk tcp.tr
	if { $qtraceflag } {
		exec gawk --lint -f ../../../ex/asym/queue.awk q.tr
	}

	if {$graph} {
		set if [open index.out r]
		while {[gets $if i] >= 0} {
			set seqfile [format "seq-%s.out" $i]
			set ackfile [format "ack-%s.out" $i]
			set cwndfile [format "cwnd-%s.out" $i]
			set ssthreshfile [format "ssthresh-%s.out" $i]
			set srttfile [format "srtt-%s.out" $i]
			set rttvarfile [format "rttvar-%s.out" $i]
			exec xgraph -display $env(DISPLAY) -bb -tk -m -x time -y seqno $seqfile $ackfile &
			exec xgraph -display $env(DISPLAY) -bb -tk -m -x time -y window $cwndfile &
		}
		close $if
	}

	if { $qtraceflag } {
		set qif [open qindex.out r]
		while {[gets $qif i] >= 0} {
			exec xgraph -display $env(DISPLAY) -bb -tk -m -x time -y "# packets" $i &
		}
		close $qif
	}
}

proc monitor_queue {ns n0 n1 queuetrace sampleInterval} {
	set id0 [$n0 id]
	set id1 [$n1 id]
	set l01 [$ns set link_($id0:$id1)]
	$ns monitor-queue $n0 $n1 $queuetrace $sampleInterval
	$l01 set qBytesEstimate_ 0
	$ns at [$ns now] "$l01 queue-sample-timeout"
}


proc trace_queue {ns n0 n1 queuetrace} {
	set id0 [$n0 id]
	set id1 [$n1 id]
	set l01 [$ns set link_($id0:$id1)]
	$ns monitor-queue $n0 $n1 $queuetrace
	$ns at [$ns now] "$l01 start-tracing"
}


proc createTcpSource { type tcptrace { maxburst 0 } { tcpTick 0.1 } { window 100 } } {
	set tcp0 [new Agent/$type]
	$tcp0 set class_ 1
	$tcp0 set maxburst_ $maxburst
	$tcp0 set tcpTick_ $tcpTick
	$tcp0 set window_ $window
	$tcp0 attach $tcptrace
	$tcp0 trace "t_seqno_" 
	$tcp0 trace "rtt_" 
	$tcp0 trace "srtt_" 
	$tcp0 trace "rttvar_" 
	$tcp0 trace "backoff_" 
	$tcp0 trace "dupacks_" 
	$tcp0 trace "ack_" 
	$tcp0 trace "cwnd_" 
	$tcp0 trace "ssthresh_" 
	$tcp0 trace "maxseq_" 
	return $tcp0
} 

proc createTcpSink { type sinktrace { ackSize 40 } { maxdelack 25 } } {
	set sink0 [new Agent/$type]
	$sink0 set packetSize_ $ackSize
	if {[string first "Asym" $type] != -1} { 
		$sink0 set maxdelack_ $maxdelack
	}
	$sink0 attach $sinktrace
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

proc configQueue { ns n0 n1 type trace { size -1 } { acksfirst false } { filteracks false } { replace_head false } { acksfromfront false } { interleave false } } {
	if { $size >= 0 } {
		$ns queue-limit $n0 $n1 $size
	}
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
	if {[string first "RED" $type] != -1} {
		configREDQueue $ns $n0 $n1 [$q01 set q_weight_] 1
	}
#	$q01 trace $trace
	$q01 reset
}

proc configREDQueue { ns n0 n1 { q_weight -1 } { fracthresh 0 } { fracminthresh 0.4 } { fracmaxthresh 0.8} } {
	set id0 [$n0 id]
	set id1 [$n1 id]
	set l01 [$ns set link_($id0:$id1)]
	set q01 [$l01 set queue_]
	if {$q_weight >= 0} {
		$q01 set q_weight_ $q_weight
	}
	if { $fracthresh } {
		set lim [$q01 set limit_]
		$q01 set thresh_ [expr $fracminthresh*$lim]
		$q01 set maxthresh_ [expr $fracmaxthresh*$lim]
	}
}

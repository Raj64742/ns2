# configure RED parameters
Queue/RED set setbit_ true
Queue/RED set drop-tail_ false
Queue/RED set fracthresh_ true
Queue/RED set fracminthresh_ 0.15
Queue/RED set fracmaxthresh_ 0.6
Queue/RED set q_weight_ 0.01
Queue/RED set wait_ false
Queue/RED set linterm_ 10
#Queue/RED set linterm_ 5

Queue set interleave_ false
Queue set acksfirst_ false
Queue set filteracks_ false
Queue set replace_head_ false
Queue set ackfromfront_ false
Queue set priority_drop_ false
Queue set random_drop_ false

Agent/TCP set disable_ecn_ 0
Agent/TCP set restart_bugfix_ true
Agent/TCP/Fack set ss-div4 0
Agent/TCP/Fack set rampdown 0
Agent/TCP set timestamps_ true
Agent/TCP set fast_loss_recov_ true
Agent/TCP set fast_reset_timer_ true

Agent/TCP/Int set rightEdge_ 0
Agent/TCP/Int set uniqTS_ 1
Agent/TCP/Int set winInc_ 1
Agent/TCP/Int set winMult_ 0.5

Agent/TCP/Session set proxyopt_ 0
Agent/TCP/Session set count_bytes_acked_ false

proc plotgraph {graph connGraphFlag midtime turnontime turnofftime { qtraceflag false } { dir "." } } {
	global env
	upvar $connGraphFlag graphFlag

	exec gawk --lint -v dir=$dir -f ../../../ex/asym/tcp-trace.awk $dir/tcp-raw.tr
	exec gawk --lint -v dir=$dir -f ../../../ex/asym/seq.awk $dir/out.tr
	exec gawk --lint -v dir=$dir -v mid=$midtime -v turnon=$turnontime -v turnoff=$turnofftime -f ../../../ex/asym/tcp.awk $dir/tcp.tr
	exec gawk --lint -v dir=$dir -f ../../../ex/asym/tcp-burst.awk $dir/tcp.tr
	if { $qtraceflag } {
		exec gawk --lint -f ../../../ex/asym/queue.awk $dir/q.tr
	}

	set if [open seq-index.out r]
	while {[gets $if i] >= 0} {
		if {$graph || $graphFlag($i)} {
			set seqfile [format "%s/seq-%s.out" $dir $i]
			set ackfile [format "%s/ack-%s.out" $dir $i]
			exec xgraph -display $env(DISPLAY) -bb -tk -m -x time -y seqno $seqfile $ackfile &
		}
	}
	close $if

	set if [open index.out r]
	while {[gets $if i] >= 0} {
		if {$graph || $graphFlag($i)} {
			set cwndfile [format "%s/cwnd-%s.out" $dir $i]
			set ssthreshfile [format "%s/ssthresh-%s.out" $dir $i]
			set srttfile [format "%s/srtt-%s.out" $dir $i]
			set rttvarfile [format "%s/rttvar-%s.out" $dir $i]
			exec xgraph -display $env(DISPLAY) -bb -tk -m -x time -y window $cwndfile &
		}
	}
	close $if

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


proc createTcpSource { type { maxburst 0 } { tcpTick 0.1 } { window 100 } } {
	set tcp0 [new Agent/$type]
	$tcp0 set class_ 1
	$tcp0 set maxburst_ $maxburst
	$tcp0 set tcpTick_ $tcpTick
	$tcp0 set window_ $window
	$tcp0 set maxcwnd_ $window
	$tcp0 set ecn_ 1
	$tcp0 set g_ 0.125
	return $tcp0
} 

proc enableTcpTracing { tcp tcptrace } {
	$tcp attach $tcptrace
	$tcp trace "t_seqno_" 
	$tcp trace "rtt_" 
	$tcp trace "srtt_" 
	$tcp trace "rttvar_" 
	$tcp trace "backoff_" 
	$tcp trace "dupacks_" 
	$tcp trace "ack_" 
	$tcp trace "cwnd_"
	$tcp trace "ssthresh_" 
	$tcp trace "maxseq_" 
	$tcp trace "seqno_"
	$tcp trace "exact_srtt_"
	$tcp trace "avg_win_"
}

proc setupTcpTracing { tcp tcptrace { sessionFlag false } } {
	enableTcpTracing $tcp $tcptrace
	if { $sessionFlag } {
		set session [[$tcp set node_] createTcpSession [$tcp set dst_]]
		enableTcpTracing $session $tcptrace
	}
}

proc setupGraphing { tcp connGraph connGraphFlag {sessionFlag false} } {
	upvar $connGraphFlag graphFlag

	set saddr [expr [$tcp set addr_]/256]
	set sport [expr [$tcp set addr_]%256]
	set daddr [expr [$tcp set dst_]/256]
	set dport [expr [$tcp set dst_]%256]
	set conn [format "%d,%d-%d,%d" $saddr $sport $daddr $dport]
	set graphFlag($conn) $connGraph

	if { $sessionFlag } {
		set session [[$tcp set node_] createTcpSession [$tcp set dst_]]
		set sport [expr [$session set addr_]%256]
		set dport 0
		set conn [format "%d,%d-%d,%d" $saddr $sport $daddr $dport]
		set graphFlag($conn) $connGraph
	}
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

proc setupTcpSession { tcp { count_bytes_acked false } } {
	if {![[$tcp set node_] existsTcpSession [$tcp set dst_]]} {
		set session [[$tcp set node_] createTcpSession [$tcp set dst_]]
		$session set maxburst_ [$tcp set maxburst_]
		$session set slow_start_restart_ [$tcp set slow_start_restart_]
		$session set restart_bugfix_ [$tcp set restart_bugfix_]
		$session set packetSize_ [$tcp set packetSize_]
		$session set ecn_ [$tcp set ecn_]
		$session set window_ [$tcp set window_]
		$session set maxcwnd_ [$tcp set maxcwnd_]

		$session set count_bytes_acked_ $count_bytes_acked
	}
}
		

proc createFtp { ns n0 tcp0 n1 sink0 } {
	$ns attach-agent $n0 $tcp0
	$ns attach-agent $n1 $sink0
	$ns connect $tcp0 $sink0
	set ftp0 [new Source/FTP]
	$ftp0 set agent_ $tcp0
	return $ftp0
}

proc configQueue { ns n0 n1 type qtrace { size -1 } { nonfifo 0 } { acksfirst false } { filteracks false } { replace_head false } { priority_drop false } { random_drop false } { random_ecn false } { reconsacks false } } { 
# proc configQueue { ns n0 n1 type qtrace rtrace { size -1 } { nonfifo 0 } { acksfirst false } { filteracks false } { replace_head false } { priority_drop false } { random_drop false } { reconsacks false } }  
	if { $size >= 0 } {
		$ns queue-limit $n0 $n1 $size
	}
	set id0 [$n0 id]
	set id1 [$n1 id]
	set l01 [$ns set link_($id0:$id1)]
	set q01 [$l01 set queue_]
	if {$nonfifo} {
		set spq [new PacketQueue/Semantic]
		$spq set acksfirst_ $acksfirst
		$spq set filteracks_ $filteracks
		$spq set replace_head_ $replace_head
		$spq set priority_drop_ $priority_drop
		$spq set random_drop_ $random_drop
		$spq set random_ecn_ $random_ecn
		if { $reconsacks == "true"} {
			set recons [new AckReconsControllerClass]
			# Set queue_ of controller.  This is 
			# used by the individual reconstructors
			$recons set queue_ $q01
			$spq set reconsAcks_ 1
			$spq ackrecons $recons
		}
		$q01 packetqueue-attach $spq
	}
	if {[string first "RED" $type] != -1} {
		configREDQueue $ns $n0 $n1 [$q01 set q_weight_] 1
	}
#	$q01 trace $trace
	if { $qtrace != 0 } {
		trace_queue $ns $n0 $n1 $qtrace
	}
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
	$q01 set setbit_ true
}

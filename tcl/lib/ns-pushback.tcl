
Node instproc add-pushback-agent {} {
	$self instvar pushback_
	set pushback_ [new Agent/Pushback]
	[Simulator instance] attach-agent $self $pushback_
	puts "Pushback Agent = $pushback_"
	$pushback_ initialize $self [[Simulator instance] get-routelogic]
	return $pushback_
}

Node instproc get-pushback-agent {} {
	$self instvar pushback_
	if [info exists pushback_] {
		return $pushback_
	} else {
		return -1
	}
}
	
Simulator instproc pushback-duplex-link {n1 n2 bw delay} {

	$self pushback-simplex-link $n1 $n2 $bw $delay
	$self pushback-simplex-link $n2 $n1 $bw $delay
}

Simulator instproc pushback-simplex-link {n1 n2 bw delay} {
	
	set pba [$n1 get-pushback-agent]
	if {$pba == -1} {
		puts "Node does not have a pushback agent"
		exit
	}
	$self simplex-link $n1 $n2 $bw $delay RED/Pushback $pba
	
	set link [$self link $n1 $n2]
	set qmon [new QueueMonitor/ED]
	$link attach-monitors [new SnoopQueue/In] [new SnoopQueue/Out] [new SnoopQueue/Drop] $qmon
	
	set queue [$link queue]
	$queue set pushbackID_ [$pba add-queue $queue]
	$queue set-monitor $qmon
	$queue set-src-dst [$n1 set id_] [$n2 set id_]

}

Agent/Pushback instproc get-pba-port {nodeid} {

	set node [[Simulator instance] set Node_($nodeid)]
	set pba [$node get-pushback-agent]
	if {$pba == -1} {
		return -1
	} else {
		return [$pba set agent_port_]
	}
}

Agent/Pushback instproc check-queue { src dst qToCheck } {

	set link [[Simulator instance] set link_($src:$dst)]
	set queue [$link queue]
	if {$qToCheck == $queue} {
		return 1
	} else {
		return 0
	}
}
	
Queue/RED/Pushback set pushbackID_ -1
Queue/RED/Pushback set rate_limiting_ 1

Agent/Pushback set last_index_ 0
Agent/Pushback set intResult_ -1
Agent/Pushback set enable_pushback_ 1

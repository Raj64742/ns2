proc setuptcp { ns src stcp dst dtcp tcptrace sinktrace } {
	set tcp1 [new Agent/$stcp]
	$tcp1 trace $tcptrace

	set sink1 [new Agent/$dtcp]
	$sink1 trace $sinktrace

	$ns attach-agent $src $tcp1
	$ns attach-agent $dst $sink1
	$ns connect $tcp1 $sink1
	
	return $tcp1
}

proc setupsource { tcp1 stype } {
	set src1 [new Source/$stype]
	$src1 set agent_ $tcp1

	return $src1
}

Class Webbench

Webbench instproc init { } {
	$self instvar ns_ tcp_cs_ ftp_cs_ tcp_sc_ ftp_sc_ reqsz_ replsz_ numpll_
	$self instvar replycount_
}

Webbench instproc webbench { ns server stcp ssink client ctcp csink reqsz replsz numpll tcptrace sinktrace }  {
	$self instvar ns_ tcp_cs_ ftp_cs_ tcp_sc_ ftp_sc_ reqsz_ replsz_ numpll_
	$self instvar replycount_

	set ns_ $ns 
	set reqsz_ $reqsz
	set replsz_ $replsz
	set numpll_ $numpll
	set replycount_ 0

# attach TCP agents and FTP sources to the server and client nodes

# client to server
set tcp_cs_ [setuptcp $ns_ $client $ctcp $server $ssink $tcptrace $sinktrace] 
$tcp_cs_ set packetSize_ $reqsz
$tcp_cs_ finish [format "%s sendreply" $self]
set ftp_cs_ [setupsource $tcp_cs_ "FTP"]

# server to client
for {set i 0} {$i < $numpll} {incr i 1} {
 	set tcp_sc_($i) [setuptcp $ns_ $server $stcp $client $csink $tcptrace $sinktrace]
	$tcp_sc_($i) finish [format "%s recdreply" $self]
	set ftp_sc_($i) [setupsource $tcp_sc_($i) "FTP"]
}

# send a 1-pkt request from client to server
$ftp_cs_ produce 1 

}


Webbench instproc sendreply { } {
	$self instvar ns_ tcp_cs_ ftp_cs_ tcp_sc_ ftp_sc_ numpll_ reqsz_ replsz_ numpll_
	
	for {set i 0} {$i < $numpll_} {incr i 1} {
		$ftp_sc_($i) produce $replsz_
	}
}

Webbench instproc recdreply { } {
	$self instvar ns_ tcp_cs_ ftp_cs_ tcp_sc_ ftp_sc_ numpll_ reqsz_ replsz_ numpll_
	$self instvar replycount_
	
	incr replycount_ 1
	if { $replycount_ == $numpll_ } {
		set curtime [$ns_ now]
		puts [format "finished Web transaction at %s" $curtime]
	}
}


# Creates connection. First creates a source agent of type s_type and binds
# it to source.  Next creates a destination agent of type d_type and binds
# it to dest.  Finally creates bindings for the source and destination agents,
# connects them, and  returns a list of source agent and destination agent.
TestSuite instproc create-connection-list {s_type source d_type dest pktClass} {
	$self instvar ns_
	set s_agent [new Agent/$s_type]
	set d_agent [new Agent/$d_type]
	$s_agent set fid_ $pktClass
	$d_agent set fid_ $pktClass
	$ns_ attach-agent $source $s_agent
	$ns_ attach-agent $dest $d_agent
	$ns_ connect $s_agent $d_agent

	return "$s_agent $d_agent"
}    

#
# create and schedule a cbr source/dst 
#
TestSuite instproc new_cbr { startTime source dest pktSize fid dump interval file stoptime } {
	$self instvar ns_
	set cbrboth \
	    [$self create-connection-list CBR $source LossMonitor $dest $fid ]
	set cbr [lindex $cbrboth 0]
	$cbr set packetSize_ $pktSize
	$cbr set interval_ $interval
	set cbrsnk [lindex $cbrboth 1]
	$ns_ at $startTime "$cbr start"
	if { $dump == 1 } {
		puts $file "class $class packet-size $pktSize"
		$ns_ at $stoptime "printCbrPkts $cbrsnk $class $file"
	}
}

#
# create and schedule a tcp source/dst
#
TestSuite instproc new_tcp { startTime source dest window class dump size file stoptime } {
	$self instvar ns_
	set tcp [$ns_ create-connection TCP/Sack1 $source TCPSink/Sack1 $dest $class ]
	$tcp set window_ $window
	#   $tcp set tcpTick_ 0.1
	$tcp set tcpTick_ 0.01
	if {$size > 0}  {
		$tcp set packetSize_ $size
	}
	set ftp [$tcp attach-source FTP]
	$ns_ at $startTime "$ftp start"
	$ns_ at $stoptime "puts $file \"class $class total_packets_acked [$tcp set ack_]\""
	if { $dump == 1 } {
		puts $file "class $class packet-size [$tcp set packetSize_]"
	}
}

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
TestSuite instproc new_Cbr { startTime source dest pktSize interval fid maxPkts} {
	$self instvar ns_
	set cbrboth \
	    [$self create-connection-list CBR $source LossMonitor $dest $fid ]
	set cbr [lindex $cbrboth 0]
	$cbr set packetSize_ $pktSize
	$cbr set interval_ $interval
	if {$maxPkts > 0} {$cbr set maxpkts_ $maxPkts}
	set cbrsnk [lindex $cbrboth 1]
	$ns_ at $startTime "$cbr start"
}

#
# create and schedule a tcp source/dst
#
TestSuite instproc new_Tcp { startTime source dest window fid dump size type maxPkts } {
	$self instvar ns_

        if { $type == "reno" } {
		set tcp [$ns_ create-connection TCP/Reno $source TCPSink $dest $fid]
        }
        if { $type == "sack" } {
		set tcp [$ns_ create-connection TCP/Sack1 $source TCPSink/Sack1 $dest $fid]
        }

	$tcp set window_ $window
	#   $tcp set tcpTick_ 0.1
	$tcp set tcpTick_ 0.01

	if {$size > 0}  {
		$tcp set packetSize_ $size
	}
	set ftp [$tcp attach-source FTP]
	if {$maxPkts > 0} {$ftp set maxpkts_ $maxPkts}
	$ns_ at $startTime "$ftp start"
}


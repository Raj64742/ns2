# This test suite is for validating the multicast support in ns.
#
# To run all tests:  test-mcast
#
# To run individual tests:
# ns test-suite-mcast.tcl DM1
# ns test-suite-mcast.tcl DM2
# ...
#
# To view a list of available tests to run with this script:
# ns test-suite-mcast.tcl
#

Class TestSuite

TestSuite instproc init {} {
	$self instvar ns_ net_ defNet_ test_ topo_ node_ testName_
	set ns_ [new Simulator -multicast on]
	$ns_ use-scheduler List
	if {$test_ == "CtrMcast1"} {
		Node expandaddr
	}
	
	$ns_ trace-all [open temp.rands w]
	$ns_ namtrace-all [open temp.rands.nam w]
	$ns_ color 30 purple
	$ns_ color 31 green
	
	if {$net_ == ""} {
		set net_ $defNet_
	}
	if ![Topology/$defNet_ info subclass Topology/$net_] {
		global argv0
		puts "$argv0: cannot run test $test_ over topology $net_"
		exit 1
	}
	
	set topo_ [new Topology/$net_ $ns_]
	foreach i [$topo_ array names node_] {
		# This would be cool, but lets try to be compatible
		# with test-suite.tcl as far as possible.
		#
		# $self instvar $i
		# set $i [$topo_ node? $i]
		#
		set node_($i) [$topo_ node? $i]
	}
	
	if {$net_ == $defNet_} {
		set testName_ "$test_"
	} else {
		set testName_ "$test_:$net_"
	}
}

TestSuite instproc finish { file } {
	$self instvar ns_
	
	$ns_ flush-trace
#	exec awk -f ../nam-demo/nstonam.awk all.tr > [append file \.tr]
#	puts "running nam ..."
#	exec nam $file &
	exit 0
}

TestSuite instproc openTrace { stopTime testName } {
	$self instvar ns_
	exec rm -f temp.rands
	set traceFile [open temp.rands w]
	puts $traceFile "v testName $testName"
	$ns_ at $stopTime \
		"close $traceFile ; $self finish $testName"
	return $traceFile
}

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> \[<topologies>\]"
	puts stderr "Valid tests are:\t[get-subclasses TestSuite Test/]"
	puts stderr "Valid Topologies are:\t[get-subclasses SkelTopology Topology/]"
	exit 1
}

proc isProc? {cls prc} {
	if [catch "Object info subclass $cls/$prc" r] {
		global argv0
		puts stderr "$argv0: no such $cls: $prc"
		usage
	}
}

proc get-subclasses {cls pfx} {
	set ret ""
	set l [string length $pfx]
	
	set c $cls
	while {[llength $c] > 0} {
		set t [lindex $c 0]
		set c [lrange $c 1 end]
		if [string match ${pfx}* $t] {
			lappend ret [string range $t $l end]
		}
		eval lappend c [$t info subclass]
	}
	set ret
}

TestSuite proc runTest {} {
	global argc argv

	switch $argc {
		1 {
			set test $argv
			isProc? Test $test

			set topo ""
		}
		2 {
			set test [lindex $argv 0]
			isProc? Test $test
			set a [lindex $argv 1]
			if {$a == "QUIET"} {
				set topo ""
			} else {
				set topo $a
				isProc? Topology $topo
			}
		}
		default {
			usage
		}
	}
	set t [new Test/$test $topo]
	$t run
}

# Skeleton topology base class
Class SkelTopology

SkelTopology instproc init {} {
	$self next
}

SkelTopology instproc node? n {
	$self instvar node_
	if [info exists node_($n)] {
		set ret $node_($n)
	} else {
		set ret ""
	}
	set ret
}

SkelTopology instproc add-fallback-links {ns nodelist bw delay qtype args} {
	$self instvar node_
	set n1 [lindex $nodelist 0]
	foreach n2 [lrange $nodelist 1 end] {
		if ![info exists node_($n2)] {
			set node_($n2) [$ns node]
		}
		$ns duplex-link $node_($n1) $node_($n2) $bw $delay $qtype
		foreach opt $args {
			set cmd [lindex $opt 0]
			set val [lindex $opt 1]
			if {[llength $opt] > 2} {
				set x1 [lindex $opt 2]
				set x2 [lindex $opt 3]
			} else {
				set x1 $n1
				set x2 $n2
			}
			$ns $cmd $node_($x1) $node_($x2) $val
			$ns $cmd $node_($x2) $node_($x1) $val
		}
		set n1 $n2
	}
}


Class NodeTopology/4nodes -superclass SkelTopology


NodeTopology/4nodes instproc init ns {
	$self next
	
	$self instvar node_
	set node_(n0) [$ns node]
	set node_(n1) [$ns node]
	set node_(n2) [$ns node]
	set node_(n3) [$ns node]
}


Class Topology/net4a -superclass NodeTopology/4nodes

# Create a simple four node topology:
#
#	              n3
#	             / 
#       1.5Mb,10ms  / 1.5Mb,10ms                              
#    n0 --------- n1
#                  \  1.5Mb,10ms
#	            \ 
#	             n2
#

Topology/net4a instproc init ns {
	$self next $ns
	$self instvar node_
	$ns duplex-link $node_(n0) $node_(n1) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n1) $node_(n2) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n1) $node_(n3) 1.5Mb 10ms DropTail
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}

Class Topology/net4b -superclass NodeTopology/4nodes

# 4 nodes on the same LAN
#
#           n0   n1
#           |    |
#       -------------
#           |    |
#           n2   n3
#
#
 
Topology/net4b instproc init ns {
	$self next $ns
	$self instvar node_
	$ns multi-link-of-interfaces [list $node_(n0) $node_(n1) $node_(n2) $node_(n3)] 1.5Mb 10ms DropTail
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}

Class NodeTopology/5nodes -superclass SkelTopology

NodeTopology/5nodes instproc init ns {
	$self next
	
	$self instvar node_
	set node_(n0) [$ns node]
	set node_(n1) [$ns node]
	set node_(n2) [$ns node]
	set node_(n3) [$ns node]
	set node_(n4) [$ns node]
}


Class Topology/net5a -superclass NodeTopology/5nodes

#
# Create a simple five node topology:
#
#                  n4
#                 /  \                    
#               n3    n2
#               |     |
#               n0    n1
#
# All links are of 1.5Mbps bandwidth with 10ms latency
#

Topology/net5a instproc init ns {
	$self next $ns
	$self instvar node_
	$ns duplex-link $node_(n0) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n1) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n3) $node_(n4) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n4) 1.5Mb 10ms DropTail 
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}


Class Topology/net5b -superclass NodeTopology/5nodes

#
# Create a five node topology:
#
#                  n4
#                 /  \                    
#               n3----n2
#               |     |
#               n0    n1
#
# All links are of 1.5Mbps bandwidth with 10ms latency
#

Topology/net5b instproc init ns {
    $self next $ns
	$self instvar node_
	$ns duplex-link $node_(n0) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n1) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n3) $node_(n4) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n4) 1.5Mb 10ms DropTail 
	if {[$class info instprocs config] != ""} {
		$self config $ns
    }
}

Class Topology/net5c -superclass NodeTopology/5nodes

#
# Create a five node topology:
#
#                  n4
#                 /  \                    
#               n3----n2
#               | \  /|
#               |  \  |
#               | / \ |
#               n0    n1
#
# All links are of 1.5Mbps bandwidth with 10ms latency
#

Topology/net5c instproc init ns {
	$self next $ns
	$self instvar node_
	$ns duplex-link $node_(n0) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n0) $node_(n2) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n1) $node_(n2) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n1) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n4) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n3) $node_(n4) 1.5Mb 10ms DropTail 
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}


Class Topology/net5d -superclass NodeTopology/5nodes

#
# Create a five node topology:
#
#                  n4
#                 /  \                    
#               n3----n2
#               | \  /|
#               |  \  |
#               | / \ |
#               n0----n1
#
# All links are of 1.5Mbps bandwidth with 10ms latency
#

Topology/net5d instproc init ns {
	$self next $ns
	$self instvar node_
	$ns duplex-link $node_(n0) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n0) $node_(n2) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n0) $node_(n1) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n1) $node_(n2) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n1) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n4) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n3) $node_(n4) 1.5Mb 10ms DropTail 
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}


Class Topology/net5e -superclass NodeTopology/5nodes

#
# Create a five node topology with 4 nodes on a LAN:
#
#                  n4
#                 /  \                    
#               n3    n2
#               |     |
#             -----------
#               |     |
#               n0    n1
#
# All links are of 1.5Mbps bandwidth with 10ms latency
#

Topology/net5e instproc init ns {
	$self next $ns
	$self instvar node_
	$ns multi-link-of-interfaces [list $node_(n0) $node_(n1) $node_(n2) $node_(n3)] 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n3) $node_(n4) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n2) $node_(n4) 1.5Mb 10ms DropTail
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}


Class NodeTopology/6nodes -superclass SkelTopology

NodeTopology/6nodes instproc init ns {
	$self next
	
	$self instvar node_
	set node_(n0) [$ns node]
	set node_(n1) [$ns node]
	set node_(n2) [$ns node]
	set node_(n3) [$ns node]
	set node_(n4) [$ns node]
	set node_(n5) [$ns node]
}


Class Topology/net6a -superclass NodeTopology/6nodes

#
# Create a simple six node topology:
#
#                  n0
#                 /  \                    
#               n1    n2
#              /  \  /  \
#             n3   n4   n5
#
# All links are of 1.5Mbps bandwidth with 10ms latency
#

Topology/net6a instproc init ns {
	$self next $ns
	$self instvar node_
	$ns duplex-link $node_(n0) $node_(n1) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n0) $node_(n2) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n1) $node_(n3) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n1) $node_(n4) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n4) 1.5Mb 10ms DropTail 
	$ns duplex-link $node_(n2) $node_(n5) 1.5Mb 10ms DropTail 
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}

Class Topology/net6b -superclass NodeTopology/6nodes

# 6 node topology with nodes n2, n3 and n5 on a LAN.
#
#          n4
#          |
#          n3
#          |
#    --------------
#      |       |
#      n5      n2
#      |       |
#      n0      n1
#
# All point-to-point links have 1.5Mbps Bandwidth, 10ms latency.
#

Topology/net6b instproc init ns {
	$self next $ns
	$self instvar node_
	$ns multi-link-of-interfaces [list $node_(n5) $node_(n2) $node_(n3)] 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n1) $node_(n2) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n4) $node_(n3) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n5) $node_(n0) 1.5Mb 10ms DropTail
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}


Class NodeTopology/8nodes -superclass SkelTopology

NodeTopology/8nodes instproc init ns {
	$self next
	
	$self instvar node_
	set node_(n0) [$ns node]
	set node_(n1) [$ns node]
	set node_(n2) [$ns node]
	set node_(n3) [$ns node]
	set node_(n4) [$ns node]
	set node_(n5) [$ns node]
	set node_(n6) [$ns node]
	set node_(n7) [$ns node]
}

Class Topology/net8a -superclass NodeTopology/8nodes

# 8 node topology with nodes n2, n3, n4 and n5 on a LAN.
#
#      n0----n1     
#      |     |
#      n2    n3
#      |     |
#    --------------
#      |     |
#      n4    n5
#      |     |
#      n6    n7
#
# All point-to-point links have 1.5Mbps Bandwidth, 10ms latency.
#

Topology/net8a instproc init ns {
	$self next $ns
	$self instvar node_
	$ns multi-link-of-interfaces [list $node_(n2) $node_(n3) $node_(n4) $node_(n5)] 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n0) $node_(n1) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n0) $node_(n2) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n1) $node_(n3) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n4) $node_(n6) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(n5) $node_(n7) 1.5Mb 10ms DropTail
	if {[$class info instprocs config] != ""} {
		$self config $ns
	}
}


# Definition of test-suite tests

# Testing group join/leave in a simple topology
Class Test/DM1 -superclass TestSuite
Test/DM1 instproc init topo {
	source ../mcast/DM.tcl
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4a
	set test_	DM1
	$self next
}
Test/DM1 instproc run {} {
	$self instvar ns_ node_ testName_

	set mproto DM
	set mrthandle [$ns_ mrtproto $mproto {}]
	
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $node_(n1) $udp0
	$udp0 set dst_ 0x8001
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	
	set udp1 [new Agent/UDP]
	$ns_ attach-agent $node_(n3) $udp1
	$udp1 set dst_ 0x8002
	$udp1 set class_ 1
	set cbr1 [new Application/Traffic/CBR]
	$cbr1 attach-agent $udp1

	set tcp [new Agent/TCP]
	set sink [new Agent/TCPSink]
	$ns_ attach-agent $node_(n0) $tcp
	$ns_ attach-agent $node_(n3) $sink
	$ns_ connect $tcp $sink
	set ftp [new Source/FTP]
	$ftp set agent_ $tcp
	
	set rcvr [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n2) $rcvr
	$ns_ at 1.2 "$node_(n2) join-group $rcvr 0x8002; $ftp start"
	$ns_ at 1.25 "$node_(n2) leave-group $rcvr 0x8002"
	$ns_ at 1.3 "$node_(n2) join-group $rcvr 0x8002"
	$ns_ at 1.35 "$node_(n2) join-group $rcvr 0x8001"
	
	$ns_ at 1.0 "$cbr0 start"
	$ns_ at 1.1 "$cbr1 start"
	
	$ns_ at 1.8 "$self finish 4a-nam"
	$ns_ run
}

# Testing group join/leave in a richer topology. Testing rcvr join before
# the source starts sending pkts to the group.
Class Test/DM2 -superclass TestSuite
Test/DM2 instproc init topo {
	source ../mcast/DM.tcl
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6a
	set test_	DM2
	$self next
}
Test/DM2 instproc run {} {
	$self instvar ns_ node_ testName_
	
	### Start multicast configuration
	DM set PruneTimeout 0.3
	set mproto DM
	set mrthandle [$ns_ mrtproto $mproto  {}]
	### End of multicast  config
	
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $node_(n0) $udp0
	$udp0 set dst_ 0x8002
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	
	set rcvr [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n3) $rcvr
	$ns_ attach-agent $node_(n4) $rcvr
	$ns_ attach-agent $node_(n5) $rcvr
	
	$ns_ at 0.2 "$node_(n3) join-group $rcvr 0x8002"
	$ns_ at 0.4 "$node_(n4) join-group $rcvr 0x8002"
	$ns_ at 0.6 "$node_(n3) leave-group $rcvr 0x8002"
	$ns_ at 0.7 "$node_(n5) join-group $rcvr 0x8002"
	$ns_ at 0.95 "$node_(n3) join-group $rcvr 0x8002"
	
	$ns_ at 0.3 "$cbr0 start"
	$ns_ at 1.0 "$self finish 6a-nam"
	
	$ns_ run
}

# Testing group join/leave in a richer topology. Testing rcvr join before
# the source starts sending pkts to the group.
Class Test/pimDM1 -superclass TestSuite
Test/pimDM1 instproc init topo {
	source ../mcast/DM.tcl
	source ../mcast/pimDM.tcl
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6a
	set test_	pimDM1
	$self next
}
Test/pimDM1 instproc run {} {
	$self instvar ns_ node_ testName_
	
	### Start multicast configuration
	pimDM set PruneTimeout 0.3
	set mproto pimDM
	set mrthandle [$ns_ mrtproto $mproto  {}]
	### End of multicast  config
	
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $node_(n0) $udp0
	$udp0 set dst_ 0x8002
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	
	set rcvr [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n3) $rcvr
	$ns_ attach-agent $node_(n4) $rcvr
	$ns_ attach-agent $node_(n5) $rcvr
	
	$ns_ at 0.2 "$node_(n3) join-group $rcvr 0x8002"
	$ns_ at 0.4 "$node_(n4) join-group $rcvr 0x8002"
	$ns_ at 0.6 "$node_(n3) leave-group $rcvr 0x8002"
	$ns_ at 0.7 "$node_(n5) join-group $rcvr 0x8002"
	$ns_ at 0.95 "$node_(n3) join-group $rcvr 0x8002"
	
	$ns_ at 0.3 "$cbr0 start"
	$ns_ at 1.0 "$self finish 6a-nam"
	
	$ns_ run
}

# Testing dynamics of links going up/down.
Class Test/dynamicDM1 -superclass TestSuite
Test/dynamicDM1 instproc init topo {
	source ../mcast/DM.tcl
	source ../mcast/dynamicDM.tcl
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6a
	set test_	dynamicDM1
	$self next
}
Test/dynamicDM1 instproc run {} {
	$self instvar ns_ node_ testName_
	$ns_ rtproto Session
	### Start multicast configuration
	dynamicDM set PruneTimeout 0.3
	dynamicDM set ReportRouteTimeout 0.15
	set mproto dynamicDM
	set mrthandle [$ns_ mrtproto $mproto  {}]
	### End of multicast  config
	
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $node_(n0) $udp0
	$udp0 set dst_ 0x8002
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	
	set rcvr [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n3) $rcvr
	$ns_ attach-agent $node_(n4) $rcvr
	$ns_ attach-agent $node_(n5) $rcvr
	
	$ns_ at 0.2 "$node_(n3) join-group $rcvr 0x8002"
	$ns_ at 0.4 "$node_(n4) join-group $rcvr 0x8002"
	$ns_ at 0.6 "$node_(n3) leave-group $rcvr 0x8002"
	$ns_ at 0.7 "$node_(n5) join-group $rcvr 0x8002"
	$ns_ at 0.8 "$node_(n3) join-group $rcvr 0x8002"
	#### Link between n0 & n1 down at 1.0, up at 1.2
	$ns_ rtmodel-at 1.0 down $node_(n0) $node_(n1)
	$ns_ rtmodel-at 1.2 up   $node_(n0) $node_(n1)
	####
	
	$ns_ at 0.1 "$cbr0 start"
	$ns_ at 1.6 "$self finish 6a-nam"
	
	$ns_ run
}

# Testing group join/leave in a simple topology, changing the RP set. 
# The RP node also has a source.
Class Test/CtrMcast1 -superclass TestSuite
Test/CtrMcast1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4a
	set test_	CtrMcast1
	$self next
}
# source and RP on same node
Test/CtrMcast1 instproc run {} {
	$self instvar ns_ node_ testName_
	
	set mproto CtrMcast
	set mrthandle [$ns_ mrtproto $mproto  {}]
	$mrthandle set_c_rp [list $node_(n2)]
	
	set udp1 [new Agent/UDP]
	$ns_ attach-agent $node_(n2) $udp1
	
	set grp [Node allocaddr]
	
	$udp1 set dst_ $grp
	##$udp1 set dst_ 0x8003
	
	set cbr1 [new Application/Traffic/CBR]
	$cbr1 attach-agent $udp1

	set udp2 [new Agent/UDP]
	$ns_ attach-agent $node_(n3) $udp2
	$udp2 set dst_ $grp
	##$udp2 set dst_ 0x8003
	
	set cbr2 [new Application/Traffic/CBR]
	$cbr2 attach-agent $udp2

	set rcvr0 [new Agent/Null]
	$ns_ attach-agent $node_(n0) $rcvr0
	set rcvr1 [new Agent/Null]
	$ns_ attach-agent $node_(n1) $rcvr1
	set rcvr2  [new Agent/Null]
	$ns_ attach-agent $node_(n2) $rcvr2
	set rcvr3 [new Agent/Null]
	$ns_ attach-agent $node_(n3) $rcvr3
	
	$ns_ at 0.2 "$cbr1 start"
	$ns_ at 0.25 "$cbr2 start"
	$ns_ at 0.3 "$node_(n1) join-group  $rcvr1 $grp"
	$ns_ at 0.4 "$node_(n0) join-group  $rcvr0 $grp"

	$ns_ at 0.45 "$mrthandle switch-treetype $grp"

	$ns_ at 0.5 "$node_(n3) join-group  $rcvr3 $grp"
	$ns_ at 0.65 "$node_(n2) join-group  $rcvr2 $grp"
	
	$ns_ at 0.7 "$node_(n0) leave-group $rcvr0 $grp"
	$ns_ at 0.8 "$node_(n2) leave-group  $rcvr2 $grp"

	$ns_ at 0.9 "$node_(n3) leave-group  $rcvr3 $grp"
	$ns_ at 1.0 "$node_(n1) leave-group $rcvr1 $grp"
	$ns_ at 1.1 "$node_(n1) join-group $rcvr1 $grp"
	$ns_ at 1.2 "$self finish 4a-nam"
	
	$ns_ run
}




# Testing performance in the presence of dynamics. Also testing a rcvr joining
# a group before the src starts sending pkts to the group.
#Class Test/CtrMcast2 -superclass TestSuite
#Test/CtrMcast2 instproc init topo {
#	 $self instvar net_ defNet_ test_
#	 set net_	$topo
#	 set defNet_	net6a
#	 set test_	CtrMcast2
#	 $self next
#}
#Test/CtrMcast2 instproc run {} {
#	 $self instvar ns_ node_ testName_
#
#	 $ns_ rtproto Session
#	 set mproto CtrMcast
#	 set mrthandle [$ns_ mrtproto $mproto  {}]
#	 
#	 set udp0 [new Agent/UDP]
#	 $ns_ attach-agent $node_(n0) $udp0
#	 $udp0 set dst_ 0x8003
#	 set cbr0 [new Application/Traffic/CBR]
#	 $cbr0 attach-agent $udp0
#
#	 set rcvr [new Agent/Null]
#	 $ns_ attach-agent $node_(n3) $rcvr
#	 $ns_ attach-agent $node_(n4) $rcvr
#	 $ns_ attach-agent $node_(n5) $rcvr
#	 
#	 $ns_ at 0.3 "$node_(n3) join-group  $rcvr 0x8003"
#	 $ns_ at 0.35 "$cbr0 start"
#	 $ns_ at 0.4 "$node_(n4) join-group  $rcvr 0x8003"
#	 $ns_ at 0.5 "$node_(n5) join-group  $rcvr 0x8003"
#
#	 ### Link between n2 & n4 down at 0.6, up at 1.2
#	 $ns_ rtmodel-at 0.6 down $node_(n2) $node_(n4)
#	 $ns_ rtmodel-at 1.2 up $node_(n2) $node_(n4)
#	 ###
#
#	 $ns_ at 1.0 "$mrthandle switch-treetype 0x8003"
#
#	 ### Link between n0 & n1 down at 1.5, up at 2.0
#	 $ns_ rtmodel-at 1.5 down $node_(n0) $node_(n1)
#	 $ns_ rtmodel-at 2.0 up $node_(n0) $node_(n1)
#	 ###
#	 $ns_ at 2.2 "$self finish 6a-nam"
#	 
#	 $ns_ run
#}
#

# Group join/leave test 
#Class Test/detailedDM1 -superclass TestSuite
#Test/detailedDM1 instproc init topo {
#	  $self instvar net_ defNet_ test_
#	  set net_	$topo
#	  set defNet_	net6a
#	  set test_	detailedDM1
#	  $self next
#}
#Test/detailedDM1 instproc run {} {
#	  $self instvar ns_ node_ testName_
#
#	  $ns_ rtproto Session
#	  ### Start multicast configuration
#	  Prune/Iface/Timer set timeout 0.3
#	  set mproto detailedDM
#	  set mrthandle [$ns_ mrtproto $mproto  {}]
#	  ### End of multicast  config
#
#	  set udp0 [new Agent/UDP]
#	  $ns_ attach-agent $node_(n0) $udp0
#	  $udp0 set dst_ 0x8002
#	  set cbr0 [new Application/Traffic/CBR]
#	  $cbr0 attach-agent $udp0
#	  
#	  set rcvr [new Agent/LossMonitor]
#	  $ns_ attach-agent $node_(n3) $rcvr
#	  $ns_ attach-agent $node_(n4) $rcvr
#	  $ns_ attach-agent $node_(n5) $rcvr
#	  
#	  $ns_ at 0.2 "$node_(n3) join-group $rcvr 0x8002"
#	  $ns_ at 0.4 "$node_(n4) join-group $rcvr 0x8002"
#	  $ns_ at 0.6 "$node_(n3) leave-group $rcvr 0x8002"
#	  $ns_ at 0.7 "$node_(n5) join-group $rcvr 0x8002"
#	  $ns_ at 1.15 "$node_(n3) join-group $rcvr 0x8002"
#	  $ns_ at 1.8 "$node_(n3) leave-group $rcvr 0x8002"
#	  
#	  $ns_ at 0.1 "$cbr0 start"
#	  $ns_ at 2.0 "$self finish 6a-nam"
#
#	  $ns_ run
#}
#
# Group join/leave test with dynamics
#
#Class Test/detailedDM2 -superclass TestSuite
#Test/detailedDM2 instproc init topo {
#	  $self instvar net_ defNet_ test_
#	  set net_	$topo
#	  set defNet_	net6a
#	  set test_	detailedDM2
#	  $self next
#}
#Test/detailedDM2 instproc run {} {
#	  $self instvar ns_ node_ testName_
#
#	  $ns_ rtproto Session
#	  ### Start multicast configuration
#	  Prune/Iface/Timer set timeout 0.3
#	  set mproto detailedDM
#	  set mrthandle [$ns_ mrtproto $mproto  {}]
#	  ### End of multicast  config
#
#	  set udp0 [new Agent/UDP]
#	  $ns_ attach-agent $node_(n0) $udp0
#	  $udp0 set dst_ 0x8002
#	  set cbr0 [new Application/Traffic/CBR]
#	  $cbr0 attach-agent $udp0
#	  
#	  set rcvr [new Agent/LossMonitor]
#	  $ns_ attach-agent $node_(n3) $rcvr
#	  $ns_ attach-agent $node_(n4) $rcvr
#	  $ns_ attach-agent $node_(n5) $rcvr
#	  
#	  $ns_ at 0.2 "$node_(n3) join-group $rcvr 0x8002"
#	  $ns_ at 0.4 "$node_(n4) join-group $rcvr 0x8002"
#	  $ns_ at 0.7 "$node_(n5) join-group $rcvr 0x8002"
#	  $ns_ at 1.8 "$node_(n3) leave-group $rcvr 0x8002"
#	  
#	  ### Link between n0 and n1 down at 1.0 up at 1.6
#	  $ns_ rtmodel-at 1.0 down $node_(n0) $node_(n1)
#	  $ns_ rtmodel-at 1.6 up $node_(n0) $node_(n1)
#	  ###
#
#	  $ns_ at 0.1 "$cbr0 start"
#	  $ns_ at 2.0 "$self finish 6a-nam"
#
#	  $ns_ run
#}
#
# Group join/leave test with dynamics. n3 joins group when an upstream link
# on its shortest path to the source has failed.
#
#Class Test/detailedDM3 -superclass TestSuite
#Test/detailedDM3 instproc init topo {
#	  $self instvar net_ defNet_ test_
#	  set net_	$topo
#	  set defNet_	net6a
#	  set test_	detailedDM3
#	  $self next
#}
#Test/detailedDM3 instproc run {} {
#	  $self instvar ns_ node_ testName_
#
#	  $ns_ rtproto Session
#	  ### Start multicast configuration
#	  Prune/Iface/Timer set timeout 0.3
#	  set mproto detailedDM
#	  set mrthandle [$ns_ mrtproto $mproto  {}]
#	  ### End of multicast  config
#
#	  set udp0 [new Agent/UDP]
#	  $ns_ attach-agent $node_(n0) $udp0
#	  $udp0 set dst_ 0x8002
#	  set cbr0 [new Application/Traffic/CBR]
#	  $cbr0 attach-agent $udp0
#	  
#	  set rcvr [new Agent/LossMonitor]
#	  $ns_ attach-agent $node_(n3) $rcvr
#	  $ns_ attach-agent $node_(n4) $rcvr
#	  $ns_ attach-agent $node_(n5) $rcvr
#	  
#	  $ns_ at 0.4 "$node_(n4) join-group $rcvr 0x8002"
#	  $ns_ at 0.7 "$node_(n5) join-group $rcvr 0x8002"
#	  $ns_ at 1.1 "$node_(n3) join-group $rcvr 0x8002"
#	  $ns_ at 1.8 "$node_(n3) leave-group $rcvr 0x8002"
#	  
#	  ### Link between n0 and n1 down at 1.0 up at 1.6
#	  $ns_ rtmodel-at 1.0 down $node_(n0) $node_(n1)
#	  $ns_ rtmodel-at 1.6 up $node_(n0) $node_(n1)
#	  ###
#
#	  $ns_ at 0.1 "$cbr0 start"
#	  $ns_ at 2.0 "$self finish 6a-nam"
#
#	  $ns_ run
#}
#
# Group join/leave test with dynamics. Node n3 joins the group when an
# upstream link on its shortest path to the source has failed.
#
#Class Test/detailedDM4 -superclass TestSuite
#Test/detailedDM4 instproc init topo {
#	  $self instvar net_ defNet_ test_
#	  set net_	$topo
#	  set defNet_	net6a
#	  set test_	detailedDM4
#	  $self next
#}
#Test/detailedDM4 instproc run {} {
#	  $self instvar ns_ node_ testName_
#
#	  $ns_ rtproto Session
#	  ### Start multicast configuration
#	  Prune/Iface/Timer set timeout 0.3
#	  set mproto detailedDM
#	  set mrthandle [$ns_ mrtproto $mproto  {}]
#	  ### End of multicast  config
#
#	  set udp0 [new Agent/UDP]
#	  $ns_ attach-agent $node_(n0) $udp0
#	  $udp0 set dst_ 0x8002
#	  set cbr0 [new Application/Traffic/CBR]
#	  $cbr0 attach-agent $udp0
#
#	  
#	  set rcvr [new Agent/LossMonitor]
#	  $ns_ attach-agent $node_(n3) $rcvr
#	  $ns_ attach-agent $node_(n4) $rcvr
#	  $ns_ attach-agent $node_(n5) $rcvr
#	  
#	  $ns_ at 0.4 "$node_(n4) join-group $rcvr 0x8002"
#	  $ns_ at 0.7 "$node_(n5) join-group $rcvr 0x8002"
#	  $ns_ at 1.35 "$node_(n3) join-group $rcvr 0x8002"
#	  $ns_ at 1.8 "$node_(n3) leave-group $rcvr 0x8002"
#	  
#	  ### Link between n0 and n1 down at 1.0 up at 1.6
#	  $ns_ rtmodel-at 1.0 down $node_(n0) $node_(n1)
#	  $ns_ rtmodel-at 1.6 up $node_(n0) $node_(n1)
#	  ###
#
#	  $ns_ at 0.5 "$cbr0 start"
#	  $ns_ at 2.0 "$self finish 6a-nam"
#
#	  $ns_ run
#}
#
# Testing in LAN topologies
# The node with the higher id is made the LAN forwarder in case there are
# more than one possible forwarders (this is decided by the Assert process).
# Class Test/detailedDM5 -superclass TestSuite
# Test/detailedDM5 instproc init topo {
# 	$self instvar net_ defNet_ test_
# 	set net_	$topo
# 	set defNet_	net8a
# 	set test_	detailedDM5
# 	$self next
# }
# Test/detailedDM5 instproc run {} {
# 	$self instvar ns_ node_ testName_

# 	$ns_ rtproto Session
# 	### Start multicast configuration
# 	Prune/Iface/Timer set timeout 0.3
# 	Deletion/Iface/Timer set timeout 0.1
# 	set mproto detailedDM
# 	set mrthandle [$ns_ mrtproto $mproto  {}]
# 	### End of multicast  config

# 	set udp0 [new Agent/UDP]
# 	$ns_ attach-agent $node_(n0) $udp0
# 	$udp0 set dst_ 0x8002
#	set cbr0 [new Application/Traffic/CBR]
#	$cbr0 attach-agent $udp0
# 	puts "grp-addr = [$cbr0 set dst_]"
# 	set rcvr [new Agent/LossMonitor]
# 	$ns_ attach-agent $node_(n3) $rcvr
# 	$ns_ attach-agent $node_(n6) $rcvr
# 	$ns_ attach-agent $node_(n7) $rcvr

# 	$ns_ at 0.1 "$cbr0 start"	
# 	$ns_ at 0.4 "$node_(n6) join-group $rcvr 0x8002"
# 	$ns_ at 1.1 "$node_(n7) join-group $rcvr 0x8002"
# 	$ns_ at 1.3 "$node_(n6) leave-group $rcvr 0x8002"
# 	$ns_ at 1.8 "$node_(n7) leave-group $rcvr 0x8002"
	
# 	$ns_ at 2.2 "$self finish 7a-nam"
# 	$ns_ run
# }

# Testing in LAN topologies
# The node with the higher id is made the LAN forwarder in case there are
# more than one possible forwarders (this is decided by the Assert process).
# Class Test/detailedDM6 -superclass TestSuite
# Test/detailedDM6 instproc init topo {
# 	$self instvar net_ defNet_ test_
# 	set net_	$topo
# 	set defNet_	net8a
# 	set test_	detailedDM6
# 	$self next
# }
# Test/detailedDM6 instproc run {} {
# 	$self instvar ns_ node_ testName_

# 	$ns_ rtproto Session
# 	### Start multicast configuration
# 	Prune/Iface/Timer set timeout 0.3
# 	Deletion/Iface/Timer set timeout 0.1
# 	set mproto detailedDM
# 	set mrthandle [$ns_ mrtproto $mproto  {}]
# 	### End of multicast  config

# 	set udp0 [new Agent/UDP]
# 	$ns_ attach-agent $node_(n0) $udp0
# 	$udp0 set dst_ 0x8002
#	set cbr0 [new Application/Traffic/CBR]
#	$cbr0 attach-agent $udp0
	
# 	set rcvr [new Agent/LossMonitor]
# 	$ns_ attach-agent $node_(n3) $rcvr
# 	$ns_ attach-agent $node_(n6) $rcvr
# 	$ns_ attach-agent $node_(n7) $rcvr

# 	$ns_ at 0.1 "$cbr0 start"	
# 	$ns_ at 0.4 "$node_(n3) join-group $rcvr 0x8002"
# 	$ns_ at 1.1 "$node_(n6) join-group $rcvr 0x8002"
# 	$ns_ at 1.2 "$node_(n3) leave-group $rcvr 0x8002"
	
	### Link between n1 and n3 down at 0.7 up at 1.0
# 	$ns_ rtmodel-at 0.7 down $node_(n1) $node_(n3)
# 	$ns_ rtmodel-at 1.0 up $node_(n1) $node_(n3)
	###

# 	$ns_ at 1.5 "$self finish 7a-nam"
# 	$ns_ run
# }

# Testing in LAN topologies
# The node with the higher id is made the LAN forwarder in case there are
# more than one possible forwarders (this is decided by the Assert process).
# Class Test/detailedDM7 -superclass TestSuite
# Test/detailedDM7 instproc init topo {
# 	$self instvar net_ defNet_ test_
# 	set net_	$topo
# 	set defNet_	net8a
# 	set test_	detailedDM7
# 	$self next
# }
# Test/detailedDM7 instproc run {} {
# 	$self instvar ns_ node_ testName_

# 	$ns_ rtproto Session
# 	### Start multicast configuration
# 	Prune/Iface/Timer set timeout 0.3
# 	Deletion/Iface/Timer set timeout 0.1
# 	set mproto detailedDM
# 	set mrthandle [$ns_ mrtproto $mproto  {}]
# 	### End of multicast  config

# 	set udp0 [new Agent/UDP]
# 	$ns_ attach-agent $node_(n0) $udp0
# 	$udp0 set dst_ 0x8002
#	set cbr0 [new Application/Traffic/CBR]
#	$cbr0 attach-agent $udp0
	
# 	set rcvr [new Agent/LossMonitor]
# 	$ns_ attach-agent $node_(n3) $rcvr
# 	$ns_ attach-agent $node_(n6) $rcvr
# 	$ns_ attach-agent $node_(n7) $rcvr

# 	$ns_ at 0.1 "$cbr0 start"	
# 	$ns_ at 0.8 "$node_(n6) join-group $rcvr 0x8002"
# 	$ns_ at 1.8 "$node_(n6) leave-group $rcvr 0x8002"
	
# 	### Link between n0 and n1 down at 0.4 up at 1.1
# 	$ns_ rtmodel-at 0.4 down $node_(n0) $node_(n1)
# 	$ns_ rtmodel-at 1.1 up $node_(n0) $node_(n1)
# 	###

# 	$ns_ at 1.8 "$self finish 7a-nam"
# 	$ns_ run
# }

TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:



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

set dir [pwd]
catch "cd tcl/test"
source misc.tcl
source topologies.tcl
catch "cd $dir"

# Method to create the multicast simulator...
TestSuite instproc get-simulator {} {
	new Simulator -multicast 1		;# viola
}

# Definition of test-suite tests

# Testing group join/leave in a simple topology
Class Test/DM1 -superclass TestSuite
Test/DM1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4a
	set test_	DM1
	$self next 1
}
Test/DM1 instproc run {} {
	$self instvar ns_ node_ testName_

	set mproto DM
	set mrthandle [$ns_ mrtproto $mproto {}]
	
	set cbr0 [new Agent/CBR]
	$cbr0 set dst_ 0x8001
	$ns_ attach-agent $node_(n1) $cbr0
	$ns_ at 1.0 "$cbr0 start"
	
	set cbr1 [new Agent/CBR]
	$cbr1 set dst_ 0x8002
	$cbr1 set class_ 1
	$ns_ attach-agent $node_(n3) $cbr1
	$ns_ at 1.1 "$cbr1 start"

	set rcvr [new Agent/LossMonitor]	;# multicast sink
	$ns_ attach-agent $node_(n2) $rcvr
	#
	$ns_ at 1.35 "$node_(n2) join-group $rcvr 0x8001"
	#
	$ns_ at 1.2 "$node_(n2) join-group $rcvr 0x8002"
	$ns_ at 1.25 "$node_(n2) leave-group $rcvr 0x8002"
	$ns_ at 1.3 "$node_(n2) join-group $rcvr 0x8002"

	set tcp [new Agent/TCP]
	set sink [new Agent/TCPSink]
	$ns_ attach-agent $node_(n0) $tcp
	$ns_ attach-agent $node_(n3) $sink
	$ns_ connect $tcp $sink

	set ftp [new Source/FTP]
	$ftp set agent_ $tcp
	$ns_ at 1.2 "$ftp start"
	
	$ns_ at 1.8 "$self finish 4a-nam"
	$ns_ run
}

# Testing group join/leave in a richer topology. Testing rcvr join before
# the source starts sending pkts to the group.
Class Test/DM2 -superclass TestSuite
Test/DM2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6a
	set test_	DM2
	$self next 1
}
Test/DM2 instproc run {} {
	$self instvar ns_ node_ testName_

	### Start multicast configuration
	DM set PruneTimeout 0.3
	set mproto DM
	set mrthandle [$ns_ mrtproto $mproto  {}]
	### End of multicast  config
	
	set cbr0 [new Agent/CBR]
	$ns_ attach-agent $node_(n0) $cbr0
	$cbr0 set dst_ 0x8002
	
	set rcvr [new Agent/LossMonitor]	;# generic multicast sink
	
	$ns_ attach-agent $node_(n3) $rcvr
	$ns_ at 0.2 "$node_(n3) join-group $rcvr 0x8002"
	$ns_ at 0.6 "$node_(n3) leave-group $rcvr 0x8002"
	$ns_ at 0.95 "$node_(n3) join-group $rcvr 0x8002"

	$ns_ attach-agent $node_(n4) $rcvr
	$ns_ at 0.4 "$node_(n4) join-group $rcvr 0x8002"
	$ns_ at 0.7 "$node_(n5) join-group $rcvr 0x8002"
	
	$ns_ attach-agent $node_(n5) $rcvr
	$ns_ at 0.3 "$cbr0 start"
	$ns_ at 1.0 "$self finish 6a-nam"
	
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
	$self next 1
}
# source and RP on same node
Test/CtrMcast1 instproc run {} {
	$self instvar ns_ node_ testName_

	set mproto CtrMcast
	set mrthandle [$ns_ mrtproto $mproto  {}]
	$mrthandle set_c_rp [list $node_(n2)]
		
	set cbr1 [new Agent/CBR]
	$ns_ attach-agent $node_(n2) $cbr1
	$cbr1 set dst_ 0x8003

	set cbr2 [new Agent/CBR]
	$ns_ attach-agent $node_(n3) $cbr2
	$cbr2 set dst_ 0x8003

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
	$ns_ at 0.3 "$node_(n1) join-group  $rcvr1 0x8003"
	$ns_ at 0.4 "$node_(n0) join-group  $rcvr0 0x8003"

	$ns_ at 0.45 "$mrthandle switch-treetype 0x8003"

	$ns_ at 0.5 "$node_(n3) join-group  $rcvr3 0x8003"
	$ns_ at 0.65 "$node_(n2) join-group  $rcvr2 0x8003"
	
	$ns_ at 0.7 "$node_(n0) leave-group $rcvr0 0x8003"
	$ns_ at 0.8 "$node_(n2) leave-group  $rcvr2 0x8003"

	$ns_ at 0.9 "$node_(n3) leave-group  $rcvr3 0x8003"
	$ns_ at 1.0 "$node_(n1) leave-group $rcvr1 0x8003"
	$ns_ at 1.1 "$node_(n1) join-group $rcvr1 0x8003"
	$ns_ at 1.2 "$self finish 4a-nam"
	
	$ns_ run
}

# Testing performance in the presence of dynamics. Also testing a rcvr joining
# a group before the src starts sending pkts to the group.
Class Test/CtrMcast2 -superclass TestSuite
Test/CtrMcast2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6a
	set test_	CtrMcast2
	$self next 1
}
Test/CtrMcast2 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ rtproto Session
	set mproto CtrMcast
	set mrthandle [$ns_ mrtproto $mproto  {}]
	
	set cbr0 [new Agent/CBR]
	$ns_ attach-agent $node_(n0) $cbr0
	$cbr0 set dst_ 0x8003

	set rcvr [new Agent/Null]
	$ns_ attach-agent $node_(n3) $rcvr
	$ns_ attach-agent $node_(n4) $rcvr
	$ns_ attach-agent $node_(n5) $rcvr
	
	$ns_ at 0.3 "$node_(n3) join-group  $rcvr 0x8003"
	$ns_ at 0.35 "$cbr0 start"
	$ns_ at 0.4 "$node_(n4) join-group  $rcvr 0x8003"
	$ns_ at 0.5 "$node_(n5) join-group  $rcvr 0x8003"

	### Link between n2 & n4 down at 0.6, up at 1.2
	$ns_ rtmodel-at 0.6 down $node_(n2) $node_(n4)
	$ns_ rtmodel-at 1.2 up $node_(n2) $node_(n4)
	###

	$ns_ at 1.0 "$mrthandle switch-treetype 0x8003"

	### Link between n0 & n1 down at 1.5, up at 2.0
	$ns_ rtmodel-at 1.5 down $node_(n0) $node_(n1)
	$ns_ rtmodel-at 2.0 up $node_(n0) $node_(n1)
	###
	$ns_ at 2.2 "$self finish 6a-nam"
	
	$ns_ run
}

# Group join/leave test
Class Test/detailedDM1 -superclass TestSuite
Test/detailedDM1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6a
	set test_	detailedDM1
	$self next 1
}
Test/detailedDM1 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ rtproto Session

	### Start multicast configuration
	Timer/Iface/Prune set timeout 0.3
	set mproto detailedDM
	set mrthandle [$ns_ mrtproto $mproto  {}]
	### End of multicast  config

	set cbr0 [new Agent/CBR]
	$ns_ attach-agent $node_(n0) $cbr0
	$cbr0 set dst_ 0x8002
	$ns_ at 0.1 "$cbr0 start"

	set rcvr [new Agent/LossMonitor]	;# generic multicast sink
	$ns_ attach-agent $node_(n3) $rcvr
	$ns_ at 0.2 "$node_(n3) join-group $rcvr 0x8002"
	$ns_ at 0.6 "$node_(n3) leave-group $rcvr 0x8002"
	$ns_ at 1.15 "$node_(n3) join-group $rcvr 0x8002"
	$ns_ at 1.8 "$node_(n3) leave-group $rcvr 0x8002"

	$ns_ attach-agent $node_(n4) $rcvr
	$ns_ at 0.4 "$node_(n4) join-group $rcvr 0x8002"
	
	$ns_ attach-agent $node_(n5) $rcvr
	$ns_ at 0.7 "$node_(n5) join-group $rcvr 0x8002"
	
	$ns_ at 2.0 "$self finish 6a-nam"

	$ns_ run
}

# Group join/leave test with dynamics
Class Test/detailedDM2 -superclass TestSuite
Test/detailedDM2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6a
	set test_	detailedDM2
	$self next 1
}
Test/detailedDM2 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ rtproto Session
	### Start multicast configuration
	Timer/Iface/Prune set timeout 0.3
	set mproto detailedDM
	set mrthandle [$ns_ mrtproto $mproto  {}]
	### End of multicast  config

	set cbr0 [new Agent/CBR]
	$ns_ attach-agent $node_(n0) $cbr0
	$cbr0 set dst_ 0x8002
	
	set rcvr [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n3) $rcvr
	$ns_ attach-agent $node_(n4) $rcvr
	$ns_ attach-agent $node_(n5) $rcvr
	
	$ns_ at 0.2 "$node_(n3) join-group $rcvr 0x8002"
	$ns_ at 0.4 "$node_(n4) join-group $rcvr 0x8002"
	$ns_ at 0.7 "$node_(n5) join-group $rcvr 0x8002"
	$ns_ at 1.8 "$node_(n3) leave-group $rcvr 0x8002"
	
	### Link between n0 and n1 down at 1.0 up at 1.6
	$ns_ rtmodel-at 1.0 down $node_(n0) $node_(n1)
	$ns_ rtmodel-at 1.6 up $node_(n0) $node_(n1)
	###

	$ns_ at 0.1 "$cbr0 start"
	$ns_ at 2.0 "$self finish 6a-nam"

	$ns_ run
}

# Group join/leave test with dynamics. n3 joins group when an upstream link
# on its shortest path to the source has failed.
Class Test/detailedDM3 -superclass TestSuite
Test/detailedDM3 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6a
	set test_	detailedDM3
	$self next 1
}
Test/detailedDM3 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ rtproto Session
	### Start multicast configuration
	Timer/Iface/Prune set timeout 0.3
	set mproto detailedDM
	set mrthandle [$ns_ mrtproto $mproto  {}]
	### End of multicast  config

	set cbr0 [new Agent/CBR]
	$ns_ attach-agent $node_(n0) $cbr0
	$cbr0 set dst_ 0x8002
	
	set rcvr [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n3) $rcvr
	$ns_ attach-agent $node_(n4) $rcvr
	$ns_ attach-agent $node_(n5) $rcvr
	
	$ns_ at 0.4 "$node_(n4) join-group $rcvr 0x8002"
	$ns_ at 0.7 "$node_(n5) join-group $rcvr 0x8002"
	$ns_ at 1.1 "$node_(n3) join-group $rcvr 0x8002"
	$ns_ at 1.8 "$node_(n3) leave-group $rcvr 0x8002"
	
	### Link between n0 and n1 down at 1.0 up at 1.6
	$ns_ rtmodel-at 1.0 down $node_(n0) $node_(n1)
	$ns_ rtmodel-at 1.6 up $node_(n0) $node_(n1)
	###

	$ns_ at 0.1 "$cbr0 start"
	$ns_ at 2.0 "$self finish 6a-nam"

	$ns_ run
}

# Group join/leave test with dynamics. Node n3 joins the group when an
# upstream link on its shortest path to the source has failed.
Class Test/detailedDM4 -superclass TestSuite
Test/detailedDM4 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6a
	set test_	detailedDM4
	$self next 1
}
Test/detailedDM4 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ rtproto Session
	### Start multicast configuration
	Timer/Iface/Prune set timeout 0.3
	set mproto detailedDM
	set mrthandle [$ns_ mrtproto $mproto  {}]
	### End of multicast  config

	set cbr0 [new Agent/CBR]
	$ns_ attach-agent $node_(n0) $cbr0
	$cbr0 set dst_ 0x8002
	
	set rcvr [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n3) $rcvr
	$ns_ attach-agent $node_(n4) $rcvr
	$ns_ attach-agent $node_(n5) $rcvr
	
	$ns_ at 0.4 "$node_(n4) join-group $rcvr 0x8002"
	$ns_ at 0.7 "$node_(n5) join-group $rcvr 0x8002"
	$ns_ at 1.35 "$node_(n3) join-group $rcvr 0x8002"
	$ns_ at 1.8 "$node_(n3) leave-group $rcvr 0x8002"
	
	### Link between n0 and n1 down at 1.0 up at 1.6
	$ns_ rtmodel-at 1.0 down $node_(n0) $node_(n1)
	$ns_ rtmodel-at 1.6 up $node_(n0) $node_(n1)
	###

	$ns_ at 0.5 "$cbr0 start"
	$ns_ at 2.0 "$self finish 6a-nam"

	$ns_ run
}

# Testing in LAN topologies
# The node with the higher id is made the LAN forwarder in case there are
# more than one possible forwarders (this is decided by the Assert process).
Class Test/detailedDM5 -superclass TestSuite
Test/detailedDM5 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net8a
	set test_	detailedDM5
	$self next 1
}
Test/detailedDM5 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ rtproto Session
	### Start multicast configuration
	Timer/Iface/Prune set timeout 0.3
	Timer/Iface/Deletion set timeout 0.1
	set mproto detailedDM
	set mrthandle [$ns_ mrtproto $mproto  {}]
	### End of multicast  config

	set cbr0 [new Agent/CBR]
	$ns_ attach-agent $node_(n0) $cbr0
	$cbr0 set dst_ 0x8002
	
	set rcvr [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n3) $rcvr
	$ns_ attach-agent $node_(n6) $rcvr
	$ns_ attach-agent $node_(n7) $rcvr

	$ns_ at 0.1 "$cbr0 start"	
	$ns_ at 0.4 "$node_(n6) join-group $rcvr 0x8002"
	$ns_ at 1.1 "$node_(n7) join-group $rcvr 0x8002"
	$ns_ at 1.3 "$node_(n6) leave-group $rcvr 0x8002"
	$ns_ at 1.8 "$node_(n7) leave-group $rcvr 0x8002"
	
	$ns_ at 2.2 "$self finish 7a-nam"
	$ns_ run
}

# Testing in LAN topologies
# The node with the higher id is made the LAN forwarder in case there are
# more than one possible forwarders (this is decided by the Assert process).
Class Test/detailedDM6 -superclass TestSuite
Test/detailedDM6 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net8a
	set test_	detailedDM6
	$self next 1
}
Test/detailedDM6 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ rtproto Session
	### Start multicast configuration
	Timer/Iface/Prune set timeout 0.3
	Timer/Iface/Deletion set timeout 0.1
	set mproto detailedDM
	set mrthandle [$ns_ mrtproto $mproto  {}]
	### End of multicast  config

	set cbr0 [new Agent/CBR]
	$ns_ attach-agent $node_(n0) $cbr0
	$cbr0 set dst_ 0x8002
	
	set rcvr [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n3) $rcvr
	$ns_ attach-agent $node_(n6) $rcvr
	$ns_ attach-agent $node_(n7) $rcvr

	$ns_ at 0.1 "$cbr0 start"	
	$ns_ at 0.4 "$node_(n3) join-group $rcvr 0x8002"
	$ns_ at 1.1 "$node_(n6) join-group $rcvr 0x8002"
	$ns_ at 1.2 "$node_(n3) leave-group $rcvr 0x8002"
	
	### Link between n1 and n3 down at 0.7 up at 1.0
	$ns_ rtmodel-at 0.7 down $node_(n1) $node_(n3)
	$ns_ rtmodel-at 1.0 up $node_(n1) $node_(n3)
	###

	$ns_ at 1.5 "$self finish 7a-nam"
	$ns_ run
}

# Testing in LAN topologies
# The node with the higher id is made the LAN forwarder in case there are
# more than one possible forwarders (this is decided by the Assert process).
Class Test/detailedDM7 -superclass TestSuite
Test/detailedDM7 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net8a
	set test_	detailedDM7
	$self next 1
}
Test/detailedDM7 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ rtproto Session
	### Start multicast configuration
	Timer/Iface/Prune set timeout 0.3
	Timer/Iface/Deletion set timeout 0.1
	set mproto detailedDM
	set mrthandle [$ns_ mrtproto $mproto  {}]
	### End of multicast  config

	set cbr0 [new Agent/CBR]
	$ns_ attach-agent $node_(n0) $cbr0
	$cbr0 set dst_ 0x8002
	
	set rcvr [new Agent/LossMonitor]
	$ns_ attach-agent $node_(n3) $rcvr
	$ns_ attach-agent $node_(n6) $rcvr
	$ns_ attach-agent $node_(n7) $rcvr

	$ns_ at 0.1 "$cbr0 start"	
	$ns_ at 0.8 "$node_(n6) join-group $rcvr 0x8002"
	$ns_ at 1.8 "$node_(n6) leave-group $rcvr 0x8002"
	
	### Link between n0 and n1 down at 0.4 up at 1.1
	$ns_ rtmodel-at 0.4 down $node_(n0) $node_(n1)
	$ns_ rtmodel-at 1.1 up $node_(n0) $node_(n1)
	###

	$ns_ at 1.8 "$self finish 7a-nam"
	$ns_ run
}

TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:



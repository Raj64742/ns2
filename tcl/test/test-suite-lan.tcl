#
# Copyright (c) 1998 University of Southern California.
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 


# To run all tests: test-all-lan
# to run individual test:
# ns test-suite-lan.tcl broadcast
# ns test-suite-lan.tcl routing-flat
# ns test-suite-lan.tcl routing-hier
# ....
#
# To view a list of available tests to run with this script:
# ns test-suite-lan.tcl
#

Class TestSuite

# Routing test using flat routing
Class Test/lan-routing-flat -superclass TestSuite

# Routing test suing hierarchical routing
Class Test/lan-routing-hier -superclass TestSuite

# Broadcast test for Classifier/Mac
Class Test/lan-broadcast -superclass TestSuite

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
	puts "Valid Tests: lan-routing-flat lan-routing-hier lan-broadcast"
	exit 1
}

TestSuite instproc init {} {
	$self instvar ns_ numnodes_ 
	set ns_ [new Simulator]
	$ns_ trace-all [open temp.rands w]
}

TestSuite instproc finish {} {
	$self instvar ns_
	$ns_ flush-trace
	#puts "finishing.."
	exit 0
}

Test/lan-routing-flat instproc init {} {
	$self instvar ns_ testName_ flag_ node_ nodelist_ \
			lan_ node0_ nodex_ tcp0_ ftp0_

	set testName_ routing-flat
	$self next
	set num 3
	for {set i 0} {$i < $num} {incr i} {
		set node_($i) [$ns_ node]
		lappend nodelist_ $node_($i)
	}
	set lan_ [$ns_ make-lan $nodelist_ 10Mb \
			2ms LL Queue/DropTail Mac/Csma/Cd Channel]
	set node0_ [$ns_ node]
	$ns_ duplex-link $node0_ $node_(1) 10Mb 2ms DropTail
	$ns_ duplex-link-op $node0_ $node_(1) orient right
	set nodex_ [$ns_ node]
	$ns_ duplex-link $nodex_ $node_(2) 20Mb 2ms DropTail
	$ns_ duplex-link-op $nodex_ $node_(2) orient left

	set tcp0_ [$ns_ create-connection TCP/Reno $node0_ TCPSink $nodex_ 0]
	$tcp0_ set window_ 15
	
	set ftp0_ [$tcp0_ attach-source FTP]
}

Test/lan-routing-flat instproc run {} {
	$self instvar ns_ ftp0_
	$ns_ at 0.0 "$ftp0_ start"
	$ns_ at 3.0 "$self finish"
	$ns_ run

}

Test/lan-routing-hier instproc init {} {
	$self instvar ns_ testName_ flag_ node_ nodelist_ \
			lan_ node0_ nodex_ tcp0_ ftp0_
	set testName_ routing-hier
	$self next 
	$ns_ set-address-format hierarchical
	set num 3
	AddrParams set domain_num_ 1
	lappend cluster_num 3
	AddrParams set cluster_num_ $cluster_num
	lappend eilastlevel [expr $num + 1] 1 1
	AddrParams set nodes_num_ $eilastlevel

	for {set i 0} {$i < $num} {incr i} {
		set node_($i) [$ns_ node 0.0.[expr $i + 1]]
		lappend nodelist_ $node_($i)
	}

	set lan_ [$ns_ newLan $nodelist_ 10Mb 2ms -address "0.0.0"]

	set node0_ [$ns_ node "0.1.0"]
	$ns_ duplex-link $node0_ $node_(1) 10Mb 2ms DropTail
	$ns_ duplex-link-op $node0_ $node_(1) orient right
	set nodex_ [$ns_ node "0.2.0"]
	$ns_ duplex-link $nodex_ $node_(2) 10Mb 2ms DropTail
	$ns_ duplex-link-op $nodex_ $node_(2) orient left

	set tcp0_ [$ns_ create-connection TCP/Reno $node0_ TCPSink $nodex_ 0]
	$tcp0_ set window_ 15
	
	set ftp0_ [$tcp0_ attach-source FTP]
}

Test/lan-routing-hier instproc run {} {
	$self instvar ns_ ftp0_
	$ns_ at 0.0 "$ftp0_ start"
	$ns_ at 3.0 "$self finish"
	$ns_ run
}
	
Test/lan-broadcast instproc init {} {
	$self instvar ns_ testName_ flag_ node_ nodelist_ \
			lan_ node0_ nodex_ nodey_ udp0_ cbr0_ rcvrx_ rcvry_
	set testName_ lan-broadcast
	$self next
 
	$ns_ multicast on

	set num 3
	for {set i 0} {$i < $num} {incr i} {
		set node_($i) [$ns_ node]
		lappend nodelist_ $node_($i)
	}
	set lan_ [$ns_ newLan $nodelist_ 10Mb 2ms]

	set node0_ [$ns_ node]
	$ns_ duplex-link $node0_ $node_(1) 10Mb 2ms DropTail
	$ns_ duplex-link-op $node0_ $node_(1) orient right
	set nodex_ [$ns_ node]
	$ns_ duplex-link $nodex_ $node_(2) 10Mb 2ms DropTail
	$ns_ duplex-link-op $nodex_ $node_(2) orient left
	set nodey_ [$ns_ node]
	$ns_ duplex-link $nodey_ $node_(0) 10Mb 2ms DropTail
	$ns_ duplex-link-op $nodey_ $node_(0) orient down

	set udp0_ [new Agent/UDP]
	$ns_ attach-agent $node0_ $udp0_
	set cbr0_ [new Application/Traffic/CBR]
	$cbr0_ attach-agent $udp0_
	$udp0_ set dst_ 0x8003

	set rcvrx_ [new Agent/Null]
	$ns_ attach-agent $nodex_ $rcvrx_
	set rcvry_ [new Agent/Null]
	$ns_ attach-agent $nodey_ $rcvry_
	set mproto DM
	set mrthandle [$ns_ mrtproto $mproto  {}]
}

Test/lan-broadcast instproc run {} {
	$self instvar ns_ cbr0_ nodex_ nodey_ rcvrx_ rcvry_
	$ns_ at 0.0 "$nodex_ join-group $rcvrx_ 0x8003"
	$ns_ at 0.0 "$nodey_ join-group $rcvry_ 0x8003"
	$ns_ at 0.1 "$cbr0_ start"
	$ns_ at 3.0 "$self finish"
	$ns_ run
}


proc runtest {arg} {
	set b [llength $arg]
	if {$b == 1} {
		set test $arg
	} elseif {$b == 2} {
		set test [lindex $arg 0]
	} else {
		usage
	}
	switch $test {
		lan-routing-flat -
		lan-routing-hier -
		lan-broadcast {
			set t [new Test/$test]
		}
		default {
			puts stderr "Unknown test $test"
			exit 1
		}
	}
	$t run
}

global argv arg0
runtest $argv




# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
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
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-lan.tcl,v 1.12 2001/05/15 19:08:56 sfloyd Exp $


# To run all tests: test-all-lan
# to run individual test:
# ns test-suite-lan.tcl broadcast
# ns test-suite-lan.tcl routing-flat
# ns test-suite-lan.tcl routing-hier
# ns test-suite-lan.tcl abstract
# ns test-suite-lan.tcl mactrace
# ....
#
# To view a list of available tests to run with this script:
# ns test-suite-lan.tcl
#
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

Class TestSuite

# Routing test using flat routing
Class Test/lan-routing-flat -superclass TestSuite

# Routing test using hierarchical routing
Class Test/lan-routing-hier -superclass TestSuite

# Broadcast test for Classifier/Mac
Class Test/lan-broadcast -superclass TestSuite

# Routing Test using Abstract LAN
Class Test/lan-abstract -superclass TestSuite

# Mac level collision traces using flat tracing
Class Test/lan-mactrace -superclass TestSuite

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
	puts "Valid Tests: lan-routing-flat lan-routing-hier lan-broadcast lan-abstract lan-mactrace"
	exit 1
}

TestSuite instproc init {} {
	$self instvar ns_ numnodes_ 
	set ns_ [new Simulator]
	$ns_ trace-all [open temp.rands w]
	$ns_ namtrace-all [open temp.rands.nam w]
}

TestSuite instproc finish {} {
	$self instvar ns_
	global quiet

	$ns_ flush-trace
	if { !$quiet } {
		puts "running nam..."
		exec ../../../nam-1/nam temp.rands.nam &
	}
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
	set lan_ [$ns_ make-lan $nodelist_ 10Mb 2ms LL Queue/DropTail Mac/802_3 Channel]
	set node0_ [$ns_ node]
	$ns_ duplex-link $node0_ $node_(1) 10Mb 2ms DropTail
	$ns_ duplex-link-op $node0_ $node_(1) orient right
	set nodex_ [$ns_ node]
	$ns_ duplex-link $nodex_ $node_(2) 20Mb 2ms DropTail
	$ns_ duplex-link-op $nodex_ $node_(2) orient left

	set tcp0_ [$ns_ create-connection TCP/Reno $node0_ TCPSink $nodex_ 0]
	$tcp0_ set window_ 15
	
	set ftp0_ [$tcp0_ attach-app FTP]
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
	# We are switching to the new format, i.e., node-config.
	# Do not test the old method based on node_factory_ here
	# since this is not a backward compatibility test. See 
	# test-suite-hier-routing.tcl for such tests.
	$ns_ node-config -addressType hierarchical
	#$ns_ set-address-format hierarchical
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
	
	set ftp0_ [$tcp0_ attach-app FTP]
}

Test/lan-routing-hier instproc run {} {
	$self instvar ns_ ftp0_
	$ns_ at 0.0 "$ftp0_ start"
	$ns_ at 3.0 "$self finish"
	$ns_ run
}
	
Test/lan-broadcast instproc init {} {
	$self instvar ns_ testName_ flag_ node_ nodelist_ lan_ 
	$self instvar node0_ nodex_ nodey_ udp0_ cbr0_ rcvrx_ rcvry_
	set testName_ lan-broadcast
	$self next
 
	set num 3
	for {set i 0} {$i < $num} {incr i} {
		set node_($i) [$ns_ node]
		lappend nodelist_ $node_($i)
	}
	set lan_ [$ns_ newLan $nodelist_ 10Mb 2ms]
	##[$lan_ set mcl_] set bcast_ 1
	
	set node0_ [$ns_ node]
	$ns_ duplex-link $node_(0) $node0_ 10Mb 2ms DropTail
	$ns_ duplex-link-op $node_(0) $node0_ orient left
	set nodex_ [$ns_ node]
	$ns_ duplex-link $node_(1) $nodex_ 10Mb 1.414ms DropTail
	$ns_ duplex-link-op $node_(1) $nodex_ orient right-up
	$ns_ duplex-link $node_(2) $nodex_ 10Mb 1.414ms DropTail
	$ns_ duplex-link-op $node_(2) $nodex_ orient right-down

	set udp0_ [new Agent/UDP]
	$ns_ attach-agent $node0_ $udp0_
	set cbr0_ [new Application/Traffic/CBR]
	$cbr0_ attach-agent $udp0_
	
	set rcvrz_ [new Agent/Null]
	$ns_ attach-agent $nodex_ $rcvrz_
	$ns_ connect $udp0_ $rcvrz_
}

Test/lan-broadcast instproc run {} {
	$self instvar ns_ cbr0_ nodex_ nodey_ rcvrx_ rcvry_
	$ns_ at 0.1 "$cbr0_ start"
	$ns_ at 2.0 "$self finish"
	$ns_ run
}


Test/lan-abstract instproc init {} {
	$self instvar ns_ testName_ flag_ node_ nodelist_ \
			lan_ tcp0_ tcp1_ ftp0_ ftp1_

	set testName_ lan-abstract
	$self next
	set num 4
	for {set i 0} {$i < $num} {incr i} {
		set node_($i) [$ns_ node]
		lappend nodelist_ $node_($i)
	}
	set lan_ [$ns_ make-abslan $nodelist_ 10Mb 1ms ]

	set tcp0_ [$ns_ create-connection TCP/Reno $node_(0) TCPSink $node_(1) 0]
	$tcp0_ set window_ 15
	set ftp0_ [$tcp0_ attach-app FTP]

	set tcp1_ [$ns_ create-connection TCP/Reno $node_(2) TCPSink $node_(3) 0]
	$tcp1_ set window_ 15
	set ftp1_ [$tcp1_ attach-app FTP]
}

Test/lan-abstract instproc run {} {
	$self instvar ns_ ftp0_ ftp1_
	$ns_ at 0.0 "$ftp0_ start"
	$ns_ at 0.0 "$ftp1_ start"
	$ns_ at 3.0 "$self finish"
	$ns_ run

}


Test/lan-mactrace instproc init {} {
	$self instvar ns_ testName_ flag_ node_ nodelist_ \
			lan_ udp0_ udp1_ cbr0_ cbr1_

	set testName_ lan-mactrace
	$self next
	set num 4
	for {set i 0} {$i < $num} {incr i} {
		set node_($i) [$ns_ node]
		lappend nodelist_ $node_($i)
	}
	set lan_ [$ns_ make-lan -trace on $nodelist_ 10Mb 1ms LL Queue/DropTail Mac/802_3 Channel Phy/WiredPhy ]

	set udp0_ [new Agent/UDP]
	$ns_ attach-agent $node_(0) $udp0_
	set null0_ [new Agent/Null]
	$ns_ attach-agent $node_(1) $null0_
	set cbr0_ [new Application/Traffic/CBR]
	$cbr0_ set interval_ 0.2
	$ns_ connect $udp0_ $null0_
	$cbr0_ attach-agent $udp0_
	
	set udp1_ [new Agent/UDP]
	$ns_ attach-agent $node_(2) $udp1_
	set null1_ [new Agent/Null]
	$ns_ attach-agent $node_(3) $null1_
	set cbr1_ [new Application/Traffic/CBR]
	$cbr1_ set interval_ 0.2
	$ns_ connect $udp1_ $null1_
	$cbr1_ attach-agent $udp1_
	
}

Test/lan-mactrace instproc run {} {
	$self instvar ns_ cbr0_ cbr1_
	$ns_ at 0.0 "$cbr0_ start"
	$ns_ at 0.0 "$cbr1_ start"
	$ns_ at 3.0 "$self finish"
	$ns_ run

}


proc runtest {arg} {
	global quiet
	set quiet 0

	set b [llength $arg]
	if {$b == 1} {
		set test $arg
	} elseif {$b == 2} {
		set test [lindex $arg 0]
		set q [lindex $arg 1]
		if { $q == "QUIET" } {
			set quiet 1
		} else usage
	} else usage

	switch $test {
		lan-routing-flat -
		lan-routing-hier -
		lan-broadcast -
		lan-abstract -
		lan-mactrace {
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












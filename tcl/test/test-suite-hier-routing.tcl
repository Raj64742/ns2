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


# This test suite is for validating hierarchical routing
# To run all tests: test-all-hier-routing
# to run individual test:
# ns test-suite-hier-routing.tcl hier-simple
# ns test-suite-hier-routing.tcl hier-cmcast
# ....
#
# To view a list of available test to run with this script:
# ns test-suite-hier-routing.tcl
#

# Each of the tests uses a 10 node hierarchical topology
#

Class TestSuite

Class Test/hier-simple -superclass TestSuite
# Simple hierarchical routing

Class Test/hier-cmcast -superclass TestSuite
# hierarchical routing with CentralisedMcast

#Class Test/hier-deDM -superclass TestSuite
# hier rtg with DetailedDM

Class Test/hier-session -superclass TestSuite
# session simulations using hierarchical routing



proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
	puts "Valid Tests: hier-simple hier-cmcast hier-deDM \
			hier-session"
	exit 1
}

TestSuite instproc init {} {
    $self instvar ns_ n_ g_ flag_ testName_
    if {$testName_ == "hier-session"} {
	set ns_ [new SessionSim]
	set g_ [Node allocaddr]
	$ns_ namtrace-all [open temp.rands w]
    } else {
	set ns_ [new Simulator]
	$ns_ trace-all [open temp.rands w]
	$ns_ namtrace-all [open temp.rands.nam w]
    }
   # $ns_ trace-all [open temp.rands w]
    $ns_ set-address-format hierarchical
    if {$flag_} {
	$ns_ multicast on
	set g_ [Node allocaddr]
	}
	#setup hierarchical topology
	AddrParams set domain_num_ 2
	lappend cluster_num 2 2
	AddrParams set cluster_num_ $cluster_num
	lappend eilastlevel 2 3 2 3
	AddrParams set nodes_num_ $eilastlevel
	set naddr {0.0.0 0.0.1 0.1.0 0.1.1 0.1.2 1.0.0 1.0.1 1.1.0 \
			1.1.1 1.1.2}
	for {set i 0} {$i < 10} {incr i} {
		set n_($i) [$ns_ node [lindex $naddr $i]]
	}
	$ns_ duplex-link $n_(0) $n_(1) 5Mb 2ms DropTail
	$ns_ duplex-link $n_(1) $n_(2) 5Mb 2ms DropTail
	$ns_ duplex-link $n_(2) $n_(3) 5Mb 2ms DropTail
	$ns_ duplex-link $n_(2) $n_(4) 5Mb 2ms DropTail
	$ns_ duplex-link $n_(1) $n_(5) 5Mb 2ms DropTail
	$ns_ duplex-link $n_(5) $n_(6) 5Mb 2ms DropTail
	$ns_ duplex-link $n_(6) $n_(7) 5Mb 2ms DropTail
	$ns_ duplex-link $n_(7) $n_(8) 5Mb 2ms DropTail
	$ns_ duplex-link $n_(8) $n_(9) 5Mb 2ms DropTail

}

TestSuite instproc finish {} {
	$self instvar ns_
	global quiet

	$ns_ flush-trace
        if { !$quiet } {
                puts "running nam..."
                exec nam temp.rands.nam &
        }
	#puts "finishing.."
	exit 0
}

Test/hier-simple instproc init {flag} {
	$self instvar ns_ testName_ flag_
	set testName_ hier-simple
	set flag_ $flag
	$self next
}

Test/hier-simple instproc run {} {
	$self instvar ns_ n_ 
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $n_(0) $udp0
	set cbr0 [new Application/Traffic/CBR]
	$cbr0 attach-agent $udp0
	set udp1 [new Agent/UDP]
	$ns_ attach-agent $n_(2) $udp1
	$udp1 set class_ 1
	set cbr1 [new Application/Traffic/CBR]
	$cbr1 attach-agent $udp1

	set null0 [new Agent/Null]
	$ns_ attach-agent $n_(8) $null0
	set null1 [new Agent/Null]
	$ns_ attach-agent $n_(6) $null1

	$ns_ connect $udp0 $null0
	$ns_ connect $udp1 $null1
	$ns_ at 1.0 "$cbr0 start"
	$ns_ at 1.1 "$cbr1 start"
	$ns_ at 3.0 "$self finish"
	$ns_ run

}

Test/hier-cmcast instproc init {flag} {
	$self instvar ns_ testName_ flag_
	set testName_ hier-cmcast
	set flag_ $flag
	$self next 
}

Test/hier-cmcast instproc run {} {
	$self instvar ns_ n_ g_
	set mproto CtrMcast
	set mrthandle [$ns_ mrtproto $mproto {}]
	$ns_ at 0.0 "$mrthandle switch-treetype $g_" 
	
	for {set i 1} {$i < 10} {incr i} {
		set rcvr($i) [new Agent/Null]
		$ns_ attach-agent $n_($i) $rcvr($i)
		$ns_ at $i "$n_($i) join-group $rcvr($i) $g_"
		incr i
	}
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $n_(0) $udp0
	$udp0 set dst_ $g_
	$udp0 set class_ 2
	set sender [new Application/Traffic/CBR]
	$sender attach-agent $udp0
	$ns_ at 0.0 "$sender start"
	$ns_ at 10.0 "$self finish"
	
	$ns_ run
	
}
	
#Test/hier-deDM instproc init {flag} {
#	  $self instvar ns_ testName_ flag_
#	  set testName_ hier-detailedDM
#	  set flag_ $flag
#	  $self next 
#}
#
#Test/hier-deDM instproc run {} {
#	  $self instvar ns_ n_ g_
#	  set mproto detailedDM
#	  set mrthandle [$ns_ mrtproto $mproto {}]
#	  
#	  for {set i 1} {$i < 10} {incr i} {
#		  set rcvr($i) [new Agent/LossMonitor]
#		  $ns_ attach-agent $n_($i) $rcvr($i)
#		  $ns_ at $i "$n_($i) join-group $rcvr($i) $g_"
#		  incr i
#	  }
#	  set udp0 [new Agent/UDP]
#	  $ns_ attach-agent $n_(0) $udp0
#	  $udp0 set dst_ $g_
#	  $udp0 set class_ 2
#	  set sender [new Application/Traffic/CBR]
#	  $sender attach-agent $udp0
#	  $ns_ at 0.1 "$sender start"
#	  $ns_ at 10.0 "$self finish"
#	  
#	  $ns_ run
#}


Test/hier-session instproc init {flag} {
	$self instvar ns_ testName_ flag_
	set testName_ hier-session
	set flag_ $flag
	$self next 
}

Test/hier-session instproc run {} {
	$self instvar ns_ n_ g_
	
	for {set i 1} {$i < 10} {incr i} {
		set rcvr($i) [new Agent/LossMonitor]
		$ns_ attach-agent $n_($i) $rcvr($i)
		$ns_ at $i "$n_($i) join-group $rcvr($i) $g_"
		incr i
	}
	set udp0 [new Agent/UDP]
	$ns_ attach-agent $n_(0) $udp0
	$udp0 set dst_ $g_
	set sender [new Application/Traffic/CBR]
	$sender attach-agent $udp0
	$ns_ create-session $n_(0) $udp0
	$ns_ at 0.1 "$sender start"
	$ns_ at 10.0 "$self finish"
	
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
	    if {[lindex $arg 1] == "QUIET"} {
		set quiet 1
	    }
	} else {
		usage
	}
	switch $test {
		hier-simple -
		hier-session {
			set t [new Test/$test 0]
		}
		hier-deDM -
		hier-cmcast {
			set t [new Test/$test 1]
		}
		default {
			stderr "Unknown test $test"
			exit 1
		}
	}
	$t run
}

global argv arg0
runtest $argv










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


# This test suite is for validating SRM
# To run all tests: test-all-srm
# to run individual test:
# ns test-suite-srm.tcl chain
# ns test-suite-srm.tcl star
# ....
#
# To view a list of available test to run with this script:
# ns test-suite-srm.tcl
#

Class TestSuite

Class Test/srm-chain -superclass TestSuite
# Simple chain topology

Class Test/srm-star -superclass TestSuite
# Simple star topology

Class Test/adapt-rep-timer -superclass TestSuite
# simple 8 node star topology, runs for 50s, tests Adaptive repair timers.

Class Test/adapt-req-timer -superclass TestSuite
# simple 8 node star topology, runs for 50s, tests Adaptive request timers.

Class Test/srm-chain-session -superclass TestSuite
# session simulations using srm in chain topo



proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
	puts "Valid Tests: srm-chain srm-star adapt-rep-timer adapt-req-timer\
              srm-chain-session"

	exit 1
}

TestSuite instproc init {} {
    $self instvar ns_ n_ g_ testName_ topo_ net_
    if {$testName_ == "srm-chain-session"} {
	set ns_ [new SessionSim]
	$ns_ namtrace-all [open temp.rands w]
    } else {
	set ns_ [new Simulator]
	$ns_ trace-all [open temp.rands w]
    }
    Simulator set EnableMcast_ 1
    Simulator set NumberInterfaces_ 1
    set g_ [Node allocaddr]

    set topo_ [new Topology/$net_ $ns_]
    for {set i 0} {$i <= [$topo_ totalnodes?]} {incr i} { 
	set n_($i) [$topo_ node? $i]
    }
}

Class Topology

Topology instproc init {
    $self instvar nmax_ n_
}

Topology instproc totalnodes? {
    $self instvar nmax_ 
    return $nmax_
}

Topology instproc node? num {
    $self instvar n_
    return n_($num)
}

class Topology/chain5 -superclass Topology

Topology/chain5 instproc init ns {
    $self instvar nmax_ n_
    set nmax_ 5
    for {set i 0} {$i <= $nmax_} {incr i} {
	set n_($i) [$ns node]
    }
    $self next
    set chainMax [expr $nmax - 1]
    set j 0
    for {set i 1} {$i <= $chainMax} {incr i} {
	$ns duplex-link $n_($i) $n_($j) 1.5Mb 10ms DropTail
	$ns duplex-link-op $n_($j) $n_($i) orient right
	set j $i
    }
    $ns duplex-link $n_([expr $nmax - 2]) $n_($nmax) 1.5Mb 10ms DropTail
    $ns duplex-link-op $n_([expr $nmax - 2]) $n_($nmax) orient right-up
    $ns duplex-link-op $n_([expr $nmax - 2]) $n_([expr $nmax-1]) orient right-down

    $ns queue-limit $n_(0) $n_(1) 2	;# q-limit is 1 more than max #packets in q.
    $ns queue-limit $n_(1) $n_(0) 2 

}

TestSuite instproc finish {src} {
	$self instvar ns_
    $src stop
    $ns_ flush-trace
    #puts "finishing.."
    exit 0
}

Test/srm-chain instproc init {
    $self instvar ns_ testName_ net_
    set testName_ srm-chain
    set net_ chain5
    $self next
}

Test/srm-chain instproc run {} {
    $self instvar ns_ n_ g_
    set mh [$ns_ mrtproto CtrMcast {}]
    $ns at 0.3 "$mh switch-treetype $g_"
    
    # now the multicast, and the agents
    set fid 0
    for {set i 0} {$i <= 5} {incr i} {
	set srm($i) [new Agent/SRM/$srmSimType]
	$srm($i) set dst_ $group
	$srm($i) set fid_ [incr fid]
	$ns at 1.0 "$srm($i) start"
	$ns attach-agent $n($i) $srm($i)
    }
    # Attach a data source to srm(1)
    set packetSize 800
    set s [new Agent/CBR]
    $s set interval_ 0.04
    $s set packetSize_ $packetSize
    $srm(0) traffic-source $s
    $srm(0) set packetSize_ $packetSize
    $s set fid_ 0
    $ns at 3.5 "$srm(0) start-source"

    #$ns rtmodel-at 3.519 down $n(0) $n(1)	;# this ought to drop exactly one
    #$ns rtmodel-at 3.521 up   $n(0) $n(1)	;# data packet?
    set loss_module [new SRMErrorModel]
    $loss_module drop-packet 2 10 1
    $loss_module drop-target [$ns set nullAgent_]
    $ns at 1.25 "$ns lossmodel $loss_module $n(0) $n(1)"

    $ns_ at 4.0 "finish"
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
	
Test/hier-deDM instproc init {flag} {
	$self instvar ns_ testName_ flag_
	set testName_ hier-detailedDM
	set flag_ $flag
	$self next 
}

Test/hier-deDM instproc run {} {
	$self instvar ns_ n_ g_
	set mproto detailedDM
	set mrthandle [$ns_ mrtproto $mproto {}]
	
	for {set i 1} {$i < 10} {incr i} {
		set rcvr($i) [new Agent/LossMonitor]
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
	$ns_ at 0.1 "$sender start"
	$ns_ at 10.0 "$self finish"
	
	$ns_ run
}


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
	set b [llength $arg]
	if {$b == 1} {
		set test $arg
	} elseif {$b == 2} {
		set test [lindex $arg 0]
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




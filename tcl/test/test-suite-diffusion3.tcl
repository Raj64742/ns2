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

# To run all tests: test-all-diffusion3
# to run individual test:
# ns test-suite-diffusion3.tcl simple-ping

# To view a list of available test to run with this script:
# ns test-suite-diffusion3.tcl

# This test validates a simple diffusion (ping) application

Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.

# ======================================================================
# Define options
# ======================================================================
global opt
set opt(chan)		Channel/WirelessChannel
set opt(prop)		Propagation/TwoRayGround
set opt(netif)		Phy/WirelessPhy
set opt(mac)		Mac/802_11
set opt(ifq)		Queue/DropTail/PriQueue
set opt(ll)		LL
set opt(ant)            Antenna/OmniAntenna

set opt(ifqlen)		50		;# max packet in ifq
set opt(seed)		0.0
set opt(lm)             "off"           ;# log movement

set opt(x)		670	;# X dimension of the topography
set opt(y)		670     ;# Y dimension of the topography
set opt(nn)		3	;# number of nodes
set opt(stop)		50	;# simulation time
set opt(prestop)        198      ;# time to prepare to stop
set opt(adhocRouting)   Directed_Diffusion
# ======================================================================

LL set mindelay_		50us
LL set delay_			25us
LL set bandwidth_		0	;# not used

Agent/Null set sport_		0
Agent/Null set dport_		0

Agent/CBR set sport_		0
Agent/CBR set dport_		0

Agent/TCPSink set sport_	0
Agent/TCPSink set dport_	0

Agent/TCP set sport_		0
Agent/TCP set dport_		0
Agent/TCP set packetSize_	1460

Queue/DropTail/PriQueue set Prefer_Routing_Protocols    1

# unity gain, omni-directional antennas
# set up the antennas to be centered in the node and 1.5 meters above it
Antenna/OmniAntenna set X_ 0
Antenna/OmniAntenna set Y_ 0
Antenna/OmniAntenna set Z_ 1.5
Antenna/OmniAntenna set Gt_ 1.0
Antenna/OmniAntenna set Gr_ 1.0

# Initialize the SharedMedia interface with parameters to make
# it work like the 914MHz Lucent WaveLAN DSSS radio interface
Phy/WirelessPhy set CPThresh_ 10.0
Phy/WirelessPhy set CSThresh_ 1.559e-11
Phy/WirelessPhy set RXThresh_ 3.652e-10
Phy/WirelessPhy set Rb_ 2*1e6
Phy/WirelessPhy set Pt_ 0.2818
Phy/WirelessPhy set freq_ 914e+6 
Phy/WirelessPhy set L_ 1.0

# ======================================================================

Class TestSuite

proc usage {} {
    global argv0
    puts stderr "usage: ns $argv0 <tests> "
    puts "Valid Tests: simple-ping"
    exit 1
}

TestSuite instproc init {} {
    global opt tracefd quiet
    $self instvar ns_
    set ns_ [new Simulator]
    set tracefd [open temp.rands w]
    $ns_ trace-all $tracefd
    # stealing seed from another test-suite
    ns-random 188312339
} 


Class Test/simple-ping -superclass TestSuite

Test/simple-ping instproc init {} {
    $self instvar ns_ testName_
    set testName_ simple-ping
    $self next
}

Test/simple-ping instproc run {} {
    global opt
    $self instvar ns_ topo god_

    set topo	[new Topography]
    $topo load_flatgrid $opt(x) $opt(y)
    set god_ [create-god $opt(nn)]

    $ns_ node-config -adhocRouting $opt(adhocRouting) \
		 -llType $opt(ll) \
		 -macType $opt(mac) \
		 -ifqType $opt(ifq) \
		 -ifqLen $opt(ifqlen) \
		 -antType $opt(ant) \
		 -propType $opt(prop) \
		 -phyType $opt(netif) \
		 -channelType $opt(chan) \
		 -topoInstance $topo \
		 -agentTrace ON \
                 -routerTrace ON \
                 -macTrace ON 
             
    for {set i 0} {$i < $opt(nn) } {incr i} {
	set node_($i) [$ns_ node $i]
        $node_($i) color black
	$node_($i) random-motion 0		;# disable random motion
        $god_ new_node $node_($i)
    }

# defining node positions
    $node_(0) set X_ 500.716707738489
    $node_(0) set Y_ 620.707335765875
    $node_(0) set Z_ 0.000000000000
    
    $node_(1) set X_ 320.192740186325
    $node_(1) set Y_ 450.384818286195
    $node_(1) set Z_ 0.000000000000

    #3rd node NOT in hearing range of other two
    $node_(2) set X_ 177.438972165239
    $node_(2) set Y_ 245.843469852367
    $node_(2) set Z_ 0.000000000000

    #Diffusion application - ping src
    set src_(0) [new Application/DiffApp/PingSender]
    $ns_ attach-diffapp $node_(0) $src_(0)
    $ns_ at 0.123 "$src_(0) publish"

    #Diffusion application - ping sink
    set snk_(0) [new Application/DiffApp/PingReceiver]
    $ns_ attach-diffapp $node_(2) $snk_(0)
    $ns_ at 1.456 "$snk_(0) subscribe"

    #
    # Tell nodes when the simulation ends
    #
    for {set i 0} {$i < $opt(nn) } {incr i} {
	$ns_ at $opt(stop).000000001 "$node_($i) reset";
    }
    $ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"
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
	set t [new Test/$test]
	$t run
}

global argv arg0
runtest $argv

#
# Copyright (c) 1995-1997 The Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the Computer Systems
#	Engineering Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-adaptive-red.tcl,v 1.4 2001/07/18 16:11:45 sfloyd Exp $
#
# To run all tests: test-all-adaptive-red

set dir [pwd]
catch "cd tcl/test"
source misc_simple.tcl
catch "cd $dir"

set flowfile fairflow.tr; # file where flow data is written
set flowgraphfile fairflow.xgr; # file given to graph tool 

TestSuite instproc finish file {
	global quiet PERL
	$self instvar ns_ tchan_ testName_
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
          $PERL ../../bin/raw2xg -a -s 0.01 -m 90 -t $file > temp.rands
	if {$quiet == "false"} {
        	exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
	}
        ## now use default graphing tool to make a data file
        ## if so desired

	if { [info exists tchan_] && $quiet == "false" } {
		$self plotQueue $testName_
	}
	$ns_ halt
}

TestSuite instproc enable_tracequeue ns {
	$self instvar tchan_ node_
	set redq [[$ns link $node_(r1) $node_(r2)] queue]
	set tchan_ [open all.q w]
	$redq trace curq_
	$redq trace ave_
	$redq attach $tchan_
}

Class Topology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}

Class Topology/net2 -superclass Topology
Topology/net2 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]    
    set node_(r1) [$ns node]    
    set node_(r2) [$ns node]    
    set node_(s3) [$ns node]    
    set node_(s4) [$ns node]    

    $self next 

    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 25
    $ns queue-limit $node_(r2) $node_(r1) 25
    # A 1500-byte packet has a transmission time of 0.008 seconds.
    # A queue of 25 1500-byte packets would be 0.2 seconds. 
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
}   

Class Topology/net3 -superclass Topology
Topology/net3 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]    
    set node_(r1) [$ns node]    
    set node_(r2) [$ns node]    
    set node_(s3) [$ns node]    
    set node_(s4) [$ns node]    

    $self next 

    $ns duplex-link $node_(s1) $node_(r1) 10Mb 0ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 1ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 10ms RED
    $ns duplex-link $node_(r2) $node_(r1) 1.5Mb 10ms RED
    $ns queue-limit $node_(r1) $node_(r2) 100
    $ns queue-limit $node_(r2) $node_(r1) 100
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 2ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 3ms DropTail
}   

TestSuite instproc plotQueue file {
	global quiet
	$self instvar tchan_
	#
	# Plot the queue size and average queue size, for RED gateways.
	#
	set awkCode {
		{
			if ($1 == "Q" && NF>2) {
				print $2, $3 >> "temp.q";
				set end $2
			}
			else if ($1 == "a" && NF>2)
				print $2, $3 >> "temp.a";
		}
	}
	set f [open temp.queue w]
	puts $f "TitleText: $file"
	puts $f "Device: Postscript"

	if { [info exists tchan_] } {
		close $tchan_
	}
	exec rm -f temp.q temp.a 
	exec touch temp.a temp.q

	exec awk $awkCode all.q

	puts $f \"queue
	exec cat temp.q >@ $f  
	puts $f \n\"ave_queue
	exec cat temp.a >@ $f
	###puts $f \n"thresh
	###puts $f 0 [[ns link $r1 $r2] get thresh]
	###puts $f $end [[ns link $r1 $r2] get thresh]
	close $f
	if {$quiet == "false"} {
		exec xgraph -bb -tk -x time -y queue temp.queue &
	}
}

TestSuite instproc tcpDumpAll { tcpSrc interval label } {
    global quiet
    $self instvar dump_inst_ ns_
    if ![info exists dump_inst_($tcpSrc)] {
	set dump_inst_($tcpSrc) 1
	set report $label/window=[$tcpSrc set window_]/packetSize=[$tcpSrc set packetSize_]
	if {$quiet == "false"} {
		puts $report
	}
	$ns_ at 0.0 "$self tcpDumpAll $tcpSrc $interval $label"
	return
    }
    $ns_ at [expr [$ns_ now] + $interval] "$self tcpDumpAll $tcpSrc $interval $label"
    set report time=[$ns_ now]/class=$label/ack=[$tcpSrc set ack_]/packets_resent=[$tcpSrc set nrexmitpack_]
    if {$quiet == "false"} {
    	puts $report
    }
}       

TestSuite instproc setTopo {} {
    $self instvar node_ net_ ns_ 

    set topo_ [new Topology/$net_ $ns_]
    set node_(s1) [$topo_ node? s1]
    set node_(s2) [$topo_ node? s2]
    set node_(s3) [$topo_ node? s3]
    set node_(s4) [$topo_ node? s4]
    set node_(r1) [$topo_ node? r1]
    set node_(r2) [$topo_ node? r2]
    [$ns_ link $node_(r1) $node_(r2)] trace-dynamics $ns_ stdout
}

TestSuite instproc maketraffic {} {
    $self instvar ns_ node_ testName_ net_
    set stoptime 50.0

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    $tcp1 set window_ 15

    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 1]
    $tcp2 set window_ 15

    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 3.0 "$ftp2 start"

    $self tcpDump $tcp1 5.0
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"
}

#####################################################################

Class Test/red1 -superclass TestSuite
Test/red1 instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ red1
    $self next
}
Test/red1 instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
    $self maketraffic 
    $ns_ run
}

Class Test/red1Adapt -superclass TestSuite
Test/red1Adapt instproc init {} {
    $self instvar net_ test_ ns_
    set net_ net2 
    set test_ red1Adapt
    $self next
}
Test/red1Adapt instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
    set forwq [[$ns_ link $node_(r1) $node_(r2)] queue]
    $forwq set adaptive_ 1 
    $self maketraffic
    $ns_ run   
}

#####################################################################

# THIS NEEDS TO REUSE CONNECTION STATE!
TestSuite instproc newtraffic { num window packets start interval conns} {
    $self instvar ns_ node_ testName_ net_

    for {set i 0} {$i < $conns } {incr i} {
       	set tcp($i) [$ns_ create-connection-list TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 2]
	set source($i) [lindex $tcp($i) 0]
	set sink($i) [lindex $tcp($i) 1]
       	$source($i) set window_ $window
	set ftp($i) [$source($i) attach-app FTP]
	$ns_ at 0.0 "$ftp($i) produce 0"
    }
    for {set i 0} {$i < $num } {incr i} {
        set tcpnum [ expr $i % $conns ]
	set time [expr $start + $i * $interval]
	$ns_ at $time  "$source($tcpnum) reset"
	$ns_ at $time  "$sink($tcpnum) reset"
        $ns_ at $time  "$ftp($tcpnum) producemore $packets"
    }
}

Class Test/red2 -superclass TestSuite
Test/red2 instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ red2
    $self next
}
Test/red2 instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
    $self maketraffic 
    $self newtraffic 20 20 1000 25 0.1 20
    $ns_ run
}

Class Test/red2Adapt -superclass TestSuite
Test/red2Adapt instproc init {} {
    $self instvar net_ test_ ns_
    set net_ net2 
    set test_ red2Adapt
    $self next
}
Test/red2Adapt instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
    set forwq [[$ns_ link $node_(r1) $node_(r2)] queue]
    $forwq set adaptive_ 1 
    $self maketraffic
    $self newtraffic 20 20 1000 25 0.1 20
    $ns_ run   
}
Class Test/red2A-Adapt -superclass TestSuite
Test/red2A-Adapt instproc init {} {
    $self instvar net_ test_ ns_
    set net_ net2 
    set test_ red2A-Adapt
    Queue/RED set alpha_ 0.02
    Queue/RED set beta_ 0.8
    Test/red2A-Adapt instproc run {} [Test/red2Adapt info instbody run ]
    $self next
}

#####################################################################

Class Test/red3 -superclass TestSuite
Test/red3 instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ red3
    $self next
}
Test/red3 instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
    $self maketraffic 
    $self newtraffic 15 20 300 0 0.1 15
    $ns_ run
}

Class Test/red3Adapt -superclass TestSuite
Test/red3Adapt instproc init {} {
    $self instvar net_ test_ ns_
    set net_ net2 
    set test_ red3Adapt
    $self next
}
Test/red3Adapt instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
    set forwq [[$ns_ link $node_(r1) $node_(r2)] queue]
    $forwq set adaptive_ 1 
    $self maketraffic
    $self newtraffic 15 20 300 0 0.1 15
    $ns_ run   
}

Class Test/red4Adapt -superclass TestSuite
Test/red4Adapt instproc init {} {
    $self instvar net_ test_ ns_
    set net_ net2 
    set test_ red4Adapt
    Queue/RED set alpha_ 0.02
    Queue/RED set beta_ 0.8
    Test/red4Adapt instproc run {} [Test/red3Adapt info instbody run ]
    $self next
}

TestSuite instproc printall { fmon } {
        puts "aggregate per-link total_drops [$fmon set pdrops_]"
        puts "aggregate per-link total_packets [$fmon set pdepartures_]"
}


# Class Test/red5 -superclass TestSuite
# Test/red5 instproc init {} {
#     $self instvar net_ test_ ns_
#     set net_ net2 
#     set test_ red5
#     Queue/RED set alpha_ 0.02
#     Queue/RED set beta_ 0.8
#     $self next
# }
# Test/red5 instproc run {} {
#     $self instvar ns_ node_ testName_ net_
#     $self setTopo
#     set slink [$ns_ link $node_(r1) $node_(r2)]; # link to collect stats on
#     set fmon [$ns_ makeflowmon Fid]
#     $ns_ attach-fmon $slink $fmon
#     $self maketraffic
#     $self newtraffic 20 20 300 0 0.001 10
#     # To run many flows:
#     # $self newtraffic 4000 20 300 0 0.005 500
#     # $self newtraffic 40000 20 300 0 0.001 500
#     $ns_ at 49.99 "$self printall $fmon" 
#     $ns_ run
# }

#####################################################################

TestSuite runTest

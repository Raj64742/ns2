#
# Copyright (c) 1995 The Regents of the University of California.
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-tcp.tcl,v 1.24 2001/05/11 05:19:34 sfloyd Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-tcp.tcl
#

source misc.tcl
source topologies.tcl
Agent/TCP set minrto_ 0
# The default is being changed to minrto_ 1

TestSuite instproc finish file {
	global quiet PERL
        exec $PERL ../../bin/set_flow_id -s all.tr | \
          $PERL ../../bin/getrc -s 2 -d 3 | \
          $PERL ../../bin/raw2xg -s 0.01 -m 90 -t $file > temp.rands
	if {$quiet == "false"} {
		exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
	}
        ## now use default graphing tool to make a data file
	## if so desired
	## regsub \\(.+\\) $file {} filename
	## exec csh gnuplotA.com temp.rands $filename
        exit 0
}

TestSuite instproc printtimers { tcp time} {
	global quiet
	if {$quiet == "false"} {
        	puts "time: $time sRTT(in ticks): [$tcp set srtt_]/8 RTTvar(in ticks): [$tcp set rttvar_]/4 backoff: [$tcp set backoff_]"
	}
}

TestSuite instproc printtimersAll { tcp time interval } {
        $self instvar dump_inst_ ns_
        if ![info exists dump_inst_($tcp)] {
                set dump_inst_($tcp) 1
                $ns_ at $time "$self printtimersAll $tcp $time $interval"
                return
        }
	set newTime [expr [$ns_ now] + $interval]
	$ns_ at $time "$self printtimers $tcp $time"
        $ns_ at $newTime "$self printtimersAll $tcp $newTime $interval"
}

#
# Links1 uses 8Mb, 5ms feeders, and a 800Kb 10ms bottleneck.
# Queue-limit on bottleneck is 2 packets.
#
Class Topology/net4 -superclass NodeTopology/4nodes
Topology/net4 instproc init ns {
    $self next $ns
    $self instvar node_
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 10ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 2
    $ns queue-limit $node_(k1) $node_(r1) 2
    if {[$class info instprocs config] != ""} {
	$self config $ns
    }
}

#
# Links1 uses 8Mb, 5ms feeders, and a 800Kb 20ms bottleneck.
# Queue-limit on bottleneck is 25 packets.
#
Class Topology/net6 -superclass NodeTopology/4nodes
Topology/net6 instproc init ns {
    $self next $ns
    $self instvar node_
    Queue/RED set setbit_ true
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 20ms RED
    $ns queue-limit $node_(r1) $node_(k1) 25
    $ns queue-limit $node_(k1) $node_(r1) 25
    if {[$class info instprocs config] != ""} {
	$self config $ns
    }
}

# Definition of test-suite tests

# This test shows two TCPs when one is ECN-capable and the other
# is not.

Class Test/ecn -superclass TestSuite
Test/ecn instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	ecn_(one_with_ecn,_one_without)
	Agent/TCP set old_ecn_ 1
	$self next
}
Test/ecn instproc run {} {
	$self instvar ns_ node_ testName_

	# Set up TCP connection
	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 30
	$tcp1 set ecn_ 1
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 start"

	# Set up TCP connection
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 20
	$tcp2 set ecn_ 0
	set ftp2 [$tcp2 attach-app FTP]
	$ns_ at 3.0 "$ftp2 start"

	$self tcpDump $tcp1 5.0
	$self tcpDump $tcp2 5.0

	$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ run
}

# This test shows the retransmit timeout value when the first packet
# of a connection is dropped, and the backoff of the retransmit timer
# as subsequent packets are dropped.
#
# This test also shows that once the retransmit timer is backed off,
# it is later un-backed.

Class Test/timers -superclass TestSuite
Test/timers instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	timers_(first_packet_dropped)
	$self next
}
Test/timers instproc run {} {
	global quiet
	$self instvar ns_ node_ testName_

	if {$quiet == "false"} {puts "tcpTICK: [Agent/TCP set tcpTick_]"}

	# Set up TCP connection
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 0]
	$tcp2 set window_ 3
	set ftp2 [$tcp2 attach-app FTP]
	$ns_ at 0.09 "$ftp2 start"
	$ns_ at 1.0 "$self printtimersAll $tcp2 1.0 1.0" 

	# Set up TCP connection
	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 1]
	$tcp1 set window_ 5
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 produce 800"
	$ns_ at 20.3 "$ftp1 producemore 5"
	$ns_ at 20.7 "$ftp1 producemore 5" 

	$self traceQueues $node_(r1) [$self openTrace 25.0 $testName_]
	$ns_ run
}

Class Test/timersA -superclass TestSuite
Test/timersA instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	timersA_(early_packet_dropped)
	Agent/TCP set timerfix_ false
	$self next
}
#  This one is funny.  
#  No fast retransmit.
#  First retransmit timer expires, packet 2 is retransmitted.  Backoff 1.
#  Ack for 2 comes in.  Karns.  Backoff is still 2.  Send packet 5, set timer.
#  Send packet 6.
#  Ack for 5 comes in.  Reset timer, the reset backoff is 1. Send packet 7.
#  Retransmit timer expires.  Set backoff to 2, reset timer.
#  
Test/timersA instproc run {} {
	global quiet
	$self instvar ns_ node_ testName_

	if {$quiet == "false"} {puts "tcpTICK: [Agent/TCP set tcpTick_]"}

	# Set up TCP connection
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 0]
	$tcp2 set window_ 3
	set ftp2 [$tcp2 attach-app FTP]
	$ns_ at 0.07 "$ftp2 start"
	$ns_ at 0.1 "$self printtimersAll $tcp2 0.1 0.1" 

	# Set up TCP connection
	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 1]
	$tcp1 set window_ 5
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 produce 1600"
	$ns_ at 25.3 "$ftp1 producemore 5"
	$ns_ at 25.7 "$ftp1 producemore 5" 
	$ns_ at 26.1 "$ftp1 producemore 5" 
	$ns_ at 26.5 "$ftp1 producemore 5" 
	$ns_ at 26.9 "$ftp1 producemore 5" 
	$ns_ at 28.8 "$ftp1 producemore 5" 

	$self traceQueues $node_(r1) [$self openTrace 3.5 $testName_]
	$ns_ run
}

Class Test/timersAfix -superclass TestSuite
Test/timersAfix instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	timersAfix_(early_packet_dropped)
	Agent/TCP set timerfix_ true
        Test/timersAfix instproc run {} [Test/timersA info instbody run]
	$self next
}

# Multiply the mean deviation by 8 instead of by 4.
#
Class Test/timersA1 -superclass TestSuite
Test/timersA1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	timersA1_(multiple_of_8_instead_of_4)
	Agent/TCP set rttvar_exp_ 3
        Test/timersA1 instproc run {} [Test/timersA info instbody run ]
	$self next
}

# timestamps, and tcpTick more fine-grained.
#
Class Test/timersA2 -superclass TestSuite
Test/timersA2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	timersA2_(timestamps,_fine-grained_clock)
        Agent/TCP set timestamps_ true
	Agent/TCP set tcpTick_ 0.001
        Test/timersA2 instproc run {} [Test/timersA info instbody run ]
	$self next
}

# Update the smoothed round-trip with weight 1/16 instead of 1/8.
#
Class Test/timersA3 -superclass TestSuite
Test/timersA3 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	timersA3_(weight_1/32_instead_of_1/8)
	Agent/TCP set timestamps_ true
	Agent/TCP set tcpTick_ 0.001
	Agent/TCP set T_SRTT_BITS 5
	Agent/TCP set T_RTTVAR_BITS 4
        Test/timersA3 instproc run {} [Test/timersA info instbody run ]
	$self next
}

Class Test/timers1 -superclass TestSuite
Test/timers1 instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo  
        set defNet_     net2
        set test_       timers1_(tcpTick_=default,0.1)
        $self next
}
Test/timers1 instproc run {} {
	global quiet
        $self instvar ns_ node_ testName_

	if {$quiet == "false"} {puts "tcpTICK: [Agent/TCP set tcpTick_]"}
        $ns_ queue-limit $node_(r1) $node_(r2) 29

        set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink/DelAck $node_(r2) 0]
#        set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink/DelAck $node_(r2) 1]
        $tcp1 set window_ 40
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 produce 180"
	$ns_ at 1.0 "$self printtimersAll $tcp1 1.0 1.0" 
	$self traceQueues $node_(r1) [$self openTrace 4.0 $testName_]
        $ns_ run
}

Class Test/timers2 -superclass TestSuite
Test/timers2 instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo  
        set defNet_     net2
        set test_       timers2_(tcpTick_=0.5)
        $self next
}

Test/timers2 instproc run {} {
	global quiet
        $self instvar ns_ node_ testName_

	set tick 0.5
	if {$quiet == "false"} {puts "tcpTICK: $tick"}
	Agent/TCP set tcpTick_ $tick 
        $ns_ queue-limit $node_(r1) $node_(r2) 29

        set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink/DelAck $node_(r2) 0]
        $tcp1 set window_ 40
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 produce 180"
	$ns_ at 1.0 "$self printtimersAll $tcp1 1.0 1.0" 
	$self traceQueues $node_(r1) [$self openTrace 4.0 $testName_]
        $ns_ run
}

Class Test/timers3 -superclass TestSuite
Test/timers3 instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo  
        set defNet_     net2
        set test_       timers3_(tcpTick_=0.001)
        $self next
}

Test/timers3 instproc run {} {
	global quiet
        $self instvar ns_ node_ testName_

	set tick 0.001
	if {$quiet == "false"} {puts "tcpTICK: $tick"}
	Agent/TCP set tcpTick_ $tick 
        $ns_ queue-limit $node_(r1) $node_(r2) 29

        set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink/DelAck $node_(r2) 0]
        $tcp1 set window_ 40
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 produce 180"
	$ns_ at 1.0 "$self printtimersAll $tcp1 1.0 1.0" 
	$self traceQueues $node_(r1) [$self openTrace 4.0 $testName_]
        $ns_ run
}

Class Test/timers4 -superclass TestSuite
Test/timers4 instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo  
        set defNet_     net2
        set test_       timers4_(tcpTick_=0.001)
        $self next
}

Test/timers4 instproc run {} {
	global quiet
        $self instvar ns_ node_ testName_

	set tick 0.001
	if {$quiet == "false"} {puts "tcpTICK: $tick"}
	Agent/TCP set tcpTick_ $tick 
        $ns_ queue-limit $node_(r1) $node_(r2) 29

        set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink/DelAck $node_(r2) 0]
        $tcp1 set window_ 10
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"
	$ns_ at 0.1 "$self printtimersAll $tcp1 0.1 0.1" 
	$self traceQueues $node_(r1) [$self openTrace 2.0 $testName_]
        $ns_ run
}

Class Test/timers5 -superclass TestSuite
Test/timers5 instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo  
        set defNet_     net2
        set test_       timers5_(tcpTick_=0.001)
        $self next
}

Test/timers5 instproc run {} {
	global quiet
        $self instvar ns_ node_ testName_

	set tick 0.001
	if {$quiet == "false"} {puts "tcpTICK: $tick"}
	Agent/TCP set tcpTick_ $tick 
        $ns_ queue-limit $node_(r1) $node_(r2) 29

        set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink/DelAck $node_(r2) 0]
        $tcp1 set window_ 2
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"
	$ns_ at 0.1 "$self printtimersAll $tcp1 0.1 0.1" 
	$self traceQueues $node_(r1) [$self openTrace 2.0 $testName_]
        $ns_ run
}

Class Test/stats1 -superclass TestSuite
Test/stats1 instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net0
        set test_       stats1
        $self next
} 
Test/stats1 instproc printtcp { label tcp time } {
	puts ""
	puts "tcp: $label time: $time" 
	puts "total_data_packets_sent: [$tcp set ndatapack_] data_bytes_sent: [$tcp set ndatabytes_]" 
	puts "packets_resent: [$tcp set nrexmitpack_] bytes_resent: [$tcp set nrexmitbytes_]" 
	puts "ack_packets_received: [$tcp set nackpack_]"
	puts "retransmit_timeouts: [$tcp set nrexmit_]" 

}
Test/stats1 instproc run {} {
        $self instvar ns_ node_ testName_

        $ns_ delay $node_(s2) $node_(r1) 200ms
        $ns_ delay $node_(r1) $node_(s2) 200ms
        $ns_ queue-limit $node_(r1) $node_(k1) 10
        $ns_ queue-limit $node_(k1) $node_(r1) 10

        set stoptime 10.1

        set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 0]
        $tcp1 set window_ 30
        set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
        $tcp2 set window_ 3
        set ftp1 [$tcp1 attach-app FTP]
        set ftp2 [$tcp2 attach-app FTP]
 
        $ns_ at 1.0 "$ftp1 start" 
        $ns_ at 1.0 "$ftp2 start"

        $self tcpDumpAll $tcp1 10.0 tcp1
 	$self tcpDumpAll $tcp2 10.0 tcp2

	$ns_ at 10.0 "$self printtcp 1 $tcp1 10.0" 
	$ns_ at 10.0 "$self printtcp 2 $tcp2 10.0" 
	$ns_ at 10.0 "puts \"\""
 
        # trace only the bottleneck link 
        $self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
        $ns_ run
}

TestSuite instproc set_lossylink {} {
    	$self instvar lossylink_ ns_ node_
    	set lossylink_ [$ns_ link $node_(r1) $node_(k1)]
   	set em [new ErrorModule Fid]
    	set errmodel [new ErrorModel/Periodic]
    	$errmodel unit pkt
    	$lossylink_ errormodule $em
}

TestSuite instproc emod {} {
	$self instvar lossylink_
        set errmodule [$lossylink_ errormodule]
        return $errmodule
}

TestSuite instproc drop_pkts pkts {
	$self instvar ns_
	set emod [$self emod]
	set errmodel1 [new ErrorModel/List]
	$errmodel1 droplist $pkts
	$emod insert $errmodel1
	$emod bind $errmodel1 1
} 

TestSuite instproc run1 { tcp0 {stoptime 30.1}} {
        $self instvar ns_ node_ testName_

	set count 100 
	set count1 3

    	set ftp0 [$tcp0 attach-app FTP]
    	$ns_ at 0.0  "$ftp0 produce $count" 
	$ns_ at 2.4  "$ftp0 producemore $count"  
	$ns_ at 2.5  "$ftp0 producemore $count"
	$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
	$ns_ run
}

Class Test/quiescent_100ms -superclass TestSuite
Test/quiescent_100ms instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       quiescent_100ms
	Agent/TCP set QOption_ 0
        $self next
} 
Test/quiescent_100ms instproc run {} {
        $self instvar ns_ node_ 
	Agent/TCP set packetSize_ 100 
	Agent/TCP set window_ 25
	set tcp0 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$self run1 $tcp0
}

Class Test/quiescentB -superclass TestSuite
Test/quiescentB instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       quiescentB
	Agent/TCP set QOption_ 0
        $self next
} 
Test/quiescentB instproc run {} {
        $self instvar ns_ node_ 
	$self set_lossylink
	Agent/TCP set packetSize_ 100 
	Agent/TCP set window_ 25
	set tcp0 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 1]
	$self drop_pkts {2}
	$self run1 $tcp0 3.0
}

Class Test/quiescentB_qoption -superclass TestSuite
Test/quiescentB_qoption instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       quiescentB_qoption
	Agent/TCP set QOption_ 1
        $self next
} 
Test/quiescentB_qoption instproc run {} {
        $self instvar ns_ node_ 
	$self set_lossylink
	Agent/TCP set packetSize_ 100 
	Agent/TCP set window_ 25
	set tcp0 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 1]
	$self drop_pkts {2}
	$self run1 $tcp0 3.0
}


TestSuite instproc run2 { tcp0 {stoptime 3.0}} { 
        $self instvar ns_ node_ testName_
	set count 25
	set count1 10
	set count2 3
	set count3 100

    	set data0 [$tcp0 attach-app Telnet]
	$data0 set interval_ 0.0005
	$ns_ at 0 "$data0 start"
	$ns_ at 0.045 "$data0 set interval_ 0.01"
	$ns_ at 2.2 "$data0 set interval_ 0.0005" 
	$ns_ at 2.5 "$data0 stop"
	$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
	$ns_ run
}

Class Test/underutilized_100ms -superclass TestSuite
Test/underutilized_100ms instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       underutilized_100ms
	Agent/TCP set QOption_ 0
        $self next
} 
Test/underutilized_100ms instproc run {{sender TCP} {receiver TCPSink}} {
        $self instvar ns_ node_ 
	Agent/TCP set packetSize_ 100 
	Agent/TCP set window_ 100
	set tcp0 [$ns_ create-connection $sender $node_(s1) $receiver $node_(k1) 0]
	$self run2 $tcp0
}

Class Test/underutilized_100ms_control -superclass TestSuite
Test/underutilized_100ms_control instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       underutilized_100ms_control
        Agent/TCP set QOption_ 0
        Agent/TCP set control_increase_ 1
        Test/underutilized_100ms_control instproc run {{sender TCP} {receiver TCPSink}} [Test/underutilized_100ms info instbody run ]
        $self next
}

Class Test/underutilized_100ms_control_Q -superclass TestSuite
Test/underutilized_100ms_control_Q instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       underutilized_100ms_control_Q
        Agent/TCP set QOption_ 1
        Agent/TCP set control_increase_ 1
        Test/underutilized_100ms_control_Q instproc run {{sender TCP} {receiver TCPSink}} [Test/underutilized_100ms info instbody run ]
        $self next
}

Class Test/underutilized_100ms_Q -superclass TestSuite
Test/underutilized_100ms_Q instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       underutilized_100ms_Q
        Agent/TCP set QOption_ 1
        Agent/TCP set control_increase_ 0
        Test/underutilized_100ms_Q instproc run {{sender TCP} {receiver TCPSink}} [Test/underutilized_100ms info instbody run ]
        $self next
}

Class Test/underutilized_100ms_control_Reno -superclass TestSuite
Test/underutilized_100ms_control_Reno instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       underutilized_100ms_control_Reno
        Agent/TCP set QOption_ 0
        Agent/TCP set control_increase_ 1
        Test/underutilized_100ms_control_Reno instproc run {{sender TCP/Reno} {receiver TCPSink}} [Test/underutilized_100ms info instbody run ]
        $self next
}

Class Test/underutilized_100ms_control_Newreno -superclass TestSuite
Test/underutilized_100ms_control_Newreno instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       underutilized_100ms_control_Newreno
        Agent/TCP set QOption_ 0
        Agent/TCP set control_increase_ 1
        Test/underutilized_100ms_control_Newreno instproc run {{sender TCP/Reno} {receiver TCPSink}} [Test/underutilized_100ms info instbody run ]
        $self next
}

Class Test/underutilized_100ms_control_Sack -superclass TestSuite
Test/underutilized_100ms_control_Sack instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       underutilized_100ms_control_Sack
        Agent/TCP set QOption_ 0
        Agent/TCP set control_increase_ 1
        Test/underutilized_100ms_control_Sack instproc run {{sender TCP/Sack1} {receiver TCPSink/Sack1}} [Test/underutilized_100ms info instbody run ]
        $self next
}

Class Test/quiescent_100ms_fine -superclass TestSuite
Test/quiescent_100ms_fine instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       quiescent_100ms_fine(EnblRTTCtr__0)
	Agent/TCP set QOption_ 1
	Agent/TCP set control_increase_ 1
	Agent/TCP set EnblRTTCtr_ 0
	Test/quiescent_100ms_fine instproc run {} [Test/quiescent_100ms info instbody run ]
        $self next
} 

Class Test/quiescent_100ms_coarse -superclass TestSuite
Test/quiescent_100ms_coarse instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       quiescent_100ms_coarse(EnblRTTCtr__1)
	Agent/TCP set QOption_ 1
	Agent/TCP set control_increase_ 1
	Agent/TCP set EnblRTTCtr_ 1
	Test/quiescent_100ms_coarse instproc run {} [Test/quiescent_100ms info instbody run ]
        $self next
} 

Class Test/quiescent_1ms_fine -superclass TestSuite
Test/quiescent_1ms_fine instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       quiescent_1ms_fine(EnblRTTCtr__0)
	Agent/TCP set QOption_ 1
	Agent/TCP set tcpTick_ 0.001 
	Agent/TCP set control_increase_ 1
	Agent/TCP set EnblRTTCtr_ 0 
	Test/quiescent_1ms_fine instproc run {} [Test/quiescent_100ms info instbody run ]
        $self next
} 

Class Test/quiescent_1ms_coarse -superclass TestSuite
Test/quiescent_1ms_coarse instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       quiescent_1ms_coarse(EnblRTTCtr__1)
	Agent/TCP set QOption_ 1
	Agent/TCP set tcpTick_ 0.001 
	Agent/TCP set control_increase_ 1
	Agent/TCP set EnblRTTCtr_ 1 
	Test/quiescent_1ms_coarse instproc run {} [Test/quiescent_100ms info instbody run ]
        $self next
} 

Class Test/quiescent_500ms_fine -superclass TestSuite
Test/quiescent_500ms_fine instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       quiescent_500ms_fine(EnblRTTCtr__0)
	Agent/TCP set QOption_ 1
	Agent/TCP set tcpTick_ 0.500
	Agent/TCP set control_increase_ 1
	Agent/TCP set EnblRTTCtr_ 0 
	Test/quiescent_500ms_fine instproc run {} [Test/quiescent_100ms info instbody run ]
        $self next
} 

Class Test/quiescent_500ms_coarse -superclass TestSuite
Test/quiescent_500ms_coarse instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       quiescent_500ms_coarse(EnblRTTCtr__1)
	Agent/TCP set QOption_ 1
	Agent/TCP set tcpTick_ 0.500 
	Agent/TCP set control_increase_ 1
	Agent/TCP set EnblRTTCtr_ 1 
	Test/quiescent_500ms_coarse instproc run {} [Test/quiescent_100ms info instbody run ]
        $self next
} 

TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:

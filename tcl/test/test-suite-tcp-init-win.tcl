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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-tcp-init-win.tcl,v 1.4 1998/04/21 23:45:11 sfloyd Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-tcp.tcl
#

source misc.tcl
source topologies.tcl

TestSuite instproc finish file {
	global quiet
        exec ../../bin/set_flow_id -s all.tr | \
          ../../bin/getrc -s 2 -d 3 | \
          ../../bin/raw2xg -s 0.01 -m 90 -t $file > temp.rands
	if {$quiet == "false"} {
		exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
	}
        ## now use default graphing tool to make a data file
	## if so desired
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
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 100ms RED
    $ns queue-limit $node_(r1) $node_(k1) 25
    $ns queue-limit $node_(k1) $node_(r1) 25
    if {[$class info instprocs config] != ""} {
	$self config $ns
    }
}

# Definition of test-suite tests

TestSuite instproc run_test {tcp1 tcp2 dumptime runtime window} {
	$self instvar ns_ node_ testName_

	$tcp1 set window_ 8
	$tcp1 set windowInitOption_ 1
	set ftp1 [$tcp1 attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"

	$tcp2 set window_ 8
	$tcp2 set windowInitOption_ 2
	set ftp2 [$tcp2 attach-source FTP]
	$ns_ at 0.0 "$ftp2 start"

	$self tcpDump $tcp1 $dumptime
	$self tcpDump $tcp1 $dumptime
	$self traceQueues $node_(r1) [$self openTrace 1.0 $testName_]
	$ns_ run
}

TestSuite instproc make_tcp {nodeA nodeB ID type} {
	$self instvar ns_ node_ 
        if {$type == "Tahoe"} {
		set tcp [$ns_ create-connection TCP $node_($nodeA) TCPSink $node_($nodeB) $ID]
	}
        if {$type == "Reno"} {
		set tcp [$ns_ create-connection TCP/Reno $node_($nodeA) TCPSink $node_($nodeB) $ID]
	}
        if {$type == "Newreno"} {
		set tcp [$ns_ create-connection TCP/Newreno $node_($nodeA) TCPSink $node_($nodeB) $ID]
	}
        if {$type == "Sack"} {
		set tcp [$ns_ create-connection TCP/Sack1 $node_($nodeA) TCPSink/Sack1 $node_($nodeB) $ID]
	}
	return $tcp
}

TestSuite instproc second_test {tcp1 tcp2} {
	$self instvar ns_ node_ testName_
	$tcp1 set window_ 40
	set ftp1 [$tcp1 attach-source FTP]
	$ns_ at 1.0 "$ftp1 start"

	$tcp2 set window_ 40
	set ftp2 [$tcp2 attach-source FTP]
	$ns_ at 0.0 "$ftp2 start"

	$self tcpDump $tcp1 5.0
	$self tcpDump $tcp2 5.0

	$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ run
}

Class Test/init1 -superclass TestSuite
Test/init1 instproc init topo {
	$self instvar net_ defNet_ test_ 
	set net_	$topo
	set defNet_	net6
	set test_	init1(PacketSize=1000)
        $self next
}
Test/init1 instproc run {} {
        $self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 1000
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
	set tcp1 [$self make_tcp s1 k1 0 Tahoe]
	set tcp2 [$self make_tcp s2 k1 1 Tahoe]
	$self run_test $tcp1 $tcp2 1.0 1.0 8
}

Class Test/init2 -superclass TestSuite
Test/init2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	init2(PacketSize=1500)
	$self next
}
Test/init2 instproc run {} {
	$self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 1500
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
	set tcp1 [$self make_tcp s1 k1 0 Tahoe] 
	set tcp2 [$self make_tcp s2 k1 1 Tahoe] 
	$tcp1 set syn_ false
	$tcp1 set delay_growth_ false
	$self run_test $tcp1 $tcp2 1.0 1.0 8
}

Class Test/init3 -superclass TestSuite
Test/init3 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	init3(PacketSize=4000)
	$self next
}
Test/init3 instproc run {} {
	$self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 4000
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
        set tcp1 [$self make_tcp s1 k1 0 Tahoe] 
        set tcp2 [$self make_tcp s2 k1 1 Tahoe]
	$tcp1 set syn_ false
	$tcp1 set delay_growth_ false
	$tcp1 set windowInit_ 6
	$self run_test $tcp1 $tcp2 1.0 1.0 8
}

Class Test/init3a -superclass TestSuite
Test/init3a instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	init3a(PacketSize=4000)
	$self next
}
Test/init3a instproc run {} {
	$self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 4000
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
        set tcp1 [$self make_tcp s1 k1 0 Tahoe] 
        set tcp2 [$self make_tcp s2 k1 1 Tahoe]
	$tcp1 set windowInit_ 6
	$self run_test $tcp1 $tcp2 1.0 1.0 8
}

Class Test/init4 -superclass TestSuite
Test/init4 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	init4(PacketSize=1000)
	$self next
}
Test/init4 instproc run {} {
	$self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 1000
	Agent/TCP set windowInitOption_ 2
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true

        set tcp1 [$self make_tcp s1 k1 0 Tahoe] 
        set tcp2 [$self make_tcp s2 k1 1 Tahoe]
	$self second_test $tcp1 $tcp2
}

Class Test/init5 -superclass TestSuite
Test/init5 instproc init topo {
	$self instvar net_ defNet_ test_ 
	set net_	$topo
	set defNet_	net6
	set test_	init5(PacketSize=1000)
        $self next
}
Test/init5 instproc run {} {
        $self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 1000
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
	set tcp1 [$self make_tcp s1 k1 0 Reno]
	set tcp2 [$self make_tcp s2 k1 1 Reno]
	$self run_test $tcp1 $tcp2 1.0 1.0 8
}

Class Test/init6 -superclass TestSuite
Test/init6 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	init6(PacketSize=1500)
	$self next
}
Test/init6 instproc run {} {
	$self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 1500
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
	set tcp1 [$self make_tcp s1 k1 0 Reno] 
	set tcp2 [$self make_tcp s2 k1 1 Reno] 
	$tcp1 set syn_ false
	$tcp1 set delay_growth_ false
	$self run_test $tcp1 $tcp2 1.0 1.0 8
}

Class Test/init7 -superclass TestSuite
Test/init7 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	init7(PacketSize=4000)
	$self next
}
Test/init7 instproc run {} {
	$self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 4000
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
        set tcp1 [$self make_tcp s1 k1 0 Reno] 
        set tcp2 [$self make_tcp s2 k1 1 Reno]
	$tcp1 set syn_ false
	$tcp1 set delay_growth_ false
	$tcp1 set windowInit_ 6
	$self run_test $tcp1 $tcp2 1.0 1.0 8
}

Class Test/init8 -superclass TestSuite
Test/init8 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	init8(PacketSize=1000)
	$self next
}
Test/init8 instproc run {} {
	$self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 1000
	Agent/TCP set windowInitOption_ 2
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true

        set tcp1 [$self make_tcp s1 k1 0 Reno] 
        set tcp2 [$self make_tcp s2 k1 1 Reno]
	$self second_test $tcp1 $tcp2
}

Class Test/init9 -superclass TestSuite
Test/init9 instproc init topo {
	$self instvar net_ defNet_ test_ 
	set net_	$topo
	set defNet_	net6
	set test_	init9(PacketSize=1000)
        $self next
}
Test/init9 instproc run {} {
        $self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 1000
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
	set tcp1 [$self make_tcp s1 k1 0 Newreno]
	set tcp2 [$self make_tcp s2 k1 1 Newreno]
	$self run_test $tcp1 $tcp2 1.0 1.0 8
}

Class Test/init10 -superclass TestSuite
Test/init10 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	init10(PacketSize=1500)
	$self next
}
Test/init10 instproc run {} {
	$self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 1500
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
	set tcp1 [$self make_tcp s1 k1 0 Newreno] 
	set tcp2 [$self make_tcp s2 k1 1 Newreno] 
	$tcp1 set syn_ false
	$tcp1 set delay_growth_ false
	$self run_test $tcp1 $tcp2 1.0 1.0 8
}

Class Test/init11 -superclass TestSuite
Test/init11 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	init11(PacketSize=4000)
	$self next
}
Test/init11 instproc run {} {
	$self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 4000
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
        set tcp1 [$self make_tcp s1 k1 0 Newreno] 
        set tcp2 [$self make_tcp s2 k1 1 Newreno]
	$tcp1 set syn_ false
	$tcp1 set delay_growth_ false
	$tcp1 set windowInit_ 6
	$self run_test $tcp1 $tcp2 1.0 1.0 8
}

Class Test/init12 -superclass TestSuite
Test/init12 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	init12(PacketSize=1000)
	$self next
}
Test/init12 instproc run {} {
	$self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 1000
	Agent/TCP set windowInitOption_ 2
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true

        set tcp1 [$self make_tcp s1 k1 0 Newreno] 
        set tcp2 [$self make_tcp s2 k1 1 Newreno]
	$self second_test $tcp1 $tcp2
}

Class Test/init13 -superclass TestSuite
Test/init13 instproc init topo {
	$self instvar net_ defNet_ test_ 
	set net_	$topo
	set defNet_	net6
	set test_	init13(PacketSize=1000)
        $self next
}
Test/init13 instproc run {} {
        $self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 1000
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
	set tcp1 [$self make_tcp s1 k1 0 Sack]
	set tcp2 [$self make_tcp s2 k1 1 Sack]
	$self run_test $tcp1 $tcp2 1.0 1.0 8
}

Class Test/init14 -superclass TestSuite
Test/init14 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	init14(PacketSize=1500)
	$self next
}
Test/init14 instproc run {} {
	$self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 1500
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
	set tcp1 [$self make_tcp s1 k1 0 Sack] 
	set tcp2 [$self make_tcp s2 k1 1 Sack] 
	$tcp1 set syn_ false
	$tcp1 set delay_growth_ false
	$self run_test $tcp1 $tcp2 1.0 1.0 8
}

Class Test/init15 -superclass TestSuite
Test/init15 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	init15(PacketSize=4000)
	$self next
}
Test/init15 instproc run {} {
	$self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 4000
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true
        set tcp1 [$self make_tcp s1 k1 0 Sack] 
        set tcp2 [$self make_tcp s2 k1 1 Sack]
	$tcp1 set syn_ false
	$tcp1 set delay_growth_ false
	$tcp1 set windowInit_ 6
	$self run_test $tcp1 $tcp2 1.0 1.0 8
}

Class Test/init16 -superclass TestSuite
Test/init16 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	init16(PacketSize=1000)
	$self next
}
Test/init16 instproc run {} {
	$self instvar ns_ node_ testName_
	Agent/TCP set packetSize_ 1000
	Agent/TCP set windowInitOption_ 2
	Agent/TCP set syn_ true
	Agent/TCP set delay_growth_ true

        set tcp1 [$self make_tcp s1 k1 0 Sack] 
        set tcp2 [$self make_tcp s2 k1 1 Sack]
	$self second_test $tcp1 $tcp2
}

TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:

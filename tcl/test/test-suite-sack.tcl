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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-sack.tcl,v 1.1 1997/04/28 19:31:34 kannan Exp $
#
#
# This test suite reproduces most of the tests from the following note:
# Floyd, S., Simulator Tests. July 1995.  
# URL ftp://ftp.ee.lbl.gov/papers/simtests.ps.Z.
#
# To run all tests:  test-all
# To run individual tests:
# ns test-suite.tcl tahoe1
# ns test-suite.tcl tahoe2
# ...
#
#awk '{print $2, $1, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12}' out2 > t

set dir [pwd]
catch "cd tcl/test"
source topologies.tcl
source misc.tcl
catch "cd $dir"

TestSuite instproc openTraces { stopTime testName filename node1 node2 } {
        $self instvar ns_
	exec rm -f $filename.tr $filename.tr1 temp.rands
	set traceFile [open $filename.tr w]
	puts $traceFile "v testName $testName"
	$ns_ at $stopTime \
		"close $traceFile" 
	$ns_ trace-queue $node1 $node2 $traceFile

	set traceFile1 [open $filename.tr1 w]
	$ns_ at $stopTime \
		"close $traceFile1 ; $self finish $testName"
	$ns_ trace-queue $node2 $node1 $traceFile1

	$self instvar node_
	if [info exists node_(b1)] {
		$ns_ trace-queue $node1 $node_(b1) $traceFile
		$ns_ trace-queue $node_(b1) $node2 $traceFile1
	}
}

#XXX need to flush pending output
Agent/TCP instproc error msg {
	$self instvar addr_
	puts stderr "tcp source at [expr $addr_ >> 8]: $msg"
	exit 1
}

# single packet drop
Class Test/sack1 -superclass TestSuite
Test/sack1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	sack1
	$self next
}

Test/sack1 instproc run {} {
	$self instvar ns_ node_ testName_

	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
	$tcp1 set window 14
	set ftp1 [$tcp1 attach-source FTP]
	$ns_ at 1.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# trace only the bottleneck link
        $self openTraces 5.0 $testName_ out $node_(r1) $node_(k1)

	$ns_ run
}

# three packet drops
Class Test/sack1a -superclass TestSuite
Test/sack1a instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	sack1a
	$self next
}

Test/sack1a instproc run {} {
	$self instvar ns_ node_ testName_

	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
	$tcp1 set window 20
	set ftp1 [$tcp1 attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$self openTraces 5.0 $testName_ out $node_(r1) $node_(k1)

	$ns_ run
}

Class Test/sack1b -superclass TestSuite
Test/sack1b instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	sack1b
	$self next
}

Test/sack1b instproc run {} {
	$self instvar ns_ node_ testName_
	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
	$tcp1 set window 26
	set ftp1 [$tcp1 attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$self openTraces 5.0 $testName_ out $node_(r1) $node_(k1)

	$ns_ run
}

Class Test/sack1c -superclass TestSuite
Test/sack1c instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	sack1c
	$self next
}

Test/sack1c instproc run {} {
	$self instvar ns_ node_ testName_
	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
	$tcp1 set window 27
	set ftp1 [$tcp1 attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$self openTraces 5.0 $testName_ out $node_(r1) $node_(k1)

	$ns_ run
}

# this does not seem right
Class Test/sack3 -superclass TestSuite
Test/sack3 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	sack3
	$self next
}

Test/sack3 instproc run {} {
	$self instvar ns_ node_ testName_
	$ns_ queue-limit $node_(r1) $node_(k1) 8
	$ns_ queue-limit $node_(k1) $node_(r1) 8
	
	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
	$tcp1 set window 100
	$tcp1 set bugFix_ false

	set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
	$tcp2 set window 16
	$tcp2 set bugFix_ false

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 0.5 "$ftp2 start"

	$self tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$self openTraces 8.0 $testName_ out $node_(r1) $node_(k1)

	$ns_ run
}

Class Test/sack4 -superclass TestSuite
Test/sack4 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	sack4
	$self next
}

Test/sack4 instproc run {} {
	$self instvar ns_ node_ testName_
	$ns_ delay $node_(s2) $node_(r1) 200ms
	$ns_ delay $node_(r1) $node_(s2) 200ms
	$ns_ queue-limit $node_(r1) $node_(k1) 11
	$ns_ queue-limit $node_(k1) $node_(r1) 11
	
	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
	$tcp1 set window 30

	set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
	$tcp2 set window 30

	set ftp1 [$tcp1 attach-source FTP]
	## $ftp1 set maxpkts 10
	set ftp2 [$tcp2 attach-source FTP]
	## $ftp2 set maxpkts 30

	$ns_ at 0.0 "$ftp1 start"
	$ns_ at 0.0 "$ftp2 start"

	$self tcpDump $tcp1 5.0

	# trace only the bottleneck link
	$self openTraces 25.0 $testName_ out $node_(r1) $node_(k1)

	$ns_ run
}

Class Test/sack5 -superclass TestSuite
Test/sack5 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net1
	set test_	sack5
	$self next
}

Test/sack5 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ delay $node_(s1) $node_(r1) 3ms
	$ns_ delay $node_(r1) $node_(s1) 3ms

	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
	$tcp1 set window 50
	$tcp1 set bugFix_ false

	set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
	$tcp2 set window 50
	$tcp2 set bugFix_ false

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 1.75 "$ftp2 produce 100"

	$self tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$self openTraces 6.0 $testName_ out $node_(r1) $node_(k1)

	$ns_ run
}

# shows a long recovery from sack.
Class Test/sackB2 -superclass TestSuite
Test/sackB2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	sackB2
	$self next
}

Test/sackB2 instproc run {} {
	$self instvar ns_ node_ testName_
	$ns_ queue-limit $node_(r1) $node_(k1) 9

	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
	$tcp1 set window 50

	set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
	$tcp2 set window 20

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 1.0 "$ftp2 start"

	$self tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$self openTraces 10.0 $testName_ out $node_(r1) $node_(k1)

	$ns_ run
}

# two packets dropped
Class Test/sackB4 -superclass TestSuite
Test/sackB4 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net2
	set test_	sackB4
	$self next
}

Test/sackB4 instproc run {} {
        $self instvar ns_ node_ testName_
        $ns_ queue-limit $node_(r1) $node_(r2) 29
        set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(r2) 0]
        $tcp1 set window 40

        set ftp1 [$tcp1 attach-source FTP]
        $ns_ at 0.0 "$ftp1 start"

        $self tcpDump $tcp1 1.0

        # trace only the bottleneck link
	$self openTraces 2.0 $testName_ out $node_(r1) $node_(r2)

        $ns_ run
}

# delayed ack not implemented yet
#Class Test/delayedSack -superclass TestSuite
#Test/delayedSack instproc init topo {
#	$self instvar net_ defNet_ test_
#	set net_	$topo
#	set defNet_	net0
#	set test_	delayedSack
#	$self next
#}
#
#Test/delayedSack instproc run {} {
# 	$self instvar ns_ node_ testName_
# 	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
# 	$tcp1 set window 50
# 
# 	# lookup up the sink and set it's delay interval
# 	[$node_(k1) agent [$tcp1 dst-port]] set interval 100ms
# 
# 	set ftp1 [$tcp1 attach-source FTP]
# 	$ns_ at 1.0 "$ftp1 start"
# 
# 	$self tcpDump $tcp1 1.0
# 
# 	# trace only the bottleneck link
# 	[$self openTraces 4.0 $testName_ out $node_(r1) $node_(k1)]
# 
# 	$ns_ run
# }

# segregation
Class Test/phaseSack -superclass TestSuite
Test/phaseSack instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	phaseSack
	$self next
}

Test/phaseSack instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ delay $node_(s2) $node_(r1) 3ms
	$ns_ delay $node_(r1) $node_(s2) 3ms
	$ns_ queue-limit $node_(r1) $node_(k1) 16
	$ns_ queue-limit $node_(k1) $node_(r1) 100

	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
	$tcp1 set window 32

	set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
	$tcp2 set window 32

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns_ at 5.0 "$ftp1 start"
	$ns_ at 1.0 "$ftp2 start"

	$self tcpDump $tcp1 5.0

	# trace only the bottleneck link
	$self openTraces 25.0 $testName_ out $node_(r1) $node_(k1)

	$ns_ run
}

# random overhead, but segregation remains 
Class Test/phaseSack2 -superclass TestSuite
Test/phaseSack2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	phaseSack2
	$self next
}

Test/phaseSack2 instproc run {} {
        $self instvar ns_ node_ testName_

        $ns_ delay $node_(s2) $node_(r1) 3ms
        $ns_ delay $node_(r1) $node_(s2) 3ms
        $ns_ queue-limit $node_(r1) $node_(k1) 16
        $ns_ queue-limit $node_(k1) $node_(r1) 100

        set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
        $tcp1 set window 32
        $tcp1 set overhead 0.01

        set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
        $tcp2 set window 32
        $tcp2 set overhead 0.01
        
        set ftp1 [$tcp1 attach-source FTP]
        set ftp2 [$tcp2 attach-source FTP]
 
        $ns_ at 5.0 "$ftp1 start"
        $ns_ at 1.0 "$ftp2 start"
        
        $self tcpDump $tcp1 5.0
 
        # trace only the bottleneck link
    $self openTraces 25.0 $testName_ out $node_(r1) $node_(k1)

        $ns_ run
}

# no segregation, because of random overhead
Class Test/phaseSack3 -superclass TestSuite
Test/phaseSack3 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	phaseSack3
	$self next
}

Test/phaseSack3 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ delay $node_(s2) $node_(r1) 9.5ms
	$ns_ delay $node_(r1) $node_(s2) 9.5ms
	$ns_ queue-limit $node_(r1) $node_(k1) 16
	$ns_ queue-limit $node_(k1) $node_(r1) 100

	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
	$tcp1 set window 32
        $tcp1 set overhead 0.01

	set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
	$tcp2 set window 32
        $tcp2 set overhead 0.01

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns_ at 5.0 "$ftp1 start"
	$ns_ at 1.0 "$ftp2 start"

	$self tcpDump $tcp1 5.0

	# trace only the bottleneck link
        $self openTraces 25.0 $testName_ out $node_(r1) $node_(k1)

	$ns_ run
}

#Class Test/timersSack -superclass TestSuite
#Test/timersSack instproc init topo {
#	$self instvar net_ defNet_ test_
#	set net_	$topo
#	set defNet_	net0
#	set test_	timersSack
#	$self next
#}
#
#Test/timersSack instproc run {} {
# 	$self instvar ns_ node_ testName_
# 	$ns_ queue-limit $node_(r1) $node_(k1) 2
# 	$ns_ queue-limit $node_(k1) $node_(r1) 100
# 
# 	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
# 	$tcp1 set window 4
# 	# lookup up the sink and set it's delay interval
# 	[$node_(k1) agent [$tcp1 dst-port]] set interval 100ms
# 
# 	set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
# 	$tcp2 set window 4
# 	# lookup up the sink and set it's delay interval
# 	[$node_(k1) agent [$tcp2 dst-port]] set interval 100ms
# 
# 	set ftp1 [$tcp1 attach-source FTP]
# 	set ftp2 [$tcp2 attach-source FTP]
# 
# 	$ns_ at 1.0 "$ftp1 start"
# 	$ns_ at 1.3225 "$ftp2 start"
# 
# 	$self tcpDump $tcp1 5.0
# 
# 	# trace only the bottleneck link
# 	$self openTraces 10.0 $testName_ out $node_(r1) $node_(k1)
# 
# 	$ns_ run
# }

TestSuite runTest


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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-tcpReset.tcl,v 1.3 2001/05/27 02:14:59 sfloyd Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-tcp.tcl
#

source misc.tcl
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1
source topologies.tcl
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

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

# 
# Arrange for tcp source stats to be dumped for $tcpSrc every
# $interval seconds of simulation time
# 
TestSuite instproc tcpDump { tcpSrc interval } {
        global quiet
        $self instvar dump_inst_ ns_
        if ![info exists dump_inst_($tcpSrc)] {
                set dump_inst_($tcpSrc) 1
                $ns_ at 0.0 "$self tcpDump $tcpSrc $interval"
                return
        }
        $ns_ at [expr [$ns_ now] + $interval] "$self tcpDump $tcpSrc $interval"
        set report [$ns_ now]/cwnd=[format "%.4f" [$tcpSrc set cwnd_]]/ssthresh=[$tcpSrc set ssthresh_]/ack=[$tcpSrc set ack_]
        if {$quiet == "false"} {
                puts $report
        }
}


# Definition of test-suite tests

Class Test/reset -superclass TestSuite
Test/reset instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	reset
	$self next
}
Test/reset instproc run {} {
	global quiet
	$self instvar ns_ node_ testName_

	# Set up TCP connection
	set tcp [$ns_ create-connection-list TCP $node_(s1) TCPSink $node_(k1) 1]
	set tcp1 [lindex $tcp 0]
        set sink [lindex $tcp 1]
	$tcp1 set window_ 5
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 produce 10"
	$ns_ at 1.1 "$tcp1 reset"
	$ns_ at 1.1 "$sink reset"
	$ns_ at 1.5 "$ftp1 producemore 20"
        $self tcpDump $tcp1 1.0

	$self traceQueues $node_(r1) [$self openTrace 3.0 $testName_]
	$ns_ run
}

Class Test/resetDelAck -superclass TestSuite
Test/resetDelAck instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	resetDelAck
	$self next
}
Test/resetDelAck instproc run {} {
	global quiet
	$self instvar ns_ node_ testName_

	# Set up TCP connection
	set tcp [$ns_ create-connection-list TCP $node_(s1) TCPSink/DelAck $node_(k1) 1]
	set tcp1 [lindex $tcp 0]
        set sink [lindex $tcp 1]
	$tcp1 set window_ 5
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 produce 10"
	$ns_ at 1.1 "$tcp1 reset"
	$ns_ at 1.1 "$sink reset"
	$ns_ at 1.5 "$ftp1 producemore 20"

	$self traceQueues $node_(r1) [$self openTrace 3.0 $testName_]
	$ns_ run
}

Class Test/resetDelAck1 -superclass TestSuite
Test/resetDelAck1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net6
	set test_	resetDelAck1
	$self next
}
Test/resetDelAck1 instproc run {} {
	global quiet
	$self instvar ns_ node_ testName_

	# Set up TCP connection
	set tcp [$ns_ create-connection-list TCP $node_(s1) TCPSink/DelAck $node_(k1) 1]
	set tcp1 [lindex $tcp 0]
        set sink [lindex $tcp 1]
	$tcp1 set window_ 5
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.0 "$ftp1 produce 10"
	$ns_ at 0.41 "$tcp1 reset"
	$ns_ at 0.41 "$sink reset"
	$ns_ at 5.00 "$ftp1 producemore 20"

	$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ run
}


TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:

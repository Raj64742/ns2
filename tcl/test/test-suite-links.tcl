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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-links.tcl,v 1.9 2002/03/08 21:55:42 sfloyd Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-tcpVariants.tcl
#

source misc_simple.tcl
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
source support.tcl
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1

Agent/TCP set minrto_ 0
# The default is being changed to minrto_ 1
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

Trace set show_tcphdr_ 1

set wrap 90
set wrap1 [expr 90 * 512 + 40]

Class Topology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}

#
# Links1 uses 8Mb, 5ms feeders, and a 800Kb 10ms bottleneck.
# Queue-limit on bottleneck is 2 packets.
#
Class Topology/net4 -superclass Topology
Topology/net4 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]

    $self next
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 0ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 0ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 80Kb 10ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 8
    $ns queue-limit $node_(k1) $node_(r1) 8
}


TestSuite instproc finish file {
	global quiet wrap PERL
        exec $PERL ../../bin/set_flow_id -s all.tr | \
          $PERL ../../bin/getrc -s 2 -d 3 | \
          $PERL ../../bin/raw2xg -s 0.01 -m $wrap -t $file > temp.rands
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

TestSuite instproc setTopo {} {
    $self instvar node_ net_ ns_ topo_

    set topo_ [new Topology/$net_ $ns_]
    set node_(s1) [$topo_ node? s1]
    set node_(s2) [$topo_ node? s2]
    set node_(r1) [$topo_ node? r1]
    set node_(k1) [$topo_ node? k1]
    [$ns_ link $node_(r1) $node_(k1)] trace-dynamics $ns_ stdout
}

TestSuite instproc setup {tcptype} {
}

# Definition of test-suite tests

###################################################
## Plain link
###################################################

Class Test/links -superclass TestSuite
Test/links instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	links
	$self next
}
Test/links instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo

	set fid 1
	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) $fid]
        $tcp1 set window_ 1
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

        $self tcpDump $tcp1 5.0

        ###$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}

###################################################
## Link with changing delay.
###################################################

Class Test/changeDelay -superclass TestSuite
Test/changeDelay instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	changeDelay
	$self next
}
Test/changeDelay instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo

	set fid 1
	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) $fid]
        $tcp1 set window_ 1
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"
	$ns_ at 2.0 "$ns_ delay $node_(r1) $node_(k1) 1000ms duplex"
	$ns_ at 8.0 "$ns_ delay $node_(r1) $node_(k1) 100ms duplex"

        $self tcpDump $tcp1 5.0

        ##$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}

###################################################
## Link with changing bandwidth.
###################################################

Class Test/links1 -superclass TestSuite
Test/links1 instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	links1
	$self next
}
Test/links1 instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo

	set fid 1
	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) $fid]
        $tcp1 set window_ 8
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

        $self tcpDump $tcp1 5.0

        ##$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}

Class Test/changeBandwidth -superclass TestSuite
Test/changeBandwidth instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	changeBandwidth
	$self next
}
Test/changeBandwidth instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo

	set fid 1
	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) $fid]
        $tcp1 set window_ 8
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"
	$ns_ at 2.0 "$ns_ bandwidth $node_(r1) $node_(k1) 8Kb duplex"
	$ns_ at 8.0 "$ns_ bandwidth $node_(r1) $node_(k1) 800Kb duplex"

        $self tcpDump $tcp1 5.0

        ##$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}

###################################################
## Two dropped packets.  
###################################################

Class Test/dropPacket -superclass TestSuite
Test/dropPacket instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	dropPacket
	Agent/TCP set minrto_ 1
	$self next
}
Test/dropPacket instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo
	$self June01defaults

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 1]
        $tcp1 set window_ 8
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"
        $self dropPkts [$ns_ link $node_(r1) $node_(k1)] 1 {20 22}
	# dropPkts is in support.tcl

        $self tcpDump $tcp1 5.0

	$ns_ at 5.0 "$self cleanupAll $testName_"
        $ns_ run
}

Class Test/delayPacket -superclass TestSuite
Test/delayPacket instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	delayPacket
	Agent/TCP set minrto_ 1
	ErrorModel set delay_pkt_ true
	ErrorModel set drop_ false
	ErrorModel set delay_ 0.3
	Test/delayPacket instproc run {} [Test/dropPacket info instbody run ]
	$self next
}

TestSuite runTest


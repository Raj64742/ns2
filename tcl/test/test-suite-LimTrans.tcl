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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/Attic/test-suite-LimTrans.tcl,v 1.1 2000/07/28 02:12:33 sfloyd Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-tcpVariants.tcl
#

#source misc_simple.tcl

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
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 100ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 8
    $ns queue-limit $node_(k1) $node_(r1) 8

    $self instvar lossylink_
    set lossylink_ [$ns link $node_(r1) $node_(k1)]
    set em [new ErrorModule Fid]
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $lossylink_ errormodule $em
}


TestSuite instproc finish file {
	global quiet wrap PERL
        exec $PERL ../../bin/set_flow_id -m all.tr > t
        exec $PERL ../../bin/getrc -s 2 -d 3 t | \
          $PERL ../../bin/raw2xg -a -s 0.01 -m $wrap -t $file > temp.rands
        exec $PERL ../../bin/getrc -s 3 -d 2 t | \
          $PERL ../../bin/raw2xg -a -s 0.01 -m $wrap -t $file > temp1.rands
	if {$quiet == "false"} {
		exec xgraph -bb -tk -nl -m -x time -y packets temp.rands \
		temp1.rands &
	}
        ## now use default graphing tool to make a data file
	## if so desired
	## NO ECN:
	# exec csh gnuplotD.com temp.rands temp1.rands $file
	## with ECN:
 	exec csh gnuplotE.com temp.rands temp1.rands $file
        exit 0
}

TestSuite instproc emod {} {
        $self instvar topo_
        $topo_ instvar lossylink_
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

TestSuite instproc mark_pkts pkts {
    $self instvar ns_
    set emod [$self emod]
    set errmodel1 [new ErrorModel/List]
    $errmodel1 set markecn_ true
    $errmodel1 droplist $pkts
    $emod insert $errmodel1
    $emod bind $errmodel1 1
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

TestSuite instproc setup {tcptype list} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo

	set fid 1
        # Set up TCP connection
    	if {$tcptype == "Sack1"} {
      		set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) \
          	TCPSink/Sack1/DelAck  $node_(k1) $fid]
    	} else {
      		set tcp1 [$ns_ create-connection TCP/$tcptype $node_(s1) \
          	TCPSink $node_(k1) $fid]
    	}
        $tcp1 set window_ 28
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 1.0 "$ftp1 start"

        $self tcpDump $tcp1 5.0
        $self drop_pkts $list

        #$self traceQueues $node_(r1) [$self openTrace 4.0 $testName_]
	$ns_ at 4.0 "$self cleanupAll $testName_"
        $ns_ run
}

# Definition of test-suite tests

###################################################
## One drop
###################################################

Class Test/No_Limited_Transmit -superclass TestSuite
Test/No_Limited_Transmit instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	No_Limited_Transmit
	$self next
}
Test/No_Limited_Transmit instproc run {} {
        Agent/TCP set tcpTick_ 0.5
        $self setup Sack1 {4}
}

Class Test/Limited_Transmit -superclass TestSuite
Test/Limited_Transmit instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	Limited_Transmit
	Agent/TCP set singledup_ 1
	Test/Limited_Transmit instproc run {} [Test/No_Limited_Transmit info instbody run ]
	$self next
}

Class Test/ECN -superclass TestSuite
Test/ECN instproc init {} {
	$self instvar net_ test_
	set net_	net4
	set test_	ECN
	Queue/RED set setbit_ true
	Agent/TCP set old_ecn_ 1
	Agent/TCP set ecn 0
	$self next
}
Test/ECN instproc run {} {
	global wrap wrap1
        $self instvar ns_ node_ testName_
	$self setTopo

        Agent/TCP set tcpTick_ 0.5
	Agent/TCP set ecn_ 1
	set fid 1
        # Set up TCP connection
      	set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) \
          	TCPSink/Sack1/DelAck  $node_(k1) $fid]
        $tcp1 set window_ 28
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 1.0 "$ftp1 start"

        $self tcpDump $tcp1 5.0
        $self mark_pkts {4}

	$ns_ at 4.0 "$self cleanupAll $testName_"
        $ns_ run
}

TestSuite runTest


# copyright (c) 1995 The Regents of the University of California.
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-quiescent.tcl,v 1.1 2002/12/19 02:34:45 sfloyd Exp $
#

source misc_simple.tcl
source support.tcl
Agent/TFRC set SndrType_ 1 

# Uncomment the line below to use a random seed for the
#  random number generator.
# ns-random 0

TestSuite instproc finish {file stoptime} {
        global quiet PERL
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
          $PERL ../../bin/raw2xg -s 0.01 -m 90 -t $file > temp.rands
        exec echo $stoptime 0 >> temp.rands 
        if {$quiet == "false"} {
                exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
        }
        ## now use default graphing tool to make a data file
        ## if so desired
#       exec csh figure2.com $file
#	exec cp temp.rands temp.$file 
#	exec csh gnuplotA.com temp.$file $file
        exit 0
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
    Queue/RED set gentle_ true
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 50
    $ns queue-limit $node_(r2) $node_(r1) 50
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
}

Class Topology/net2a -superclass Topology
Topology/net2a instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(r2) [$ns node]
    set node_(s3) [$ns node]
    set node_(s4) [$ns node]

    $self next
    Queue/RED set gentle_ true
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 0.15Kb 2ms RED
    $ns queue-limit $node_(r1) $node_(r2) 2
    $ns queue-limit $node_(r2) $node_(r1) 50
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
}

TestSuite instproc setTopo {} {
    $self instvar node_ net_ ns_ topo_

    set topo_ [new Topology/$net_ $ns_]
    if {$net_ == "net2" || $net_ == "net2a"} {
        set node_(s1) [$topo_ node? s1]
        set node_(s2) [$topo_ node? s2]
        set node_(s3) [$topo_ node? s3]
        set node_(s4) [$topo_ node? s4]
        set node_(r1) [$topo_ node? r1]
        set node_(r2) [$topo_ node? r2]
        [$ns_ link $node_(r1) $node_(r2)] trace-dynamics $ns_ stdout
    }
}

Class Test/tfrc_onoff -superclass TestSuite
Test/tfrc_onoff instproc init {} {
    $self instvar net_ test_ guide_ stopTime1_ sndr_ rcvr_
    set net_	net2
    set test_ tfrc_onoff	
    set guide_  \
    "TFRC with a data source with limited, bursty data, no congestion."
    set sndr_ TFRC
    set rcvr_ TFRCSink
    Agent/TFRC set oldCode_ false
    Agent/TFRC set SndrType_ 1
    set stopTime1_ 10
    $self next
}
Test/tfrc_onoff instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ stopTime1_ sndr_ rcvr_
    if {$quiet == "false"} {puts $guide_}
    $self setTopo
    set stopTime $stopTime1_

    set tf1 [$ns_ create-connection $sndr_ $node_(s1) $rcvr_ $node_(s3) 0]
    set ftp [new Application/FTP]
    $ftp attach-agent $tf1
    $ns_ at 0 "$ftp produce 100"
    $ns_ at 5 "$ftp producemore 100"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    # trace only the bottleneck link
    $ns_ run
}

Class Test/tfrc_onoff_oldcode -superclass TestSuite
Test/tfrc_onoff_oldcode instproc init {} {
    $self instvar net_ test_ guide_ stopTime1_ sndr_ rcvr_
    set net_	net2
    set test_ tfrc_onoff_oldcode	
    set guide_  \
    "TFRC, bursty data, no congestion, old code."
    set sndr_ TFRC
    set rcvr_ TFRCSink
    Agent/TFRC set oldCode_ true
    Agent/TFRC set SndrType_ 1
    set stopTime1_ 10
    Test/tfrc_onoff_oldcode instproc run {} [Test/tfrc_onoff info instbody run ]
    $self next
}

Class Test/tcp_onoff -superclass TestSuite
Test/tcp_onoff instproc init {} {
    $self instvar net_ test_ guide_ stopTime1_ sndr_ rcvr_
    set net_	net2
    set test_ tcp_onoff	
    set guide_  \
    "TCP with a data source with limited, bursty data, no congestion."
    set sndr_ TCP/Sack1
    set rcvr_ TCPSink/Sack1
    Agent/TFRC set SndrType_ 1
    set stopTime1_ 10
    Test/tcp_onoff instproc run {} [Test/tfrc_onoff info instbody run ]
    $self next
}

TestSuite runTest



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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-aimd.tcl,v 1.1 1999/11/19 05:48:34 sfloyd Exp $
#

source misc_simple.tcl

# Uncomment the line below to use a random seed for the
#  random number generator.
# ns-random 0

TestSuite instproc finish file {
        global quiet PERL
	$self instvar cwnd_chan_
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
          $PERL ../../bin/raw2xg -s 0.01 -m 90 -t $file > temp1.rands
        if {$quiet == "false"} {
                exec xgraph -bb -tk -nl -m -x time -y packets temp1.rands &
        }
        if { [info exists cwnd_chan_] } {
                $self plot_cwnd
    		exec cp temp.cwnd temp.rands
        }
        ## now use default graphing tool to make a data file
        ## if so desired
#       exec csh figure2.com $file
#	exec csh gnuplotA.com temp.rands $file
###        exit 0
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
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 5ms RED
    $ns queue-limit $node_(r1) $node_(r2) 50
    $ns queue-limit $node_(r2) $node_(r1) 50
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
}

TestSuite instproc setTopo {} {
    $self instvar node_ net_ ns_ topo_

    set topo_ [new Topology/$net_ $ns_]
    if {$net_ == "net2"} {
        set node_(s1) [$topo_ node? s1]
        set node_(s2) [$topo_ node? s2]
        set node_(s3) [$topo_ node? s3]
        set node_(s4) [$topo_ node? s4]
        set node_(r1) [$topo_ node? r1]
        set node_(r2) [$topo_ node? r2]
        [$ns_ link $node_(r1) $node_(r2)] trace-dynamics $ns_ stdout
    }
}

Class Test/tcp -superclass TestSuite
Test/tcp instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	tcp
    $self next
}
Test/tcp instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ dumpfile_
    $self setTopo
    Agent/TCP set window_ 20
    set stopTime  20.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    set ftp1 [$tcp1 attach-app FTP]
    $self enable_tracecwnd $ns_ $tcp1
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at $stopTime0 "$ftp1 stop"

    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s4) 1]
    set ftp2 [$tcp2 attach-app FTP]
    $ns_ at 10.0 "$ftp2 start"
    $ns_ at 15.0 "$ftp2 stop"


    $self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    ## $ns_ at $stopTime3 "exec cp temp.cwnd temp.rands; exit 0"
    $ns_ at $stopTime2 "exit 0"

    # trace only the bottleneck link
    $ns_ run
}

Class Test/tcpA -superclass TestSuite
Test/tcpA instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	tcpA{increase_0.41,decrease_0.75}
    Agent/TCP set increase_num_ 0.41
    Agent/TCP set decrease_num_ 0.75
    Test/tcpA instproc run {} [Test/tcp info instbody run ]
    $self next
}

Class Test/tcpB -superclass TestSuite
Test/tcpB instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	tcpB{increase_1.00,decrease_0.875}
    Agent/TCP set increase_num_ 1.0
    Agent/TCP set decrease_num_ 0.875
    Test/tcpB instproc run {} [Test/tcp info instbody run ]
    $self next
}

TestSuite runTest


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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-friendly.tcl,v 1.1 1998/09/21 06:32:49 sfloyd Exp $
#

# UNDER CONSTRUCTION!!

source misc_simple.tcl

TestSuite instproc finish file {
        global quiet
        exec ../../bin/getrc -s 2 -d 3 all.tr | \
          ../../bin/raw2xg -s 0.01 -m 90 -t $file > temp.rands
        if {$quiet == "false"} {
                exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
        }
        ## now use default graphing tool to make a data file
        ## if so desired
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
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 25
    $ns queue-limit $node_(r2) $node_(r1) 25
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

proc stop {tracefile} {
    close $tracefile
    exit
}

# single packet drop
Class Test/test1 -superclass TestSuite
Test/test1 instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	test1
    $self next
}
Test/test1 instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo

    set tracefile [open all.tr w]
    $ns_ trace-all $tracefile

    # Set up TCP connection
    Agent/RTP/TFCC set packetSize_ 1000
    
    set tf1 [new Agent/RTP/TFCC $ns_]
    set tfr1 [new Agent/RTP/TFCC $ns_]
    set tf2 [new Agent/RTP/TFCC $ns_]
    set tfr2 [new Agent/RTP/TFCC $ns_]
    
    $ns_ attach-agent $node_(s1) $tf1
    $ns_ attach-agent $node_(s3) $tfr1
    $ns_ connect $tf1 $tfr1
    
    $ns_ attach-agent $node_(s2) $tf2
    $ns_ attach-agent $node_(s4) $tfr2
    $ns_ connect $tf2 $tfr2
    
    set tcp1 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(s4) 0]
    set ftp1 [$tcp1 attach-app FTP] 
    set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(s4) 0]
    set ftp2 [$tcp2 attach-app FTP] 
    set tcp3 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(s4) 0]
    set ftp3 [$tcp3 attach-app FTP] 
    $ns_ at 0.0 "$tf1 start"
    $ns_ at 10.0 "$tf2 start"
    $ns_ at 19.0 "$ftp1 start"
    $ns_ at 20.0 "$ftp2 start"
    $ns_ at 21.0 "$ftp3 start"
    $ns_ at 40 "$tf1 stop"
    $ns_ at 40 "$tf2 stop"
    $ns_ at 50 "$ftp2 stop"
    $ns_ at 40 "$ftp3 stop"
    $ns_ at 60 "stop $tracefile"

    $self tcpDump $tcp1 1.0

    # trace only the bottleneck link
    $self traceQueues $node_(r1) [$self openTrace 20.0 $testName_]
    $ns_ run
}


TestSuite runTest


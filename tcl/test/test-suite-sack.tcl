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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-sack.tcl,v 1.12 2001/05/27 02:14:59 sfloyd Exp $
#

source misc_simple.tcl
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1

Agent/TCP set minrto_ 0
# The default is being changed to minrto_ 1
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

TestSuite instproc finish file {
        global quiet PERL
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
          $PERL ../../bin/raw2xg -s 0.01 -m 90 -t $file > temp.rands
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

# 
# Links1 uses 8Mb, 5ms feeders, and a 800Kb 100ms bottleneck.
# Queue-limit on bottleneck is 6 packets. 
# 
Class Topology/net0 -superclass Topology
Topology/net0 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]

    $self next
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 100ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 6
    $ns queue-limit $node_(k1) $node_(r1) 6
}

# 
# Links1 uses 10Mb, 5ms feeders, and a 1.5Mb 100ms bottleneck.
# Queue-limit on bottleneck is 23 packets.
# 

Class Topology/net1 -superclass Topology
Topology/net1 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]

    $self next
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 5ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 5ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 1.5Mb 100ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 23
    $ns queue-limit $node_(k1) $node_(r1) 23
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
    if {$net_ == "net0" || $net_ == "net1"} {
        set node_(s1) [$topo_ node? s1]
        set node_(s2) [$topo_ node? s2]
        set node_(r1) [$topo_ node? r1]
        set node_(k1) [$topo_ node? k1]
        [$ns_ link $node_(r1) $node_(k1)] trace-dynamics $ns_ stdout
    }
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

# single packet drop
Class Test/sack1 -superclass TestSuite
Test/sack1 instproc init {} {
    $self instvar net_ test_
    set net_	net0
    set test_	sack1
    $self next
}
Test/sack1 instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
    $tcp1 set window_ 14
    set ftp1 [$tcp1 attach-app FTP]
    $ns_ at 1.0 "$ftp1 start"

    $self tcpDump $tcp1 1.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
    $ns_ at 5.0 "$self cleanupAll $testName_"
    $ns_ run
}

Class Test/sack1z -superclass TestSuite
Test/sack1z instproc init {} {
    $self instvar net_ test_
    set net_	net0
    set test_	sack1z
    $self next
}
Test/sack1z instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo

    Agent/TCP set maxburst_ 4
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
    $tcp1 set window_ 14
    set ftp1 [$tcp1 attach-app FTP]
    $ns_ at 0.0 "$ftp1 start"

    $self tcpDump $tcp1 1.0
    
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
    $ns_ at 5.0 "$self cleanupAll $testName_"
    $ns_ run
}

# three packet drops
Class Test/sack1a -superclass TestSuite
Test/sack1a instproc init {} {
    $self instvar net_ test_
    set net_	net0
    set test_	sack1a
    $self next
}
Test/sack1a instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
    $tcp1 set window_ 20
    set ftp1 [$tcp1 attach-app FTP]
    $ns_ at 0.0 "$ftp1 start"

    $self tcpDump $tcp1 1.0
    
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
    $ns_ at 5.0 "$self cleanupAll $testName_"
    $ns_ run
}

# three packet drops
Class Test/sack1aa -superclass TestSuite
Test/sack1aa instproc init {} {
    $self instvar net_ test_
    set net_	net0
    set test_	sack1aa
    $self next
}
Test/sack1aa instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo

    Agent/TCP set maxburst_ 4
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
    $tcp1 set window_ 20
    set ftp1 [$tcp1 attach-app FTP]
    $ns_ at 0.0 "$ftp1 start"

    $self tcpDump $tcp1 1.0
    
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
    $ns_ at 5.0 "$self cleanupAll $testName_"
    $ns_ run
}

Class Test/sack1b -superclass TestSuite
Test/sack1b instproc init {} {
    $self instvar net_ test_
    set net_	net0
    set test_	sack1b
    $self next
}
Test/sack1b instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
    $tcp1 set window_ 26
    set ftp1 [$tcp1 attach-app FTP]
    $ns_ at 0.0 "$ftp1 start"

    $self tcpDump $tcp1 1.0
    
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
    $ns_ at 5.0 "$self cleanupAll $testName_"
    $ns_ run
}

Class Test/sack1c -superclass TestSuite
Test/sack1c instproc init {} {
    $self instvar net_ test_
    set net_	net0
    set test_	sack1c
    $self next
}
Test/sack1c instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
    $tcp1 set window_ 27
    set ftp1 [$tcp1 attach-app FTP]
    $ns_ at 0.0 "$ftp1 start"

    $self tcpDump $tcp1 1.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
    $ns_ at 5.0 "$self cleanupAll $testName_"

    $ns_ run
}

Class Test/sack3 -superclass TestSuite
Test/sack3 instproc init {} {
    $self instvar net_ test_
    set net_	net0
    set test_	sack3
    $self next
}
Test/sack3 instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo
    $ns_ queue-limit $node_(r1) $node_(k1) 8
    $ns_ queue-limit $node_(k1) $node_(r1) 8
	
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
    $tcp1 set window_ 100
    $tcp1 set bugFix_ false

    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
    $tcp2 set window_ 16
    $tcp2 set bugFix_ false

    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    $ns_ at 1.0 "$ftp1 start"
    $ns_ at 0.5 "$ftp2 start"

    $self tcpDump $tcp1 1.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 4.0 $testName_]
    $ns_ at 4.0 "$self cleanupAll $testName_"
    $ns_ run
}

Class Test/sack5 -superclass TestSuite
Test/sack5 instproc init {} {
    $self instvar net_ test_
    set net_	net1
    set test_	sack5
    $self next
}
Test/sack5 instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo

    $ns_ delay $node_(s1) $node_(r1) 3ms
    $ns_ delay $node_(r1) $node_(s1) 3ms

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
    $tcp1 set window_ 50
    $tcp1 set bugFix_ false

    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
    $tcp2 set window_ 50
    $tcp2 set bugFix_ false

    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    $ns_ at 1.0 "$ftp1 start"
    $ns_ at 1.75 "$ftp2 produce 100"

    $self tcpDump $tcp1 1.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 6.0 $testName_]
    $ns_ at 6.0 "$self cleanupAll $testName_"
    $ns_ run
}

Class Test/sack5a -superclass TestSuite
Test/sack5a instproc init {} {
    $self instvar net_ test_
    set net_	net1
    set test_	sack5a
    $self next
}
Test/sack5a instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo

    Agent/TCP set maxburst_ 4
    $ns_ delay $node_(s1) $node_(r1) 3ms
    $ns_ delay $node_(r1) $node_(s1) 3ms

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
    $tcp1 set window_ 50
    $tcp1 set bugFix_ false

    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
    $tcp2 set window_ 50
    $tcp2 set bugFix_ false

    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    $ns_ at 1.0 "$ftp1 start"
    $ns_ at 1.75 "$ftp2 produce 100"

    $self tcpDump $tcp1 1.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 6.0 $testName_]
    $ns_ at 6.0 "$self cleanupAll $testName_"
    $ns_ run
}

# shows a long recovery from sack.
Class Test/sackB2 -superclass TestSuite
Test/sackB2 instproc init {} {
    $self instvar net_ test_
    set net_	net0
    set test_	sackB2
    $self next
}
Test/sackB2 instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo
    $ns_ queue-limit $node_(r1) $node_(k1) 9

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
    $tcp1 set window_ 50

    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
    $tcp2 set window_ 20

    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    $ns_ at 1.0 "$ftp1 start"
    $ns_ at 1.0 "$ftp2 start"

    $self tcpDump $tcp1 1.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 8.0 $testName_]
    $ns_ at 8.0 "$self cleanupAll $testName_"
    $ns_ run
}

# two packets dropped
Class Test/sackB4 -superclass TestSuite
Test/sackB4 instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	sackB4
    $self next
}
Test/sackB4 instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo
    $ns_ queue-limit $node_(r1) $node_(r2) 29
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(r2) 0]
    $tcp1 set window_ 40

    set ftp1 [$tcp1 attach-app FTP]
    $ns_ at 0.0 "$ftp1 start"

    $self tcpDump $tcp1 1.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 2.0 $testName_]
    $ns_ at 2.0 "$self cleanupAll $testName_"
    $ns_ run
}

# two packets dropped
Class Test/sackB4a -superclass TestSuite
Test/sackB4a instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	sackB4a
    $self next
}
Test/sackB4a instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo
    $ns_ queue-limit $node_(r1) $node_(r2) 29
    Agent/TCP set maxburst_ 4
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(r2) 0]
    $tcp1 set window_ 40

    set ftp1 [$tcp1 attach-app FTP]
    $ns_ at 0.0 "$ftp1 start"

    $self tcpDump $tcp1 1.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 2.0 $testName_]
    $ns_ at 2.0 "$self cleanupAll $testName_"
    $ns_ run
}

# delayed ack not implemented yet
#Class Test/delayedSack -superclass TestSuite
#Test/delayedSack instproc init {} {
#    $self instvar net_ test_
#    set net_    net0
#    set test_	delayedSack
#    $self next
#}
#Test/delayedSack instproc run {} {
#     $self instvar ns_ node_ testName_
#     $self setTopo
#     set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
#     $tcp1 set window_ 50
# 
#     # lookup up the sink and set it's delay interval
#     [$node_(k1) agent [$tcp1 dst-port]] set interval 100ms
# 
#     set ftp1 [$tcp1 attach-app FTP]
#     $ns_ at 1.0 "$ftp1 start"
# 
#     $self tcpDump $tcp1 1.0
# 
#     # trace only the bottleneck link
#     #$self traceQueues $node_(r1) [$self openTrace 4.0 $testName_]
#     $ns_ run
# }

## segregation
#Class Test/phaseSack -superclass TestSuite
#Test/phaseSack instproc init {} {
#    $self instvar net_ test_
#    set net_	net0
#    set test_	phaseSack
#    $self next
#}
#Test/phaseSack instproc run {} {
#    $self instvar ns_ node_ testName_
#    $self setTopo
#
#    $ns_ delay $node_(s2) $node_(r1) 3ms
#    $ns_ delay $node_(r1) $node_(s2) 3ms
#    $ns_ queue-limit $node_(r1) $node_(k1) 16
#    $ns_ queue-limit $node_(k1) $node_(r1) 100
#
#    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
#    $tcp1 set window_ 32
#
#    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
#    $tcp2 set window_ 32
#
#    set ftp1 [$tcp1 attach-app FTP]
#    set ftp2 [$tcp2 attach-app FTP]
#
#    $ns_ at 5.0 "$ftp1 start"
#    $ns_ at 1.0 "$ftp2 start"
#
#    $self tcpDump $tcp1 5.0
#
#    # trace only the bottleneck link
#    #$self traceQueues $node_(r1) [$self openTrace 25.0 $testName_]
#    $ns_ run
#}
#
## random overhead, but segregation remains 
#Class Test/phaseSack2 -superclass TestSuite
#Test/phaseSack2 instproc init {} {
#    $self instvar net_ test_
#    set net_	net0
#    set test_	phaseSack2
#    $self next
#}
#Test/phaseSack2 instproc run {} {
#    $self instvar ns_ node_ testName_
#    $self setTopo
#
#    $ns_ delay $node_(s2) $node_(r1) 3ms
#    $ns_ delay $node_(r1) $node_(s2) 3ms
#    $ns_ queue-limit $node_(r1) $node_(k1) 16
#    $ns_ queue-limit $node_(k1) $node_(r1) 100
#
#    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
#    $tcp1 set window_ 32
#    $tcp1 set overhead_ 0.01
#
#    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
#    $tcp2 set window_ 32
#    $tcp2 set overhead_ 0.01
#    
#    set ftp1 [$tcp1 attach-app FTP]
#    set ftp2 [$tcp2 attach-app FTP]
#    
#    $ns_ at 5.0 "$ftp1 start"
#    $ns_ at 1.0 "$ftp2 start"
#    
#    $self tcpDump $tcp1 5.0
#    
#    # trace only the bottleneck link
#    #$self traceQueues $node_(r1) [$self openTrace 25.0 $testName_]
#    $ns_ run
#}
#
## no segregation, because of random overhead
#Class Test/phaseSack3 -superclass TestSuite
#Test/phaseSack3 instproc init {} {
#    $self instvar net_ test_
#    set net_	net0
#    set test_	phaseSack3
#    $self next
#}
#Test/phaseSack3 instproc run {} {
#    $self instvar ns_ node_ testName_
#    $self setTopo
#
#    $ns_ delay $node_(s2) $node_(r1) 9.5ms
#    $ns_ delay $node_(r1) $node_(s2) 9.5ms
#    $ns_ queue-limit $node_(r1) $node_(k1) 16
#    $ns_ queue-limit $node_(k1) $node_(r1) 100
#
#    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
#    $tcp1 set window_ 32
#    $tcp1 set overhead_ 0.01
#
#    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
#    $tcp2 set window_ 32
#    $tcp2 set overhead_ 0.01
#
#    set ftp1 [$tcp1 attach-app FTP]
#    set ftp2 [$tcp2 attach-app FTP]
#
#    $ns_ at 5.0 "$ftp1 start"
#    $ns_ at 1.0 "$ftp2 start"
#
#    $self tcpDump $tcp1 5.0
#
#    # trace only the bottleneck link
#    #$self traceQueues $node_(r1) [$self openTrace 25.0 $testName_]
#    $ns_ run
#}

#Class Test/timersSack -superclass TestSuite
#Test/timersSack instproc init {} {
#    $self instvar net_ test_
#    set net_	net0
#    set test_	timersSack
#    $self next
#}
#Test/timersSack instproc run {} {
#     $self instvar ns_ node_ testName_
#     $self setTopo
#     $ns_ queue-limit $node_(r1) $node_(k1) 2
#     $ns_ queue-limit $node_(k1) $node_(r1) 100
# 
#     set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
#     $tcp1 set window_ 4
#     # lookup up the sink and set it's delay interval
#     [$node_(k1) agent [$tcp1 dst-port]] set interval 100ms
# 
#     set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
#     $tcp2 set window_ 4
#     # lookup up the sink and set it's delay interval
#     [$node_(k1) agent [$tcp2 dst-port]] set interval 100ms
# 
#     set ftp1 [$tcp1 attach-app FTP]
#     set ftp2 [$tcp2 attach-app FTP]
# 
#     $ns_ at 1.0 "$ftp1 start"
#     $ns_ at 1.3225 "$ftp2 start"
# 
#     $self tcpDump $tcp1 5.0
# 
#     # trace only the bottleneck link
#     #$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
#     $ns_ run
# }

TestSuite runTest


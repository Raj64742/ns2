#
# Copyright (c) 2003  International Computer Science Institute
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
#      This product includes software developed by ACIRI, the AT&T
#      Center for Internet Research at ICSI (the International Computer
#      Science Institute).
# 4. Neither the name of ACIRI nor of ICSI may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

source misc_simple.tcl
source support.tcl

# Uncomment the line below to use a random seed for the
#  random number generator.
# ns-random 0

TestSuite instproc finish {file stoptime} {
        global quiet PERL
	set wrap 90
	exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
	  $PERL ../../bin/raw2xg -s 0.01 -m $wrap -t $file > temp.rands
        exec $PERL ../../bin/getrc -s 3 -d 2 all.tr | \
          $PERL ../../bin/raw2xg -a -c -p -s 0.01 -m $wrap -t $file >> \
	  temp.rands
        exec echo $stoptime 0 >> temp.rands 
        if {$quiet == "false"} {
                exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
        }
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
    $ns queue-limit $node_(r1) $node_(r2) 50
    $ns queue-limit $node_(r2) $node_(r1) 50 
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
}

Class Test/example1 -superclass TestSuite
Test/example1 instproc init {} {
    $self instvar net_ test_ guide_ stopTime1_ 
    set net_	net2
    set test_ example1	
    set guide_  \
    "Example validation test with TCP, packet traces."
    set stopTime1_ 10
    Agent/TCP set window_ 64
    $self next pktTraceFile
}
Test/example1 instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ stopTime1_ 
    if {$quiet == "false"} {puts $guide_}
    $self setTopo
    set stopTime $stopTime1_

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    set ftp [new Application/FTP]
    $ftp attach-agent $tcp1
    $ns_ at 0 "$ftp produce 100"
    $ns_ at 3 "$ftp producemore 1000"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}

Class Test/example2 -superclass TestSuite
Test/example2 instproc init {} {
    $self instvar net_ test_ guide_ stopTime1_ 
    set net_	net2
    set test_ example2	
    set guide_  \
    "Example validation test with TFRC, packet traces."
    set stopTime1_ 10
    Agent/TFRC set SndrType_ 1
    $self next pktTraceFile
}
Test/example2 instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_ stopTime1_ 
    if {$quiet == "false"} {puts $guide_}
    $self setTopo
    set stopTime $stopTime1_

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    set ftp [new Application/FTP]
    $ftp attach-agent $tf1
    $ns_ at 0 "$ftp produce 100"
    $ns_ at 3 "$ftp producemore 1000"

    $ns_ at $stopTime "$self cleanupAll $testName_ $stopTime" 

    $ns_ run
}

TestSuite runTest


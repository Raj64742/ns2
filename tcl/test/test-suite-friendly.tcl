
# cOPYRight (c) 1995 The Regents of the University of California.
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-friendly.tcl,v 1.14 1999/10/04 18:44:11 sfloyd Exp $
#

source misc_simple.tcl
Agent/TFRC set df_ 0.5
Agent/TCP set window_ 100
# Uncomment the line below to use a random seed for the
#  random number generator.
# ns-random 0

TestSuite instproc finish file {
        global quiet PERL
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
          $PERL ../../bin/raw2xg -s 0.01 -m 90 -t $file > temp1.rands
        if {$quiet == "false"} {
                exec xgraph -bb -tk -nl -m -x time -y packets temp1.rands &
        }
        ## now use default graphing tool to make a data file
        ## if so desired
#       exec csh figure2.com $file
#	exec csh gnuplotA.com temp1.rands $file
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
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
#    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 5ms RED
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

# 
# Arrange for TFCC stats to be dumped for $src every
# $interval seconds of simulation time
# 
TestSuite instproc tfccDump { label src interval file } {
	set dumpfile temp.s
        $self instvar dump_inst_ ns_ f
        if ![info exists dump_inst_($src)] {
                set dump_inst_($src) 1
                $ns_ at 0.0 "$self tfccDump $label $src $interval $file"
                return
        }
        $ns_ at [expr [$ns_ now] + $interval] "$self tfccDump $label $src $interval $file"
        set report "[$ns_ now] $label [$src set ndatapack_] " 
        puts $file $report
}

#
# Arrange for TCP stats to be dumped for $tcp every
# $interval seconds of simulation time
#
TestSuite instproc pktsDump { label tcp interval file } {
    $self instvar dump_inst_ ns_
    if ![info exists dump_inst_($tcp)] {
        set dump_inst_($tcp) 1
        $ns_ at 0.0 "$self pktsDump $label $tcp $interval $file"
        return
    }
    $ns_ at [expr [$ns_ now] + $interval] "$self pktsDump $label $tcp $interval $file"
    set report "[$ns_ now] $label [$tcp set ack_]"
    puts $file $report
}

# display graph of results
TestSuite instproc finish_1 testname {
        global quiet
        $self instvar topo_

        set graphfile temp.rands

        set awkCode  {
		{
                if ($2 == fid) { print $1, $3 - last; last = $3 }
		}
        }

        set f [open $graphfile w]
        puts $f "TitleText: $testname"
        puts $f "Device: Postscript"

        exec rm -f temp.p
        exec touch temp.p
        foreach i { 1 2 3 4 5} {
                exec echo "\n\"flow $i" >> temp.p
                exec awk $awkCode fid=$i temp.s > temp.$i
                exec cat temp.$i >> temp.p
                exec echo " " >> temp.p
        }

        exec cat temp.p >@ $f
        close $f
        if {$quiet == "false"} {
		exec cp -f $graphfile temp2.rands
                exec xgraph -bb -tk -x time -y packets temp2.rands &
        }
#	exec csh gnuplotB.com temp2.rands $testname
#       exec csh figure2.com temp.rands $testname

###        exit 0
}

TestSuite instproc runFriendly {} {
    $self instvar ns_ node_ interval_ dumpfile_

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 3]
    set tf2 [$ns_ create-connection TFRC $node_(s2) TFRCSink $node_(s4) 4]
    $ns_ at 0.0 "$tf1 start"
    $ns_ at 4.0 "$tf2 start"
    $ns_ at 30 "$tf1 stop"
    $ns_ at 30 "$tf2 stop"

    $self tfccDump 1 $tf1 $interval_ $dumpfile_
    $self tfccDump 2 $tf2 $interval_ $dumpfile_
}

TestSuite instproc runTcp {} {
    $self instvar ns_ node_ interval_ dumpfile_

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 3]
    set ftp1 [$tcp1 attach-app FTP]
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s4) 4]
    set ftp2 [$tcp2 attach-app FTP]

    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 4.0 "$ftp2 start"
    $ns_ at 30 "$ftp1 stop"
    $ns_ at 30 "$ftp2 stop"

    $self pktsDump 1 $tcp1 $interval_ $dumpfile_
    $self pktsDump 2 $tcp2 $interval_ $dumpfile_
}

TestSuite instproc runTcps {} {
    $self instvar ns_ node_ interval_ dumpfile_

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s4) 0]
    set ftp1 [$tcp1 attach-app FTP] 
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s4) 1]
    set ftp2 [$tcp2 attach-app FTP] 
    set tcp3 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s4) 2]
    set ftp3 [$tcp3 attach-app FTP] 
    $ns_ at 8.0 "$ftp1 start"
    $ns_ at 12.0 "$ftp2 start"
    $ns_ at 16.0 "$ftp3 start"
    $ns_ at 24 "$ftp2 stop"
    $ns_ at 20 "$ftp3 stop"

    $self pktsDump 3 $tcp1 $interval_ $dumpfile_
    $self pktsDump 4 $tcp2 $interval_ $dumpfile_
    $self pktsDump 5 $tcp3 $interval_ $dumpfile_
}

Class Test/slowStart -superclass TestSuite
Test/slowStart instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	slowStart
    ##
    $self next
}
Test/slowStart instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_
    $self setTopo
    set interval_ 0.1
    set stopTime 40.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    $ns_ at 0.0 "$tf1 start"
    $ns_ at 30 "$tf1 stop"
    set tf2 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 1]
    $ns_ at 16 "$tf2 start"
    $ns_ at $stopTime "$tf2 stop"

    $self tfccDump 1 $tf1 $interval_ $dumpfile_
    $self tfccDump 2 $tf2 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

Class Test/slowStartTcp -superclass TestSuite
Test/slowStartTcp instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	slowStartTcp
    $self next
}
Test/slowStartTcp instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_
    $self setTopo
    set interval_ 0.1
    set stopTime 40.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    set ftp1 [$tcp1 attach-app FTP]
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 30 "$ftp1 stop"
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 1]
    set ftp2 [$tcp2 attach-app FTP]
    $ns_ at 16 "$ftp2 start"
    $ns_ at $stopTime "$ftp2 stop"

    $self tfccDump 1 $tcp1 $interval_ $dumpfile_
    $self tfccDump 2 $tcp2 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

Class Test/impulse -superclass TestSuite
Test/impulse instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	impulse
    $self next
}
Test/impulse instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_
    $self setTopo
    set interval_ 0.1
    set stopTime 40.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    $ns_ at 0.0 "$tf1 start"
    $ns_ at $stopTime "$tf1 stop"
    set tf2 [$ns_ create-connection TFRC $node_(s2) TFRCSink $node_(s4) 1]
    $ns_ at 10.0 "$tf2 start"
    $ns_ at 20.0 "$tf2 stop"

    $self tfccDump 1 $tf1 $interval_ $dumpfile_
    $self tfccDump 2 $tf2 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

# Feedback 4 times per roundtrip time.
Class Test/impulseMultReport -superclass TestSuite
Test/impulseMultReport instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	impulseMultReport
    Agent/TFRCSink set NumFeedback_ 4
    $self next
}
Test/impulseMultReport instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_
    $self setTopo
    set interval_ 0.1
    set stopTime 40.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tf1 [$ns_ create-connection TFRC $node_(s1) TFRCSink $node_(s3) 0]
    $ns_ at 0.0 "$tf1 start"
    set tf2 [$ns_ create-connection TFRC $node_(s2) TFRCSink $node_(s4) 1]
    $ns_ at 10.0 "$tf2 start"
    $ns_ at 20.0 "$tf2 stop"

    $self tfccDump 1 $tf1 $interval_ $dumpfile_
    $self tfccDump 2 $tf2 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

Class Test/impulseTcp -superclass TestSuite
Test/impulseTcp instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	impulseTcp
    $self next
}
Test/impulseTcp instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_
    $self setTopo
    set interval_ 0.1
    set stopTime 40.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    set ftp1 [$tcp1 attach-app FTP]
    $ns_ at 0.0 "$ftp1 start"
    set tf2 [$ns_ create-connection TFRC $node_(s2) TFRCSink $node_(s4) 1]
    $ns_ at 10.0 "$tf2 start"
    $ns_ at 20.0 "$tf2 stop"

    $self tfccDump 1 $tcp1 $interval_ $dumpfile_
    $self tfccDump 2 $tf2 $interval_ $dumpfile_

    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

# Two TFRC connections and three TCP connections.
Class Test/two-friendly -superclass TestSuite
Test/two-friendly instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	two-friendly
    $self next
}
Test/two-friendly instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_
    $self setTopo
    set interval_ 0.1
    set stopTime 30.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    $self runFriendly 
    $self runTcps
    
    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}

# Only TCP
Class Test/OnlyTcp -superclass TestSuite
Test/OnlyTcp instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	OnlyTcp
    $self next
}
Test/OnlyTcp instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ interval_ dumpfile_
    $self setTopo
    set interval_ 0.1
    set stopTime 30.0
    set stopTime0 [expr $stopTime - 0.001]
    set stopTime2 [expr $stopTime + 0.001]

    set dumpfile_ [open temp.s w]
    if {$quiet == "false"} {
        set tracefile [open all.tr w]
        $ns_ trace-all $tracefile
    }

    $self runTcp 
    $self runTcps
    
    $ns_ at $stopTime0 "close $dumpfile_; $self finish_1 $testName_"
    $self traceQueues $node_(r1) [$self openTrace $stopTime $testName_]
    if {$quiet == "false"} {
	$ns_ at $stopTime2 "close $tracefile"
    }
    $ns_ at $stopTime2 "exec cp temp2.rands temp.rands; exit 0"

    # trace only the bottleneck link
    $ns_ run
}


# BAD PARAMETERS! - small measurement interval
# SampleSizeMult_: controls interval for measuring packet drop rate
# MinNumLoss_: controls number of losses required in measurement interval
Class Test/BadParams -superclass TestSuite
Test/BadParams instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	BadParams
    Agent/TFRC set SampleSizeMult_ 0.5
    Agent/TFRC set MinNumLoss_ 0
    Agent/TFRCSink set SampleSizeMult_ 0.5
    Agent/TFRCSink set MinNumLoss_ 0 
    Test/BadParams instproc run {} [Test/two-friendly info instbody run ]
    $self next
}

# BAD PARAMETERS! - very slow increase
Class Test/BadParams2 -superclass TestSuite
Test/BadParams2 instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	BadParams2
    Agent/TFRC set ssmult_ 1.1
    Test/BadParams2 instproc run {} [Test/two-friendly info instbody run ]
    $self next
}

TestSuite runTest


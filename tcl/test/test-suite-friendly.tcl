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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-friendly.tcl,v 1.8 1999/07/10 17:18:13 sfloyd Exp $
#

# UNDER CONSTRUCTION!!

source misc_simple.tcl
Agent/TFRM set df_ 0.5 

TestSuite instproc finish file {
        global quiet PERL
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
          $PERL ../../bin/raw2xg -s 0.01 -m 90 -t $file > temp.rands
        if {$quiet == "false"} {
		exec cp -f temp.rands temp1.rands
                exec xgraph -bb -tk -nl -m -x time -y packets temp1.rands &
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
    Queue/RED set gentle_ true
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
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
		exec cp -f $graphfile temp1.rands
                exec xgraph -bb -tk -x time -y packets temp1.rands &
        }
#       exec csh figure2.com $file

        exit 0
}

proc stop {tracefile} {
    close $tracefile
    exit
}

TestSuite instproc runFriendly {} {
    $self instvar ns_ node_ interval_ dumpfile_

    set tf1 [$ns_ create-connection TFRM $node_(s1) TFRMSink $node_(s3) 3]
    set tf2 [$ns_ create-connection TFRM $node_(s2) TFRMSink $node_(s4) 4]
    $ns_ at 0.0 "$tf1 start"
#    $ns_ at 5.0 "$tf1 start"
    $ns_ at 10.0 "$tf2 start"
    $ns_ at 40 "$tf1 stop"
    $ns_ at 40 "$tf2 stop"

    $self tfccDump 1 $tf1 $interval_ $dumpfile_
    $self tfccDump 2 $tf2 $interval_ $dumpfile_
}

TestSuite instproc runTcps {} {
    $self instvar ns_ node_ interval_ dumpfile_

    set tcp1 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(s4) 0]
    set ftp1 [$tcp1 attach-app FTP] 
    set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(s4) 1]
    set ftp2 [$tcp2 attach-app FTP] 
    set tcp3 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(s4) 2]
    set ftp3 [$tcp3 attach-app FTP] 
    $ns_ at 19.0 "$ftp1 start"
#    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 20.0 "$ftp2 start"
    $ns_ at 21.0 "$ftp3 start"
    $ns_ at 50 "$ftp2 stop"
    $ns_ at 40 "$ftp3 stop"

    $self pktsDump 3 $tcp1 $interval_ $dumpfile_
    $self pktsDump 4 $tcp2 $interval_ $dumpfile_
    $self pktsDump 5 $tcp3 $interval_ $dumpfile_
}

# Two TFRM connections and three TCP connections.
Class Test/test1 -superclass TestSuite
Test/test1 instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	test1
    Agent/TFRM set version_ 1
    Agent/TFRM set slowincr_ 0
    $self next
}
Test/test1 instproc run {} {
    $self instvar ns_ node_ testName_ interval_ dumpfile_
    $self setTopo
    set interval_ 1.0
    set stopTime 60.0

    set dumpfile_ [open temp.s w]

    $self runFriendly 
    $self runTcps
    
    $ns_ at $stopTime "close $dumpfile_; $self finish_1 $testName_"

    # trace only the bottleneck link
    $ns_ run
}

# Two TFRM connections and three TCP connections.
Class Test/test2 -superclass TestSuite
Test/test2 instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	test2
    Agent/TFRM set version_ 1
    Agent/TFRM set slowincr_ 0
    $self next
}
Test/test2 instproc run {} {
    $self instvar ns_ node_ testName_ interval_ dumpfile_
    $self setTopo
    set interval_ 1.0
    set stopTime 31.0

    set dumpfile_ [open temp.s w]
    set tracefile [open all.tr w]
    $ns_ trace-all $tracefile

    $self runFriendly 
    $self runTcps
    
    $ns_ at $stopTime "stop $tracefile;"

    # trace only the bottleneck link
    $self traceQueues $node_(r1) [$self openTrace 30.0 $testName_]
    $ns_ run
}


# Two TFRM connections and three TCP connections.
Class Test/multIncr -superclass TestSuite
Test/multIncr instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	multIncr
    Agent/TFRM set version 1
    Agent/TFRM set incrrate_ 1
    Agent/TFRM set slowincr_ 3
    $self next
}
Test/multIncr instproc run {} {
    $self instvar ns_ node_ testName_ interval_ dumpfile_
    $self setTopo
    set interval_ 1.0
    set stopTime 60.0

    set dumpfile_ [open temp.s w]

    $self runFriendly 
    $self runTcps
    
    $ns_ at $stopTime "close $dumpfile_; $self finish_1 $testName_"

    # trace only the bottleneck link
    $ns_ run
}

# Two TFRM connections and three TCP connections.
Class Test/multIncr2 -superclass TestSuite
Test/multIncr2 instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	multIncr2
    Agent/TFRM set version 1
    Agent/TFRM set incrrate_ 1
    Agent/TFRM set slowincr_ 3
    $self next
}
Test/multIncr2 instproc run {} {
    $self instvar ns_ node_ testName_ interval_ dumpfile_
    $self setTopo
    set interval_ 1.0
    set stopTime 31.0

    set dumpfile_ [open temp.s w]
    set tracefile [open all.tr w]
    $ns_ trace-all $tracefile

    $self runFriendly 
    $self runTcps
    
    $ns_ at $stopTime "stop $tracefile;"

    # trace only the bottleneck link
    $self traceQueues $node_(r1) [$self openTrace 30.0 $testName_]
    $ns_ run
}

TestSuite runTest


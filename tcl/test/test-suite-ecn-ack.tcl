#
# Copyright (c) 1995-1997 The Regents of the University of California.
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-ecn-ack.tcl,v 1.1 1998/05/28 01:25:36 sfloyd Exp $
#
# This test suite reproduces most of the tests from the following note:
# Floyd, S., 
# Ns Simulator Tests for Random Early Detection (RED), October 1996.
# URL ftp://ftp.ee.lbl.gov/papers/redsims.ps.Z.
#
# To run all tests: test-all-red

set dir [pwd]
catch "cd tcl/test"
source misc.tcl
source topologies.tcl
catch "cd $dir"

set flowfile fairflow.tr; # file where flow data is written
set flowgraphfile fairflow.xgr; # file given to graph tool 

TestSuite instproc finish file {
	global quiet 
	$self instvar ns_ tchan_ testName_ cwnd_chan_
        exec ../../bin/getrc -s 2 -d 3 all.tr | \
	   ../../bin/raw2xg -a -e -s 0.01 -m 90 -t $file > temp.rands
	exec ../../bin/getrc -s 3 -d 2 all.tr | \
	  ../../bin/raw2xg -a -e -s 0.01 -m 90 -t $file > temp1.rands
	if {$quiet == "false"} {
        	exec xgraph -bb -tk -nl -m -x time -y packets temp.rands \
                     temp1.rands &
	}
        ## now use default graphing tool to make a data file
        ## if so desired

	$ns_ halt
}

TestSuite instproc tcpDumpAll { tcpSrc interval label } {
    global quiet
    $self instvar dump_inst_ ns_
    if ![info exists dump_inst_($tcpSrc)] {
	set dump_inst_($tcpSrc) 1
	set report $label/window=[$tcpSrc set window_]/packetSize=[$tcpSrc set packetSize_]
	if {$quiet == "false"} {
		puts $report
	}
	$ns_ at 0.0 "$self tcpDumpAll $tcpSrc $interval $label"
	return
    }
    $ns_ at [expr [$ns_ now] + $interval] "$self tcpDumpAll $tcpSrc $interval $label"
    set report time=[$ns_ now]/class=$label/ack=[$tcpSrc set ack_]/packets_resent=[$tcpSrc set nrexmitpack_]
    if {$quiet == "false"} {
    	puts $report
    }
}       

TestSuite instproc emod {} {
	$self instvar topo_
	$topo_ instvar lossylink_
        set errmodule [$lossylink_ errormodule]
	return $errmodule
}

TestSuite instproc setloss {} {
	$self instvar topo_
	$topo_ instvar lossylink_
        set errmodule [$lossylink_ errormodule]
        set errmodel [$errmodule errormodels]
        if { [llength $errmodel] > 1 } {
                puts "droppedfin: confused by >1 err models..abort"
                exit 1
        }
	return $errmodel
}

#######################################################################

TestSuite instproc ecnsetup { tcptype { tcp1fid 0 } } {
    $self instvar ns_ node_ testName_ net_

    set delay 10ms
    $ns_ delay $node_(r1) $node_(r2) $delay
    $ns_ delay $node_(r2) $node_(r1) $delay

    set stoptime 2.0
    set redq [[$ns_ link $node_(r1) $node_(r2)] queue]
    $redq set setbit_ true
    $redq set limit_ 50

    if {$tcptype == "Tahoe"} {
      set tcp1 [$ns_ create-connection TCP $node_(s1) \
	  TCPSink $node_(s3) $tcp1fid]
    } elseif {$tcptype == "Sack1"} {
      set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) \
	  TCPSink/Sack1  $node_(s3) $tcp1fid]
    } else {
      set tcp1 [$ns_ create-connection TCP/$tcptype $node_(s1) \
	  TCPSink $node_(s3) $tcp1fid]
    }
    $tcp1 set window_ 35
    $tcp1 set ecn_ 1
    set ftp1 [$tcp1 attach-source FTP]
        
    $ns_ at 0.0 "$ftp1 start"
        
    $self tcpDump $tcp1 5.0
        
    # trace only the bottleneck link
    $self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
}

# Drop the specified packet.
TestSuite instproc drop_pkt { number } {
    $self instvar ns_ lossmodel
    set lossmodel [$self setloss]
    $lossmodel set offset_ $number
    $lossmodel set period_ 10000
}

TestSuite instproc drop_pkts pkts {
    $self instvar ns_
    set emod [$self emod]
    set errmodel1 [new ErrorModel/List]
    $errmodel1 droplist $pkts
    $emod insert $errmodel1
    $emod bind $errmodel1 1

}

#######################################################################
# Reno Tests #
#######################################################################

# ECN followed by packet loss.
Class Test/ecn_ack -superclass TestSuite
Test/ecn_ack instproc init topo {
        $self instvar net_ defNet_ test_
        Queue/RED set setbit_ true
        set net_	$topo
        set defNet_	net2-lossy
	Agent/TCP set bugFix_ true
        set test_	ecn_ack
        $self next
}
Test/ecn_ack instproc run {} {
	$self instvar ns_
	$self ecnsetup Sack1
	$self drop_pkt 20000
	$ns_ run
}

TestSuite runTest

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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-ecn.tcl,v 1.1 1998/05/05 02:04:08 sfloyd Exp $
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
	$self instvar ns_ tchan_ testName_
        exec ../../bin/getrc -s 2 -d 3 all.tr | \
          ../../bin/raw2xg -a -s 0.01 -m 90 -t $file > temp.rands
	if {$quiet == "false"} {
        	exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
	}
        ## now use default graphing tool to make a data file
        ## if so desired

	if { [info exists tchan_] && $quiet == "false" } {
		$self plotQueue $testName_
	}
	$ns_ halt
}

TestSuite instproc enable_tracequeue ns {
	$self instvar tchan_ node_
	set redq [[$ns link $node_(r1) $node_(r2)] queue]
	set tchan_ [open all.q w]
	$redq trace curq_
	$redq trace ave_
	$redq attach $tchan_
}

#
# Reconfigure the net2 topology for the RED experiments.
#
Topology/net2 instproc config ns {
    $self instvar node_
    # force identical behavior to ns-1.
    # the recommended value for linterm is now 10
    # and is placed in the default file (3/31/97)
    [[$ns link $node_(r1) $node_(r2)] queue] set linterm_ 50
    [[$ns link $node_(r2) $node_(r1)] queue] set linterm_ 50
	
}

TestSuite instproc plotQueue file {
	global quiet
	$self instvar tchan_
	#
	# Plot the queue size and average queue size, for RED gateways.
	#
	set awkCode {
		{
			if ($1 == "Q" && NF>2) {
				print $2, $3 >> "temp.q";
				set end $2
			}
			else if ($1 == "a" && NF>2)
				print $2, $3 >> "temp.a";
		}
	}
	set f [open temp.queue w]
	puts $f "TitleText: $file"
	puts $f "Device: Postscript"

	if { [info exists tchan_] } {
		close $tchan_
	}
	exec rm -f temp.q temp.a 
	exec touch temp.a temp.q

	exec awk $awkCode all.q

	puts $f \"queue
	exec cat temp.q >@ $f  
	puts $f \n\"ave_queue
	exec cat temp.a >@ $f
	###puts $f \n"thresh
	###puts $f 0 [[ns link $r1 $r2] get thresh]
	###puts $f $end [[ns link $r1 $r2] get thresh]
	close $f
	if {$quiet == "false"} {
		exec xgraph -bb -tk -x time -y queue temp.queue &
	}
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

Class Test/ecn -superclass TestSuite
Test/ecn instproc init topo {
    $self instvar net_ defNet_ test_
    Queue/RED set setbit_ true
    set net_	$topo
    set defNet_	net2
    set test_	ecn
    $self next
}
Test/ecn instproc run {} {
    $self instvar ns_ node_ testName_

    set stoptime 10.0
    set redq [[$ns_ link $node_(r1) $node_(r2)] queue]
    $redq set setbit_ true

    set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 15
    $tcp1 set ecn_ 1

    set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(s3) 1]
    $tcp2 set window_ 15
    $tcp2 set ecn_ 1
        
    set ftp1 [$tcp1 attach-source FTP]
    set ftp2 [$tcp2 attach-source FTP]
        
    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 3.0 "$ftp2 start"
        
    $self tcpDump $tcp1 5.0
        
    # trace only the bottleneck link
    $self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
        
    $ns_ run
}

TestSuite instproc ecnsetup { tcptype } {
    $self instvar ns_ node_ testName_ net_

    set stoptime 4.0
    set redq [[$ns_ link $node_(r1) $node_(r2)] queue]
    $redq set setbit_ true

    if {$tcptype == "Tahoe"} {
      set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(s3) 0]
    } else {
      set tcp1 [$ns_ create-connection TCP/$tcptype $node_(s1) TCPSink $node_(s3) 0]
    }
    $tcp1 set window_ 24 
    $tcp1 set ecn_ 1
    set ftp1 [$tcp1 attach-source FTP]
        
    #$self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
        
    $self tcpDump $tcp1 5.0
        
    # trace only the bottleneck link
    $self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
}

# ECN followed by packet loss.
Class Test/ecn_drop_reno -superclass TestSuite
Test/ecn_drop_reno instproc init topo {
    $self instvar net_ defNet_ test_
    Queue/RED set setbit_ true
    set net_	$topo
    set defNet_	net2-lossy
    set test_	ecn_drop_reno
    $self next
}

Test/ecn_drop_reno instproc run {} {
	$self instvar ns_
	$self ecnsetup Reno
    set lossmodel [$self setloss]
    $lossmodel set offset_ 430
    $lossmodel set period_ 10000
	$ns_ run
}

# Same as "ecn_drop_reno", but without the packet loss.
Class Test/ecn_nodrop_reno -superclass TestSuite
Test/ecn_nodrop_reno instproc init topo {
    $self instvar net_ defNet_ test_
    Queue/RED set setbit_ true
    set net_	$topo
    set defNet_	net2-lossy
    set test_	ecn_nodrop_reno
    $self next
}

Test/ecn_nodrop_reno instproc run {} {
	$self instvar ns_
	$self ecnsetup Reno 
	set lossmodel [$self setloss]
	$lossmodel set offset_ 10000
	$lossmodel set period_ 10000
	$ns_ run
}

# ECN followed by packet loss.
Class Test/ecn_drop_tahoe -superclass TestSuite
Test/ecn_drop_tahoe instproc init topo {
    $self instvar net_ defNet_ test_
    Queue/RED set setbit_ true
    set net_	$topo
    set defNet_	net2-lossy
    set test_	ecn_drop_tahoe
    $self next
}

Test/ecn_drop_tahoe instproc run {} {
	$self instvar ns_
	$self ecnsetup Tahoe
    set lossmodel [$self setloss]
    $lossmodel set offset_ 430
    $lossmodel set period_ 10000
	$ns_ run
}

# Same as "ecn_drop_tahoe", but without the packet loss.
Class Test/ecn_nodrop_tahoe -superclass TestSuite
Test/ecn_nodrop_tahoe instproc init topo {
    $self instvar net_ defNet_ test_
    Queue/RED set setbit_ true
    set net_	$topo
    set defNet_	net2-lossy
    set test_	ecn_nodrop_tahoe
    $self next
}

Test/ecn_nodrop_tahoe instproc run {} {
	$self instvar ns_
	$self ecnsetup Tahoe 
	set lossmodel [$self setloss]
	$lossmodel set offset_ 10000
	$lossmodel set period_ 10000
	$ns_ run
}

#
#######################################################################

# 
# Links1 uses 8Mb, 5ms feeders, and a 800Kb 20ms bottleneck.
# Queue-limit on bottleneck is 25 packets.
#    
Class Topology/net6 -superclass NodeTopology/4nodes
Topology/net6 instproc init ns {
    $self next $ns
    $self instvar node_
    Queue/RED set setbit_ true
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 20ms RED
    $ns queue-limit $node_(r1) $node_(k1) 25
    $ns queue-limit $node_(k1) $node_(r1) 25
    if {[$class info instprocs config] != ""} { 
        $self config $ns
    } 
} 

# This test shows two TCPs when one is ECN-capable and the other 
# is not.

Class Test/ecn1 -superclass TestSuite
Test/ecn1 instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo
        set defNet_     net6
        set test_       ecn1_(one_with_ecn1,_one_without)
        $self next
}
Test/ecn1 instproc run {} {
        $self instvar ns_ node_ testName_

        # Set up TCP connection 
        set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
        $tcp1 set window_ 30
        $tcp1 set ecn_ 1
        set ftp1 [$tcp1 attach-source FTP]
        $ns_ at 0.0 "$ftp1 start"

        # Set up TCP connection
        set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
        $tcp2 set window_ 20
        $tcp2 set ecn_ 0
        set ftp2 [$tcp2 attach-source FTP]
        $ns_ at 3.0 "$ftp2 start"

        $self tcpDump $tcp1 5.0
        $self tcpDump $tcp2 5.0 

        $self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
        $ns_ run
}

TestSuite runTest

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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-red.tcl,v 1.5 1997/10/16 00:59:04 sfloyd Exp $
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

set flowfile fairflow.tr
set flowgraphfile fairflow.xgr
set pthresh 100

# to generate flow graph with unforced drops:
#       set category 1
#       set awkprocedure unforcedmakeawk
# to generate flow graph with forced drops:
#       set category 0
#       set awkprocedure forcedmakeawk

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

#
# Reconfigure the net2 topology for the RED experiments.
#
Topology/net2 instproc config ns {
    $self instvar node_
    # force identical behavior to ns-1.
    # the recommended value for linterm is now 10
    # and is placed in the default file (3/31/97)
    [$ns link $node_(r1) $node_(r2)] set linterm_ 50
    [$ns link $node_(r2) $node_(r1)] set linterm_ 50
}

TestSuite instproc plotQueue file {
    return
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

	exec rm -f temp.q temp.a 
	exec touch temp.a temp.q
	exec awk $awkCode out.tr

	puts $f \"queue
	flush $f
	exec cat temp.q >@ $f
	flush $f
	puts $f \n\"ave_queue
	flush $f
	exec cat temp.a >@ $f
	###puts $f \n"thresh
	###puts $f 0 [[ns link $r1 $r2] get thresh]
	###puts $f $end [[ns link $r1 $r2] get thresh]
	close $f
	exec xgraph -bb -tk -x time -y queue temp.queue &
}

TestSuite instproc tcpDumpAll { tcpSrc interval label } {
    $self instvar dump_inst_ ns_
    if ![info exists dump_inst_($tcpSrc)] {
	set dump_inst_($tcpSrc) 1
	puts $label/window=[$tcpSrc set window_]/packetSize=[$tcpSrc set packetSize_]
	$ns_ at 0.0 "$self tcpDumpAll $tcpSrc $interval $label"
	return
    }
    $ns_ at [expr [$ns_ now] + $interval] "$self tcpDumpAll $tcpSrc $interval $label"
    puts time=[$ns_ now]/class=$label/ack=[$tcpSrc set ack_]
}       


Class Test/red1 -superclass TestSuite
Test/red1 instproc init topo {
    $self instvar net_ defNet_ test_
    set net_	$topo
    set defNet_	net2
    set test_	red1
    $self next
}
Test/red1 instproc run {} {
    $self instvar ns_ node_ testName_

    set stoptime 10.0
    
    set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 15

    set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(s3) 1]
    $tcp2 set window_ 15

    set ftp1 [$tcp1 attach-source FTP]
    set ftp2 [$tcp2 attach-source FTP]

    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 3.0 "$ftp2 start"
    $ns_ at $stoptime "plotQueue $testName_"

    $self tcpDump $tcp1 5.0

    # trace only the bottleneck link
    $self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]

    #puts seed=[ns-random 0]
    $ns_ run
}

# For this test the average queue size in measured in bytes.
Class Test/red1_bytes -superclass TestSuite
Test/red1_bytes instproc init topo {
    $self instvar net_ defNet_ test_
    set net_	$topo
    set defNet_	net2
    set test_	red1_bytes
    $self next
}
Test/red1_bytes instproc run {} {
    $self instvar ns_ node_ testName_

    set stoptime 10.0
    
    [$ns_ link $node_(r1) $node_(r2)] set bytes_ true
    [$ns_ link $node_(r1) $node_(r2)] set mean_pktsize_ 1000

	
    set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 15

    set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(s3) 1]
    $tcp2 set window_ 15

    set ftp1 [$tcp1 attach-source FTP]
    set ftp2 [$tcp2 attach-source FTP]

    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 3.0 "$ftp2 start"
    $ns_ at $stoptime "plotQueue $testName_"

    $self tcpDump $tcp1 5.0

    # trace only the bottleneck link
    $self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]

    #puts seed=[ns-random 0]
    $ns_ run
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
    [$ns_ link $node_(r1) $node_(r2)] set setbit_ true

    set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 15
    $tcp1 set ecn_ 1

    set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(s3) 1]
    $tcp2 set window_ 15
    $tcp2 set ecn_ 1
        
    set ftp1 [$tcp1 attach-source FTP]
    set ftp2 [$tcp2 attach-source FTP]
        
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 3.0 "$ftp2 start"
    $ns_ at $stoptime "plotQueue $testName_"
        
    $self tcpDump $tcp1 5.0
        
    # trace only the bottleneck link
    $self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
        
    #puts seed=[ns-random 0]
    $ns_ run
}

# "Red2" changes some of the RED gateway parameters.
# This should give worse performance than "red1".
Class Test/red2 -superclass TestSuite
Test/red2 instproc init topo {
    $self instvar net_ defNet_ test_
    set net_	$topo
    set defNet_	net2
    set test_	red2
    $self next
}
Test/red2 instproc run {} {
    $self instvar ns_ node_ testName_

    set stoptime 10.0
    [$ns_ link $node_(r1) $node_(r2)] set thresh_ 5
    [$ns_ link $node_(r1) $node_(r2)] set maxthresh_ 10
    [$ns_ link $node_(r1) $node_(r2)] set q_weight_ 0.003
	
    set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 15

    set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(s3) 1]
    $tcp2 set window_ 15

    set ftp1 [$tcp1 attach-source FTP]
    set ftp2 [$tcp2 attach-source FTP]

    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 3.0 "$ftp2 start"
    $ns_ at $stoptime "plotQueue $testName_"

    $self tcpDump $tcp1 5.0
    
    # trace only the bottleneck link
    $self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]

    #puts seed=[ns-random 0]
    $ns_ run
}

# The queue is measured in "packets".
Class XTest/red_twoway -superclass TestSuite
XTest/red_twoway instproc init topo {
    $self instvar net_ defNet_ test_
    set net_	$topo
    set defNet_	net2
    set test_	red_twoway
    $self next
}
XTest/red_twoway instproc run {} {
    $self instvar ns_ node_ testName_

    set stoptime 10.0
	
    set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 15
    set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(s4) 1]
    $tcp2 set window_ 15
    set ftp1 [$tcp1 attach-source FTP]
    set ftp2 [$tcp2 attach-source FTP]

    set tcp3 [$ns_ create-connection TCP/Reno $node_(s3) TCPSink $node_(s1) 2]
    $tcp3 set window_ 15
    set tcp4 [$ns_ create-connection TCP/Reno $node_(s4) TCPSink $node_(s2) 3]
    $tcp4 set window_ 15
    set ftp3 [$tcp3 attach-source FTP]
    set telnet1 [$tcp4 attach-source TELNET] ; $telnet1 set interval 0

    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 2.0 "$ftp2 start"
    $ns_ at 3.5 "$ftp3 start"
    $ns_ at 1.0 "$telnet1 start"
    $ns_ at $stoptime "plotQueue $testName_"

    $self tcpDump $tcp1 5.0

    # trace only the bottleneck link
    $self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]

    #puts seed=[ns-random 0]
    $ns_ run
}

# The queue is measured in "bytes".
Class XTest/red_twowaybytes -superclass TestSuite
XTest/red_twowaybytes instproc init topo {
    $self instvar net_ defNet_ test_
    set net_	$topo
    set defNet_	net2
    set test_	red_twowaybytes
    $self next
}
XTest/red_twowaybytes instproc run {} {
    $self instvar ns_ node_ testName_

    set stoptime 10.0
    [$ns_ link $node_(r1) $node_(r2)] set bytes_ true
    [$ns_ link $node_(r2) $node_(r1)] set bytes_ true
		
    set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 15
    set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(s4) 1]
    $tcp2 set window_ 15
    set ftp1 [$tcp1 attach-source FTP]
    set ftp2 [$tcp2 attach-source FTP]

    set tcp3 [$ns_ create-connection TCP/Reno $node_(s3) TCPSink $node_(s1) 2]
    $tcp3 set window_ 15
    set tcp4 [$ns_ create-connection TCP/Reno $node_(s4) TCPSink $node_(s2) 3]
    $tcp4 set window_ 15
    set ftp3 [$tcp3 attach-source FTP]
    set telnet1 [$tcp4 attach-source TELNET] ; $telnet1 set interval 0

    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 2.0 "$ftp2 start"
    $ns_ at 3.5 "$ftp3 start"
    $ns_ at 1.0 "$telnet1 start"
    $ns_ at $stoptime "plotQueue $testName_"

    $self tcpDump $tcp1 5.0

    # trace only the bottleneck link
    $self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]

    #puts seed=[ns-random 0]
    $ns_ run
}

#
#######################################################################
#
# The rest of the file defines the test suite for flows and flowgraphs.
# As such they are not ported over to ns-2 yet.  The tests themselves
# are nulled out.
#			-- Kannan	Wed May  7 15:15:37 PDT 1997
#

proc create_flowstats {} {

	global r1 r2 r1fm flowfile pthresh category

	set r1fm [new flowmgr srcdstcls]
	set flowdesc [open $flowfile w]
	$r1fm attach $flowdesc
## 2 "categories" of drops
        $r1fm ndropcats 2
## after 100 total packets have been dropped, invoke flowDump
        $r1fm cat-drops-pthresh $category $pthresh
	$r1fm cat-drops-pcallback $category flowDump 
	[ns link $r1 $r2] flow-mgr $r1fm
}

proc create_flowstats1 {} {

	global r1 r2 r1fm flowfile pthresh 

#	set r1fm [new flowmgr srcdstcls]
 	set r1fm [new flowmgr redsrcdstcls]
	set flowdesc [open $flowfile w]
	$r1fm attach $flowdesc
## 2 "categories" of drops
        $r1fm ndropcats 2
## after 100 total packets have been dropped, invoke flowDump1
        $r1fm total-drops-pthresh $pthresh
	$r1fm total-drops-pcallback flowDump1 
	[ns link $r1 $r2] flow-mgr $r1fm
}

#
# awk code used to produce:
#       x axis: # arrivals for this flow+category / # total arrivals [bytes]
#       y axis: # drops for this flow+category / # drops this category [pkts]
proc unforcedmakeawk { } {
        set awkCode {
            BEGIN { print "\"flow 0" }
            {
                if ($2 != prev) {
                        print " "; print "\"flow " $2; print 100.0 * $9/$13, 100.0 * $10 / $14; prev = $2
                } else
                        print 100.0 * $9 / $13, 100.0 * $10 / $14
            }
        }
        return $awkCode
}

#
# awk code used to produce:
#       x axis: # arrivals for this flow+category / # total arrivals [bytes]
#       y axis: # drops for this flow+category / # drops this category [bytes]
proc forcedmakeawk { } {
        set awkCode {
            BEGIN { print "\"flow 0" }
            {
                if ($2 != prev) {
                        print " "; print "\"flow " $2; print 100.0 * $9/$13, 100.0 * $11 / $15; prev = $2
                } else
                        print 100.0 * $9 / $13, 100.0 * $11 / $15
            }
        }
        return $awkCode
}

#
# awk code used to produce:
#      x axis: # arrivals for this flow+category / # total arrivals [bytes]
#      y axis: # drops for this flow / # drops [pkts and bytes combined]
proc allmakeawk { } {
        set awkCode {
            BEGIN { print "\"flow 0"; cat0=0; cat1=0}
            {
                if ($1 != prevtime && cat1 + cat0 > 0){
			cat1_part = frac_packets * cat1 / ( cat1 + cat0 ) 
			cat0_part = frac_bytes * cat0 / ( cat1 + cat0 ) 
                        print 100.0 * frac_arrivals, 100.0 * ( cat1_part + cat0_part )
			frac_bytes = 0; frac_packets = 0; frac_arrivals = 0;
			cat1 = 0; cat0 = 0;
			prevtime = $1
		}
		if ($2 != prev) {
			print " "; print "\"flow "prev;
			prev = $2
		}
		if ($3==0) {  
			if ($15>0) {frac_bytes = $11 / $15} 
			else {frac_bytes = 0}
			cat0 = $14
		} else if ($3==1) { 
			if ($14>0) {frac_packets = $10 / $14}
			else {frac_packets = 0}
			if ($13>0) {frac_arrivals = ( $9 / $13 )} 
			else {frac_arrivals = 0}
			cat1 = $14
		}
		prevtime = $1
            }
	    END {
		cat1_part = frac_packets * cat1 / ( cat1 + cat0 ) 
		cat0_part = frac_bytes * cat0 / ( cat1 + cat0 ) 
                print 100.0 * frac_arrivals, 100.0 * ( cat1_part + cat0_part )
	    }
        }
        return $awkCode
}

proc create_flow_graph { graphtitle graphfile } {
        global flowfile awkprocedure
        set tmpfile1 /tmp/fg1[pid]
        set tmpfile2 /tmp/fg2[pid]

        exec rm -f $graphfile
        set outdesc [open $graphfile w]
        #
        # this next part is xgraph specific
        #
        puts $outdesc "TitleText: $graphtitle"
        puts $outdesc "Device: Postscript"

        exec rm -f $tmpfile1 $tmpfile2
        puts "writing flow xgraph data to $graphfile..."

        exec sort -n +1 -o $flowfile $flowfile
        exec awk [$awkprocedure] $flowfile >@ $outdesc
        close $outdesc
}

proc finish_flow { file } {
	global flowgraphfile
	create_flow_graph $file $flowgraphfile
	puts "running xgraph..."
	exec xgraph -bb -tk -nl -m -lx 0,100 -ly 0,100 -x "% of data bytes" -y "% of discards" $flowgraphfile &
	exit 0
}

proc flowDump { link fm flow category } {
	$fm dump $category
	$fm resetcounters
}

proc flowDump1 { link fm flow } {
	$fm dump 
	$fm resetcounters
}

proc new_tcp { startTime source dest window class dump size } {
	set tcp [$ns_ create-connection TCP/Reno $source TCPSink $dest $class ]
	$tcp set window_ $window
	if {$size > 0}  {$tcp set packet-size $size }
	set ftp [$tcp attach-source FTP]
	$ns_ at $startTime "$ftp start"
        if {$dump == 1 } {$self tcpDumpAll $tcp 20.0 $class }
}

proc new_cbr { startTime source dest pktSize interval class } {
 	set cbr [ns_create_cbr $source $dest $pktSize $interval $class ]
	$ns_ at $startTime "$cbr start"
}

proc flows {} {
	global s1 s2 r1 r2 s3 s4 r1fm qgraphfile flowfile 
        set stoptime 500.0
	set testname test_two
	
	create_testnet2
	[$ns_ link $node_(r1) $node_(r2)] set mean_pktsize 1000
	[$ns_ link $node_(r2) $node_(r1)] set mean_pktsize 1000
	[$ns_ link $node_(r1) $node_(r2)] set linterm 10
	[$ns_ link $node_(r2) $node_(r1)] set linterm 10
	[$ns_ link $node_(r1) $node_(r2)] set queue-limit 100
	[$ns_ link $node_(r2) $node_(r1)] set queue-limit 100

	create_flowstats 
	[$ns_ link $node_(r1) $node_(r2)] set bytes true
	[$ns_ link $node_(r1) $node_(r2)] set wait false

        new_tcp 1.0 $node_(s1) $node_(s3) 100 1 1 1000
	new_tcp 1.2 $node_(s2) $node_(s4) 100 2 1 50
	new_cbr 1.4 $node_(s1) $node_(s4) 190 0.003 3

	$ns_ at $stoptime "$r1fm flush"
	$ns_ at $stoptime "finish_flow $testName_"

	$ns_ run
}

proc Xtest_flows {} {
	global category awkprocedure
   	set category 1
	set awkprocedure unforcedmakeawk
#	set seed [ns-random 0]
#	puts seed=$seed
	flows 
}

proc Xtest_flows1 {} {
	global category awkprocedure 
   	set category 0
	set awkprocedure forcedmakeawk
#	set seed [ns-random 0]
#	ns-random $seed
#	puts seed=$seed 
	flows 
}

proc Xtest_flowsAll {} {
	global s1 s2 r1 r2 s3 s4 r1fm qgraphfile flowfile awkprocedure
        set stoptime 500.0
	set testname test_two
	set awkprocedure allmakeawk
	
	create_testnet2
	[$ns_ link $node_(r1) $node_(r2)] set mean_pktsize 1000
	[$ns_ link $node_(r2) $node_(r1)] set mean_pktsize 1000
	[$ns_ link $node_(r1) $node_(r2)] set linterm 10
	[$ns_ link $node_(r2) $node_(r1)] set linterm 10
	[$ns_ link $node_(r1) $node_(r2)] set queue-limit 100
	[$ns_ link $node_(r2) $node_(r1)] set queue-limit 100

	create_flowstats1  
	[$ns_ link $node_(r1) $node_(r2)] set bytes true
	[$ns_ link $node_(r1) $node_(r2)] set wait false

        new_tcp 1.0 $node_(s1) $node_(s3) 100 1 1 1000
	new_tcp 1.2 $node_(s2) $node_(s4) 100 2 1 50
	new_cbr 1.4 $node_(s1) $node_(s4) 190 0.003 3

	$ns_ at $stoptime "$r1fm flush"
	$ns_ at $stoptime "finish_flow $testName_"

#	puts seed=[ns-random 0]
	$ns_ run
}

TestSuite runTest


#
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
# FOR UPDATING GLOBAL DEFAULTS:
Queue/RED set bytes_ false              
# default changed on 10/11/2004.
Queue/RED set queue_in_bytes_ false
# default changed on 10/11/2004.
Queue/RED set q_weight_ 0.002
Queue/RED set thresh_ 5 
Queue/RED set maxthresh_ 15
# The RED parameter defaults are being changed for automatic configuration.
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1
Agent/TCP set exitFastRetrans_ false
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/Attic/test-suite-red-v1a.tcl,v 1.13 2004/10/13 02:29:06 sfloyd Exp $
#
# This test suite reproduces most of the tests from the following note:
# Floyd, S., 
# Ns Simulator Tests for Random Early Detection (RED), October 1996.
# URL ftp://ftp.ee.lbl.gov/papers/redsims.ps.Z.
#

#
# This file uses the ns-1 interfaces, not the newere v2 interfaces.
# Don't copy this code for use in new simulations,
# copy the new code with the new interfaces!
#

#
# To run all tests: test-all-red
#       
# The default value for ns_red(linterm) has been changed from 50 (its
#  old value)to 10.

Agent/TCP set minrto_ 0
# The default is being changed to minrto_ 1
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

set flowfile fairflow.tr
set flowgraphfile fairflow.xgr
set pthresh 100
set randseed 0
set quiet false

# to generate flow graph with unforced drops:
#       set category 1
#       set awkprocedure unforcedmakeawk
# to generate flow graph with forced drops:
#       set category 0
#       set awkprocedure forcedmakeawk

# Create a simple six node topology:
#
#        s1		    s3
#         \ 		    /
# 10Mb,2ms \  1.5Mb,20ms   / 10Mb,4ms
#           r1 --------- r2
# 10Mb,3ms /		   \ 10Mb,5ms
#         /		    \
#        s2		    s4 
#
proc create_testnet2 { } {

	global s1 s2 r1 r2 s3 s4 
	# This line below was added for NS-2.
	Queue/RED set ns1_compat_ true
	set s1 [ns node]
	set s2 [ns node]
	set r1 [ns node]
	set r2 [ns node]
	set s3 [ns node]
	set s4 [ns node]

	ns_duplex $s1 $r1 10Mb 2ms drop-tail
	ns_duplex $s2 $r1 10Mb 3ms drop-tail
	set L [ns_duplex $r1 $r2 1.5Mb 20ms red]
	[lindex $L 0] set queue-limit 25
	[lindex $L 1] set queue-limit 25
	ns_duplex $s3 $r2 10Mb 4ms drop-tail
	ns_duplex $s4 $r2 10Mb 5ms drop-tail
}

proc finish file {
	global quiet

	#
	# split queue/drop events into two separate files.
	# we don't bother checking for the link we're interested in
	# since we know only such events are in our trace file
	#
	set awkCode {
		{
			if (($1 == "+" || $1 == "-" ) && \
			  ($5 == "tcp" || $5 == "ack"))
				print $2, $8 + ($11 % 90) * 0.01 >> "temp.p";
			else if ($1 == "d")
				print $2, $8 + ($11 % 90) * 0.01 >> "temp.d";
		}
	}

	set f [open temp.rands w]
	puts $f "TitleText: $file"
	puts $f "Device: Postscript"
	
	exec rm -f temp.p temp.d 
	exec touch temp.d temp.p
	exec awk $awkCode out.tr

	puts $f \"packets
	flush $f
	exec cat temp.p >@ $f
	flush $f
	# insert dummy data sets so we get X's for marks in data-set 4
	puts $f [format "\n\"skip-1\n0 1\n\n\"skip-2\n0 1\n\n"]

	puts $f \"drops
	flush $f
	#
	# Repeat the first line twice in the drops file because
	# often we have only one drop and xgraph won't print marks
	# for data sets with only one point.
	#
	exec head -1 temp.d >@ $f
	exec cat temp.d >@ $f
	close $f
	if {$quiet == "false"} {
		exec xgraph -bb -tk -nl -m -x time -y packet temp.rands &
	}
	
	exit 0
}

proc plotQueue file {
	global quiet

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
	if {$quiet == "false"} {
		exec xgraph -bb -tk -x time -y queue temp.queue &
	}
}

#
# Arrange for tcp source stats to be dumped for $tcpSrc every
# $interval seconds of simulation time
#
proc tcpDump { tcpSrc interval } {
	proc dump { src interval } {
		global quiet
		ns at [expr [ns now] + $interval] "dump $src $interval"
		set report [ns now]/cwnd=[$src get cwnd]/ssthresh=[$src get ssthresh]/ack=[$src get ack]
		if {$quiet == "false"} {
			puts $report
		}
	}
	ns at 0.0 "dump $tcpSrc $interval"
}

proc openTrace { stopTime testName } {
	global r1 k1    
	set traceFile [open out.tr w]
	ns at $stopTime \
		"close $traceFile ; finish $testName"
	set T [ns trace]
	$T attach $traceFile
	return $T
}

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

proc tcpDumpAll { tcpSrc interval label } { 
        proc dump { src interval label } {  
                ns at [expr [ns now] + $interval] "dump $src $interval $label"
		puts time=[ns now]/class=$label/ack=[$src get ack]
        }
        puts $label:window=[$tcpSrc get window]/packet-size=[$tcpSrc get packet-size]
        ns at 0.0 "dump $tcpSrc $interval $label"
}       


proc flowDump { link fm flow category } {
	$fm dump $category
	$fm resetcounters
}

proc flowDump1 { link fm flow } {
	$fm dump 
	$fm resetcounters
}

proc test_red1 {} {
	global s1 s2 r1 r2 s3 s4 randseed
	create_testnet2
        set stoptime 10.0
	set testname test_red1
	
	set tcp1 [ns_create_connection tcp-reno $s1 tcp-sink $s3 0]
	$tcp1 set window 15

	set tcp2 [ns_create_connection tcp-reno $s2 tcp-sink $s3 1]
	$tcp2 set window 15

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 0.0 "$ftp1 start"
	ns at 3.0 "$ftp2 start"
	ns at $stoptime "plotQueue $testname"

	tcpDump $tcp1 5.0

	# trace only the bottleneck link
	[ns link $r1 $r2] trace [openTrace $stoptime $testname ]

	#puts seed=[ns random $randseed]
	ns run
}

# For this test the average queue size in measured in bytes.
proc test_red1_bytes {} {
	global s1 s2 r1 r2 s3 s4 randseed
	create_testnet2
        set stoptime 10.0
	set testname test_red1_bytes
        [ns link $r1 $r2] set queue_in_bytes true
        [ns link $r1 $r2] set bytes true
	[ns link $r1 $r2] set mean_pktsize 1000
	# the following 3 lines really don't matter
	# here because only 1-way traffic is being monitored,
	# but rather is for consistency
        [ns link $r2 $r1] set queue_in_bytes true
        [ns link $r2 $r1] set bytes true
	[ns link $r2 $r1] set mean_pktsize 1000
	
	set tcp1 [ns_create_connection tcp-reno $s1 tcp-sink $s3 0]
	$tcp1 set window 15

	set tcp2 [ns_create_connection tcp-reno $s2 tcp-sink $s3 1]
	$tcp2 set window 15

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 0.0 "$ftp1 start"
	ns at 3.0 "$ftp2 start"
	ns at $stoptime "plotQueue $testname"

	tcpDump $tcp1 5.0

	# trace only the bottleneck link
	[ns link $r1 $r2] trace [openTrace $stoptime $testname ]

	#puts seed=[ns random $randseed]
	ns run
}

proc test_ecn {} { 
        global s1 s2 r1 r2 s3 s4 randseed
        create_testnet2
	Agent/TCP set old_ecn_ 1
        set stoptime 10.0
        set testname test_ecn 
        [ns link $r1 $r2] set setbit true

        set tcp1 [ns_create_connection tcp-reno $s1 tcp-sink $s3 0]
        $tcp1 set window 15
        $tcp1 set ecn 1

        set tcp2 [ns_create_connection tcp-reno $s2 tcp-sink $s3 1]
        $tcp2 set window 15
        $tcp2 set ecn 1
        
        set ftp1 [$tcp1 source ftp]
        set ftp2 [$tcp2 source ftp]
        
        ns at 0.0 "$ftp1 start"
        ns at 3.0 "$ftp2 start"
        ns at $stoptime "plotQueue $testname"
        
        tcpDump $tcp1 5.0
        
        # trace only the bottleneck link
        [ns link $r1 $r2] trace [openTrace $stoptime $testname ]
        
        #puts seed=[ns random $randseed]
        ns run
}

# "Red2" changes some of the RED gateway parameters.
# This should give worse performance than "red1".
proc test_red2 {} {
	global s1 s2 r1 r2 s3 s4
	global randseed
	create_testnet2
	[ns link $r1 $r2] set thresh 5
	[ns link $r1 $r2] set maxthresh 10
	[ns link $r1 $r2] set q_weight 0.003
	set stoptime 10.0
	set testname test_red2
	
	set tcp1 [ns_create_connection tcp-reno $s1 tcp-sink $s3 0]
	$tcp1 set window 15

	set tcp2 [ns_create_connection tcp-reno $s2 tcp-sink $s3 1]
	$tcp2 set window 15

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 0.0 "$ftp1 start"
	ns at 3.0 "$ftp2 start"
	ns at $stoptime "plotQueue $testname"

	tcpDump $tcp1 5.0

	# trace only the bottleneck link
	[ns link $r1 $r2] trace [openTrace $stoptime $testname ]

	#puts seed=[ns random $randseed]
	ns run
}

# The queue is measured in "packets".
proc test_red_twoway {} {
	global s1 s2 r1 r2 s3 s4 randseed
	create_testnet2
	set stoptime 10.0
	set testname test_red_twoway
	
	set tcp1 [ns_create_connection tcp-reno $s1 tcp-sink $s3 0]
	$tcp1 set window 15
	set tcp2 [ns_create_connection tcp-reno $s2 tcp-sink $s4 1]
	$tcp2 set window 15
	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	set tcp3 [ns_create_connection tcp-reno $s3 tcp-sink $s1 2]
	$tcp3 set window 15
	set tcp4 [ns_create_connection tcp-reno $s4 tcp-sink $s2 3]
	$tcp4 set window 15
	set ftp3 [$tcp3 source ftp]
	set telnet1 [$tcp4 source telnet] ; $telnet1 set interval 0

	ns at 0.0 "$ftp1 start"
	ns at 2.0 "$ftp2 start"
	ns at 3.5 "$ftp3 start"
	ns at 1.0 "$telnet1 start"
	ns at $stoptime "plotQueue $testname"

	tcpDump $tcp1 5.0

	# trace only the bottleneck link
	[ns link $r1 $r2] trace [openTrace $stoptime $testname]

	#puts seed=[ns random $randseed]
	ns run
}

# The queue is measured in "bytes".
proc test_red_twowaybytes {} {
	global s1 s2 r1 r2 s3 s4 randseed
	create_testnet2
	set stoptime 10.0
	set testname test_red_twowaybytes
	[ns link $r1 $r2] set bytes true
	[ns link $r2 $r1] set bytes true
	[ns link $r1 $r2] set queue_in_bytes true
	[ns link $r2 $r1] set queue_in_bytes true
		
	set tcp1 [ns_create_connection tcp-reno $s1 tcp-sink $s3 0]
	$tcp1 set window 15
	set tcp2 [ns_create_connection tcp-reno $s2 tcp-sink $s4 1]
	$tcp2 set window 15
	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	set tcp3 [ns_create_connection tcp-reno $s3 tcp-sink $s1 2]
	$tcp3 set window 15
	set tcp4 [ns_create_connection tcp-reno $s4 tcp-sink $s2 3]
	$tcp4 set window 15
	set ftp3 [$tcp3 source ftp]
	set telnet1 [$tcp4 source telnet] ; $telnet1 set interval 0

	ns at 0.0 "$ftp1 start"
	ns at 2.0 "$ftp2 start"
	ns at 3.5 "$ftp3 start"
	ns at 1.0 "$telnet1 start"
	ns at $stoptime "plotQueue $testname"

	tcpDump $tcp1 5.0

	# trace only the bottleneck link
	[ns link $r1 $r2] trace [openTrace $stoptime $testname]

	#puts seed=[ns random $randseed]
	ns run
}

proc new_tcp { startTime source dest window class dump size } {
	set tcp [ns_create_connection tcp-reno $source tcp-sink $dest $class ]
	$tcp set window $window
	if {$size > 0}  {$tcp set packet-size $size }
	set ftp [$tcp source ftp]
	ns at $startTime "$ftp start"
        if {$dump == 1 } {tcpDumpAll $tcp 20.0 $class }
}

proc new_cbr { startTime source dest pktSize interval class } {
 	set cbr [ns_create_cbr $source $dest $pktSize $interval $class ]
	ns at $startTime "$cbr start"
}

proc flows {} {
	global s1 s2 r1 r2 s3 s4 r1fm qgraphfile flowfile 
        set stoptime 500.0
	set testname test_two
	
##	create_testnet2
##	[ns link $r1 $r2] set mean_pktsize 1000
##	[ns link $r2 $r1] set mean_pktsize 1000
##	[ns link $r1 $r2] set queue-limit 100
##	[ns link $r2 $r1] set queue-limit 100
##
##	create_flowstats 
##	[ns link $r1 $r2] set bytes true
##	[ns link $r1 $r2] set wait false
##
##      new_tcp 1.0 $s1 $s3 100 1 1 1000
##	new_tcp 1.2 $s2 $s4 100 2 1 50
##	new_cbr 1.4 $s1 $s4 190 0.003 3
##
##	ns at $stoptime "$r1fm flush"
##	ns at $stoptime "finish_flow $testname"
##
#	ns run
	puts "WARNING:  This test was not run."
	puts "This test is not implemented in backward compatibility mode."
}

proc test_flows {} {
	global category awkprocedure randseed
   	set category 1
	set awkprocedure unforcedmakeawk
	#set seed [ns random $randseed]
	#puts seed=$seed
	flows 
}

proc test_flows1 {} {
	global category awkprocedure randseed
   	set category 0
	set awkprocedure forcedmakeawk
	#set seed [ns random $randseed]
	#puts seed=$seed 
	flows 
}

proc test_flowsAll {} {
	global s1 s2 r1 r2 s3 s4 r1fm qgraphfile flowfile awkprocedure
	global randseed
        set stoptime 500.0
	set testname test_two
	set awkprocedure allmakeawk
	
##	create_testnet2
##	[ns link $r1 $r2] set mean_pktsize 1000
##	[ns link $r2 $r1] set mean_pktsize 1000
##	[ns link $r1 $r2] set queue-limit 100
##	[ns link $r2 $r1] set queue-limit 100
##
##	create_flowstats1  
##	[ns link $r1 $r2] set bytes true
##	[ns link $r1 $r2] set wait false
##
##      new_tcp 1.0 $s1 $s3 100 1 1 1000
##	new_tcp 1.2 $s2 $s4 100 2 1 50
##	new_cbr 1.4 $s1 $s4 190 0.003 3
##
##	ns at $stoptime "$r1fm flush"
##	ns at $stoptime "finish_flow $testname"
##
##	#puts seed=[ns random $randseed]
##	ns run
	puts "WARNING:  This test was not run."
	puts "This test is not implemented in backward compatibility mode."
}

proc setrandom { seed } {
	global randseed
	set randseed $seed
	puts "setting random number seed to $randseed"
	
}

if { $argc != 1 && $argc != 2 } {
	puts stderr {usage: ns test-suite-red.tcl testname <randseed> }
	exit 1
}
if { $argc == 2 } {
	global quiet
	set second [lindex $argv 1]
	set argv [lindex $argv 0]
	if { $second == "QUIET" || $second == "quiet" } {
		set quiet true
	} else {
		setrandom $second
	}
}
if { "[info procs test_$argv]" != "test_$argv" } {
	puts stderr "$argv: no such test: $argv"
}
test_$argv

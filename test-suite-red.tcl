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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/test-suite-red.tcl,v 1.1 1996/12/19 03:22:46 mccanne Exp $
#
#
# This test suite reproduces most of the tests from the following note:
# Floyd, S., Simulator Tests. July 1995.  
# URL ftp://ftp.ee.lbl.gov/papers/simtests.ps.Z.
#
# To run all tests:  test-all
# To run individual tests:
# ns test-suite.tcl tahoe1
# ns test-suite.tcl tahoe2
# ...
#
#
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
	exec xgraph -bb -tk -nl -m -x time -y packet temp.rands &
	
	exit 0
}

proc plotQueue file {

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

#
# Arrange for tcp source stats to be dumped for $tcpSrc every
# $interval seconds of simulation time
#
proc tcpDump { tcpSrc interval } {
	proc dump { src interval } {
		ns at [expr [ns now] + $interval] "dump $src $interval"
		puts [ns now]/cwnd=[$src get cwnd]/ssthresh=[$src get ssthresh]/ack=[$src get ack]
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

proc test_red1 {} {
	global s1 s2 r1 r2 s3 s4
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

	puts seed=[ns random 0]
	ns run
}

proc test_ecn {} { 
        global s1 s2 r1 r2 s3 s4
        create_testnet2
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
        
        puts seed=[ns random 0]
        ns run
}

# "Red2" changes some of the RED gateway parameters.
# This should give worse performance than "red1".
proc test_red2 {} {
	global s1 s2 r1 r2 s3 s4
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

	puts seed=[ns random 0]
	ns run
}

# The queue is measured in "packets".
proc test_red_twoway {} {
	global s1 s2 r1 r2 s3 s4
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

	puts seed=[ns random 0]
	ns run
}

if { $argc != 1 } {
	puts stderr {usage: ns test-suite.tcl [ tahoe1 tahoe2 ... reno reno2 ... ]}
	exit 1
}
if { "[info procs test_$argv]" != "test_$argv" } {
	puts stderr "test-suite.tcl: no such test: $argv"
}
test_$argv


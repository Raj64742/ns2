#
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/Attic/test-suite-sack-v1a.tcl,v 1.7 2001/11/28 23:04:27 sfloyd Exp $
#

#
# This file uses the ns-1 interfaces, not the newere v2 interfaces.
# Don't copy this code for use in new simulations,
# copy the new code with the new interfaces!
#

Agent/TCP set minrto_ 0
# The default is being changed to minrto_ 1
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

set quiet false
Agent/TCP set oldCode_ true

# Create a simple four node topology:
#
#        s1
#         \ 
#  8Mb,5ms \  0.8Mb,100ms
#           r1 --------- k1
#  8Mb,5ms /
#         /
#        s2
#
proc create_testnet { } {

	global s1 s2 r1 k1 quiet
	set s1 [ns node]
	set s2 [ns node]
	set r1 [ns node]
	set k1 [ns node]

	if {"$quiet" == "false"} {
		puts s1=[$s1 id]
		puts s2=[$s2 id]
		puts r1=[$r1 id]
		puts k1=[$k1 id]
	}

	ns_duplex $s1 $r1 8Mb 5ms drop-tail
	ns_duplex $s2 $r1 8Mb 5ms drop-tail
	set L [ns_duplex $r1 $k1 800Kb 100ms drop-tail]
	# transmission time for a 1000-byte packet:  0.01 sec.
	[lindex $L 0] set queue-limit 6
	[lindex $L 1] set queue-limit 6
}

proc create_testnet1 { } {

	global s1 s2 r1 k1
	set s1 [ns node]
	set s2 [ns node]
	set r1 [ns node]
	set k1 [ns node]

	ns_duplex $s1 $r1 10Mb 5ms drop-tail
	ns_duplex $s2 $r1 10Mb 5ms drop-tail
	set L [ns_duplex $r1 $k1 1.5Mb 100ms drop-tail]
	[lindex $L 0] set queue-limit 23
	[lindex $L 1] set queue-limit 23
}

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
	global quiet

	set f [open temp.rands w]
	puts $f "TitleText: $file"
	puts $f "Device: Postscript"
	
	exec rm -f temp.p temp.d 
	exec touch temp.d temp.p
	#
	# split queue/drop events into two separate files.
	# we don't bother checking for the link we're interested in
	# since we know only such events are in our trace file
	#
	exec awk {
		{
			if (($1 == "+" || $1 == "-" ) && \
			    ($5 == "tcp" || $5 == "ack"))\
					print $2, $8 + ($11 % 90) * 0.01
		}
	} out.tr > temp.p
	exec awk {
		{
			if ($1 == "d")
				print $2, $8 + ($11 % 90) * 0.01
		}
	} out.tr > temp.d
	exec awk {
		{
			if (($1 == "-" ) && \
			    ($5 == "tcp" || $5 == "ack"))\
					print $2, $8 + ($11 % 90) * 0.01
		}
	} out.tr1 > temp.p1

	puts $f \"packets
	flush $f
	exec cat temp.p >@ $f
	flush $f
#	puts $f \n\"acks
#	flush $f
#	exec cat temp.p1 >@ $f
	# insert dummy data sets so we get X's for marks in data-set 4
	puts $f [format "\n\" \n0 1\n\n\" \n0 1\n\n"]

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

proc tcpDumpAll { tcpSrc interval label } {
	proc dump { src interval label } {
		ns at [expr [ns now] + $interval] "dump $src $interval $label"
		puts $label/time=[ns now]/cwnd=[$src get cwnd]/ssthresh=[$src get ssthresh]/ack=[$src get ack]/rtt=[$src get rtt]
	}
	puts $label:window=[$tcpSrc get window]/packet-size=[$tcpSrc get packet-size]/bug-fix=[$tcpSrc get bug-fix]
	ns at 0.0 "dump $tcpSrc $interval $label"
}

proc openTraces { stopTime testName filename node1 node2} {
        global s1 s2 r1 k1
	exec rm -f $filename.tr $filename.tr1 temp.rands
	set traceFile [open $filename.tr w]
	ns at $stopTime \
		"close $traceFile" 
	set T [ns trace]
	$T attach $traceFile
	[ns link $node1 $node2] trace $T

	set traceFile1 [open $filename.tr1 w]
	ns at $stopTime \
		"close $traceFile1 ; finish $testName"
	set T1 [ns trace]
	$T1 attach $traceFile1
	[ns link $node2 $node1] trace $T1
}

#XXX need to flush pending output
Agent/TCP instproc error msg {
	$self instvar addr_
	puts stderr "tcp source at [expr $addr_ >> 8]: $msg"
	exit 1
}


# single packet drop
proc test_sack1 {} {
	global s1 s2 r1 k1
	create_testnet

	set tcp1 [ns_create_connection tcp-sack1 $s1 sack1-tcp-sink $k1 0]
	$tcp1 set window 14
	set ftp1 [$tcp1 source ftp]
	ns at 1.0 "$ftp1 start"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
        openTraces 5.0 test_sack1_window_14 out $r1 $k1

	ns run
}

# three packet drops
proc test_sack1a {} {
	global s1 s2 r1 k1
	create_testnet
	set tcp1 [ns_create_connection tcp-sack1 $s1 sack1-tcp-sink $k1 0]
	$tcp1 set window 20
	set ftp1 [$tcp1 source ftp]
	ns at 0.0 "$ftp1 start"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
	openTraces 5.0 test_sack1a_window_20 out $r1 $k1

	ns run
}

proc test_sack1b {} {
	global s1 s2 r1 k1
	create_testnet
	set tcp1 [ns_create_connection tcp-sack1 $s1 sack1-tcp-sink $k1 0]
	$tcp1 set window 26
	set ftp1 [$tcp1 source ftp]
	ns at 0.0 "$ftp1 start"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
	openTraces 5.0 test_sack1b_window_26 out $r1 $k1

	ns run
}

proc test_sack1c {} {
	global s1 s2 r1 k1
	create_testnet
	set tcp1 [ns_create_connection tcp-sack1 $s1 sack1-tcp-sink $k1 0]
	$tcp1 set window 27
	set ftp1 [$tcp1 source ftp]
	ns at 0.0 "$ftp1 start"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
	openTraces 5.0 test_sack1c_window_27 out $r1 $k1

	ns run
}

# this does not seem right
proc test_sack3 {} {
	global s1 s2 r1 k1
	create_testnet
	[ns link $r1 $k1] set queue-limit 8
	[ns link $k1 $r1] set queue-limit 8
	
	set tcp1 [ns_create_connection tcp-sack1 $s1 sack1-tcp-sink $k1 0]
	$tcp1 set window 100
	$tcp1 set bug-fix false

	set tcp2 [ns_create_connection tcp-sack1 $s2 sack1-tcp-sink $k1 1]
	$tcp2 set window 16
	$tcp2 set bug-fix false

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 1.0 "$ftp1 start"
	ns at 0.5 "$ftp2 start"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
	openTraces 8.0 test_sack3 out $r1 $k1

	ns run
}

proc test_sack4 {} {
	global s1 s2 r1 k1
	create_testnet
	[ns link $s2 $r1] set delay 200ms
	[ns link $r1 $s2] set delay 200ms
	[ns link $r1 $k1] set queue-limit 11
	[ns link $k1 $r1] set queue-limit 11
	
	set tcp1 [ns_create_connection tcp-sack1 $s1 sack1-tcp-sink $k1 0]
	$tcp1 set window 30

	set tcp2 [ns_create_connection tcp-sack1 $s2 sack1-tcp-sink $k1 1]
	$tcp2 set window 30

	set ftp1 [$tcp1 source ftp]
	## $ftp1 set maxpkts 10
	set ftp2 [$tcp2 source ftp]
	## $ftp2 set maxpkts 30

	ns at 0.0 "$ftp1 start"
	ns at 0.0 "$ftp2 start"

	tcpDump $tcp1 5.0

	# trace only the bottleneck link
	openTraces 25.0 test_sack4 out $r1 $k1

	ns run
}

proc test_sack5 {} {

	global s1 s2 r1 k1
	create_testnet1
	[ns link $s1 $r1] set delay 3ms
	[ns link $r1 $s1] set delay 3ms 

	set tcp1 [ns_create_connection tcp-sack1 $s1 sack1-tcp-sink $k1 0]
	$tcp1 set window 50
	$tcp1 set bug-fix false

	set tcp2 [ns_create_connection tcp-sack1 $s2 sack1-tcp-sink $k1 1]
	$tcp2 set window 50
	$tcp2 set bug-fix false

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 1.0 "$ftp1 start"
	ns at 1.75 "$ftp2 produce 100"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
	openTraces 6.0 test_sack5 out $r1 $k1

	ns run
}

# shows a long recovery from sack.
proc test_sackB2 {} {
	global s1 s2 r1 k1
	create_testnet
	[ns link $r1 $k1] set queue-limit 9

	set tcp1 [ns_create_connection tcp-sack1 $s1 sack1-tcp-sink $k1 0]
	$tcp1 set window 50

	set tcp2 [ns_create_connection tcp-sack1 $s2 sack1-tcp-sink $k1 1]
	$tcp2 set window 20

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 1.0 "$ftp1 start"
	ns at 1.0 "$ftp2 start"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
	openTraces 10.0 test_sackB2 out $r1 $k1

	ns run
}

# two packets dropped
proc test_sackB4 {} { 
        global s1 s2 r1 r2 s3 s4
        create_testnet2
        [ns link $r1 $r2] set queue-limit 29
        set tcp1 [ns_create_connection tcp-sack1 $s1 sack1-tcp-sink $r2 0]
        $tcp1 set window 40

        set ftp1 [$tcp1 source ftp]
        ns at 0.0 "$ftp1 start"

        tcpDump $tcp1 1.0

        # trace only the bottleneck link
	openTraces 2.0 test_sackB4 out $r1 $r2

        ns run
}

# delayed ack not implemented yet
# proc test_delayedSack {} {
# 	global s1 s2 r1 k1
# 	create_testnet
# 	set tcp1 [ns_create_connection tcp-sack1 $s1 sack1-tcp-sink $k1 0]
# 	$tcp1 set window 50
# 
# 	# lookup up the sink and set it's delay interval
# 	[$k1 agent [$tcp1 dst-port]] set interval 100ms
# 
# 	set ftp1 [$tcp1 source ftp]
# 	ns at 1.0 "$ftp1 start"
# 
# 	tcpDump $tcp1 1.0
# 
# 	# trace only the bottleneck link
# 	[ns link $r1 $k1] trace [openTraces 4.0 test_delayedSack]
# 
# 	ns run
# }

# segregation
proc test_phaseSack {} {
	global s1 s2 r1 k1
	create_testnet
	[ns link $s2 $r1] set delay 3ms
	[ns link $r1 $s2] set delay 3ms
	[ns link $r1 $k1] set queue-limit 16
	[ns link $k1 $r1] set queue-limit 100

	set tcp1 [ns_create_connection tcp-sack1 $s1 sack1-tcp-sink $k1 0]
	$tcp1 set window 32

	set tcp2 [ns_create_connection tcp-sack1 $s2 sack1-tcp-sink $k1 1]
	$tcp2 set window 32

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 5.0 "$ftp1 start"
	ns at 1.0 "$ftp2 start"

	tcpDump $tcp1 5.0

	# trace only the bottleneck link
	openTraces 25.0 test_phaseSack out $r1 $k1

	ns run
}

# random overhead, but segregation remains 
# This is because of the phase of the "tcp1" connection, where
#  packets arrive at the router right after a slot is freed in the buffer.
proc test_phaseSack2 {} {
        global s1 s2 r1 k1
        create_testnet  
        [ns link $s2 $r1] set delay 3ms
        [ns link $r1 $s2] set delay 3ms
        [ns link $r1 $k1] set queue-limit 16
        [ns link $k1 $r1] set queue-limit 100

        set tcp1 [ns_create_connection tcp-sack1 $s1 sack1-tcp-sink $k1 0]
        $tcp1 set window 32
        $tcp1 set overhead 0.01

        set tcp2 [ns_create_connection tcp-sack1 $s2 sack1-tcp-sink $k1 1]
        $tcp2 set window 32
        $tcp2 set overhead 0.01
        
        set ftp1 [$tcp1 source ftp]
        set ftp2 [$tcp2 source ftp]
 
        ns at 5.0 "$ftp1 start"
        ns at 1.0 "$ftp2 start"
        
        tcpDump $tcp1 5.0
 
        # trace only the bottleneck link
	openTraces 25.0 test_phaseSack2_random_overhead? out $r1 $k1

        ns run
}

# no segregation, because of random overhead
proc test_phaseSack3 {} {
	global s1 s2 r1 k1
	create_testnet
	[ns link $s2 $r1] set delay 9.5ms
	[ns link $r1 $s2] set delay 9.5ms
	[ns link $r1 $k1] set queue-limit 16
	[ns link $k1 $r1] set queue-limit 100

	set tcp1 [ns_create_connection tcp-sack1 $s1 sack1-tcp-sink $k1 0]
	$tcp1 set window 32
        $tcp1 set overhead 0.01

	set tcp2 [ns_create_connection tcp-sack1 $s2 sack1-tcp-sink $k1 1]
	$tcp2 set window 32
        $tcp2 set overhead 0.01

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 5.0 "$ftp1 start"
	ns at 1.0 "$ftp2 start"

	tcpDump $tcp1 5.0

	# trace only the bottleneck link
	openTraces 25.0 test_phaseSack3_random_overhead out $r1 $k1

	ns run
}

# proc test_timersSack {} {
# 	global s1 s2 r1 k1
# 	create_testnet
# 	[ns link $r1 $k1] set queue-limit 2
# 	[ns link $k1 $r1] set queue-limit 100
# 
# 	set tcp1 [ns_create_connection tcp-sack1 $s1 sack1-tcp-sink $k1 0]
# 	$tcp1 set window 4
# 	# lookup up the sink and set it's delay interval
# 	[$k1 agent [$tcp1 dst-port]] set interval 100ms
# 
# 	set tcp2 [ns_create_connection tcp-sack1 $s2 sack1-tcp-sink $k1 1]
# 	$tcp2 set window 4
# 	# lookup up the sink and set it's delay interval
# 	[$k1 agent [$tcp2 dst-port]] set interval 100ms
# 
# 	set ftp1 [$tcp1 source ftp]
# 	set ftp2 [$tcp2 source ftp]
# 
# 	ns at 1.0 "$ftp1 start"
# 	ns at 1.3225 "$ftp2 start"
# 
# 	tcpDump $tcp1 5.0
# 
# 	# trace only the bottleneck link
# 	[ns link $r1 $k1] trace [openTraces 10.0 test_timersSack]
# 
# 	ns run
# }

if { $argc != 1 && $argc != 2 } {
        puts stderr {usage: ns test-suite-cbq.tcl testname}
        exit 1
} 
if { $argc == 2 } {
        global quiet
        set second [lindex $argv 1]
        set argv [lindex $argv 0]
        if { $second == "QUIET" || $second == "quiet" } {
                set quiet true
        }
} 
if { "[info procs test_$argv]" != "test_$argv" } {
	puts stderr "test-suite.tcl: no such test: $argv"
}
test_$argv


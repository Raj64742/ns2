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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/test-suite-v2.tcl,v 1.1 1997/03/28 20:12:03 tomh Exp $
#
#
# This test suite reproduces most of the tests from the following note:
# Floyd, S., Simulator Tests. July 1995.  
# URL ftp://ftp.ee.lbl.gov/papers/simtests.ps.Z.
#
# To run individual tests:
# ns test-suite.tcl tahoe1
# ns test-suite.tcl tahoe2
# ...
#
#
# Create a simple four node topology:
#
#	   s1
#	     \ 
#  8Mb,5ms \   0.8Mb,100ms
#	        r1 --------- k1
#  8Mb,5ms /
#	     /
#	   s2
#
Simulator instproc create_testnet {} {

	$self instvar s1 s2 r1 k1
	set s1 [$self node]
	set s2 [$self node]
	set r1 [$self node]
	set k1 [$self node]

	$self duplex-link $s1 $r1 8Mb 5ms DropTail
	$self duplex-link $s2 $r1 8Mb 5ms DropTail
	$self duplex-link $r1 $k1 800Kb 100ms DropTail
	$self queue-limit $r1 $k1 6
	$self queue-limit $k1 $r1 6
}

Simulator instproc create_testnet1 {} {
	$self instvar s1 s2 r1 k1
	set s1 [$self node]
	set s2 [$self node]
	set r1 [$self node]
	set k1 [$self node]

	$self duplex-link $s1 $r1 10Mb 5ms DropTail
	$self duplex-link $s2 $r1 10Mb 5ms DropTail
	$self duplex-link $r1 $k1 1.5Mb 100ms DropTail
	$self queue-limit $r1 $k1 23
	$self queue-limit $k1 $r1 23
}

#
# Create a simple six node topology:
#
#        s1                 s3
#         \                 /
# 10Mb,2ms \  1.5Mb,20ms   / 10Mb,4ms
#           r1 --------- r2
# 10Mb,3ms /               \ 10Mb,5ms
#         /                 \
#        s2                 s4 
#
Simulator instproc create_testnet2 {} {
	$self instvar s1 s2 r1 r2 s3 s4
	set s1 [$self node]
	set s2 [$self node]
	set r1 [$self node]
	set r2 [$self node]
	set s3 [$self node]
	set s4 [$self node]

	$self duplex-link $s1 $r1 10Mb 2ms DropTail
	$self duplex-link $s2 $r1 10Mb 3ms DropTail
	$self duplex-link $r1 $r2 1.5Mb 20ms RED
	$self queue-limit $r1 $r2 25
	$self queue-limit $r2 $r1 25
	$self duplex-link $s3 $r2 10Mb 4ms DropTail
	$self duplex-link $s4 $r2 10Mb 5ms DropTail
}

proc finish file {

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

#
# Arrange for tcp source stats to be dumped for $tcpSrc every
# $interval seconds of simulation time
#
Simulator instproc tcpDump { tcpSrc interval } {
	$self instvar dump_inst_
	if ![info exists dump_inst_($tcpSrc)] {
		set dump_inst_($tcpSrc) 1
		$self at 0.0 "$self tcpDump $tcpSrc $interval"
		return
	}
	$self at [expr [$self now] + $interval] "$self tcpDump $tcpSrc $interval"
	puts [$self now]/cwnd=[format "%.4f" [$tcpSrc set cwnd_]]/ssthresh=[$tcpSrc set ssthresh_]/ack=[$tcpSrc set ack_]
}

Simulator instproc tcpDumpAll { tcpSrc interval label } {
	$self instvar dump_inst_
	if ![info exists dump_inst_($tcpSrc)] {
		set dump_inst_($tcpSrc) 1
		puts $label/window=[$tcpSrc set window_]/packetSize=[$tcpSrc set packetSize_]]/bugFix=[$tcpSrc set bugFix_]	
		$self at 0.0 "$self tcpDumpAll $tcpSrc $interval $label"
		return
	}
	$self at [expr [$self now] + $interval] "$self tcpDumpAll $tcpSrc $interval $label"
	puts $label/time=[$self now]/cwnd=[format "%.4f" [$tcpSrc set cwnd_]]/ssthresh=[$tcpSrc set ssthresh_]/ack=[$tcpSrc set ack_]/rtt=[$tcpSrc set rtt_]	
}

Simulator instproc openTrace { stopTime testName } {
	exec rm -f out.tr temp.rands
	set traceFile [open out.tr w]
	$self at $stopTime \
		"close $traceFile ; finish $testName"
	return $traceFile
}

proc test_tahoe1 {} {

	# Create instance of simulator
	set ns [new Simulator]

	# Set up testbed
	$ns create_testnet
	$ns instvar s1 s2 r1 k1
   
	# Set up TCP connection
	set tcp1 [$ns create-connection TCP $s1 TCPSink $k1 0]
	$tcp1 set window_ 50

	# Set up FTP source
	set ftp1 [$tcp1 attach-source FTP]
	$ns at 0.0 "$ftp1 start"

	$ns tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 5.0 test_tahoe]

	$ns run
}

proc test_tahoe2 {} {

	set ns [new Simulator]

	$ns create_testnet
	$ns instvar s1 s2 r1 k1

	set tcp1 [$ns create-connection TCP $s1 TCPSink $k1 0]
	$tcp1 set window_ 14

	set ftp1 [$tcp1 attach-source FTP]
	$ns at 1.0 "$ftp1 start"

	$ns tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 5.0 test_tahoe2]

	$ns run
}

proc test_tahoe3 {} {

	set ns [new Simulator]
	$ns create_testnet
	$ns instvar s1 s2 r1 k1
	$ns queue-limit $r1 $k1 8   
	$ns queue-limit $k1 $r1 8   

	set tcp1 [$ns create-connection TCP $s1 TCPSink $k1 0]
	$tcp1 set window_ 100
	set tcp2 [$ns create-connection TCP $s2 TCPSink $k1 1]
	$tcp2 set window_ 16

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns at 1.0 "$ftp1 start"
	$ns at 0.5 "$ftp2 start"

	$ns tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 8.0 test_tahoe3]

	$ns run
}

proc test_tahoe4 {} {
	
	set ns [new Simulator]
	$ns create_testnet
	$ns instvar s1 s2 r1 k1
	$ns delay $s2 $r1 200ms
	$ns delay $r1 $s2 200ms 
	$ns queue-limit $r1 $k1 11
	$ns queue-limit $k1 $r1 11

	set tcp1 [$ns create-connection TCP $s1 TCPSink $k1 0]
	$tcp1 set window_ 30
	set tcp2 [$ns create-connection TCP $s2 TCPSink $k1 1]
	$tcp2 set window_ 30

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns at 0.0 "$ftp1 start"
	$ns at 0.0 "$ftp2 start"
	
	$ns tcpDump $tcp1 5.0

	# trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 25.0 test_tahoe4]

	$ns run
}

proc test_no_bug {} {
	
	set ns [new Simulator]
	$ns create_testnet1
	$ns instvar s1 s2 r1 k1
	$ns delay $s1 $r1 3ms
	$ns delay $r1 $s1 3ms

	set tcp1 [$ns create-connection TCP $s1 TCPSink $k1 0]
	$tcp1 set window_ 50
	set tcp2 [$ns create-connection TCP $s2 TCPSink $k1 1]
	$tcp2 set window_ 50

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]
	
	$ns at 1.0 "$ftp1 start"
	$ns at 1.75 "$ftp2 produce 100"

	$ns tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 6.0 test_no_bug]
	
	$ns run
}

proc test_bug {} {
	
	set ns [new Simulator]
	$ns create_testnet1
	$ns instvar s1 s2 r1 k1
	$ns delay $s1 $r1 3ms
	$ns delay $r1 $s1 3ms

	set tcp1 [$ns create-connection TCP $s1 TCPSink $k1 0]
	$tcp1 set window_ 50
	$tcp1 set bugFix_ false
	set tcp2 [$ns create-connection TCP $s2 TCPSink $k1 1]
	$tcp2 set window_ 50
	$tcp2 set bugFix_ false

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]
	
	$ns at 1.0 "$ftp1 start"
#	$ns at 1.0 "$ftp2 start"
	$ns at 1.75 "$ftp2 produce 100"

	$ns tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 6.0 test_bug]
	
	$ns run
}

proc test_reno1 {} {

	set ns [new Simulator]

	$ns create_testnet
	$ns instvar s1 s2 r1 k1
   
	set tcp1 [$ns create-connection TCP/Reno $s1 TCPSink $k1 0]
	$tcp1 set window_ 14

	set ftp1 [$tcp1 attach-source FTP]
	$ns at 1.0 "$ftp1 start"

	$ns tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 5.0 test_reno1]

	$ns run
}

proc test_reno {} {

	set ns [new Simulator]

	$ns create_testnet
	$ns instvar s1 s2 r1 k1
   
	set tcp1 [$ns create-connection TCP/Reno $s1 TCPSink $k1 0]
	$tcp1 set window_ 28
	$tcp1 set maxcwnd_ 14

	set ftp1 [$tcp1 attach-source FTP]
	$ns at 1.0 "$ftp1 start"

	$ns tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 5.0 test_reno]

	$ns run
}

proc test_renoA {} {

	set ns [new Simulator]

	$ns create_testnet
	$ns instvar s1 s2 r1 k1
	$ns queue-limit $r1 $k1 8   
   
	set tcp1 [$ns create-connection TCP/Reno $s1 TCPSink $k1 0]
	$tcp1 set window_ 28
	set tcp2 [$ns create-connection TCP/Reno $s1 TCPSink $k1 1]
	$tcp2 set window_ 4
	set tcp3 [$ns create-connection TCP/Reno $s1 TCPSink $k1 2]
	$tcp3 set window_ 4

	set ftp1 [$tcp1 attach-source FTP]
	$ns at 1.0 "$ftp1 start"
	set ftp2 [$tcp2 attach-source FTP]
	$ns at 1.2 "$ftp2 produce 7"
	set ftp3 [$tcp3 attach-source FTP]
	$ns at 1.2 "$ftp3 produce 7"

	$ns tcpDump $tcp1 1.0
	$ns tcpDump $tcp2 1.0
	$ns tcpDump $tcp3 1.0

	# trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 5.0 test_renoA]

	$ns run
}

proc test_reno2 {} {

	set ns [new Simulator]

	$ns create_testnet
	$ns instvar s1 s2 r1 k1
	$ns queue-limit $r1 $k1 9
   
	set tcp1 [$ns create-connection TCP/Reno $s1 TCPSink $k1 0]
	$tcp1 set window_ 50
	set tcp2 [$ns create-connection TCP/Reno $s1 TCPSink $k1 1]
	$tcp2 set window_ 20

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns at 1.0 "$ftp1 start"
	$ns at 1.0 "$ftp2 start"

	$ns tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 10.0 test_reno2]

	$ns run
}

proc test_reno3 {} {

	set ns [new Simulator]

	$ns create_testnet
	$ns instvar s1 s2 r1 k1
	$ns queue-limit $r1 $k1 8
	$ns queue-limit $k1 $r1 8 
   
	set tcp1 [$ns create-connection TCP/Reno $s1 TCPSink $k1 0]
	$tcp1 set window_ 100
	set tcp2 [$ns create-connection TCP/Reno $s1 TCPSink $k1 1]
	$tcp2 set window_ 16

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns at 1.0 "$ftp1 start"
	$ns at 0.5 "$ftp2 start"

	$ns tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 8.0 test_reno3]

	$ns run
}

proc test_reno4 {} {

	set ns [new Simulator]

	$ns create_testnet2
	$ns instvar s1 s2 r1 r2 s3 s4
	$ns queue-limit $r1 $r2 29
   
	set tcp1 [$ns create-connection TCP/Reno $s1 TCPSink/DelAck $r2 0]
	$tcp1 set window_ 80
	$tcp1 set maxcwnd_ 40

	set ftp1 [$tcp1 attach-source FTP]
	$ns at 0.0 "$ftp1 start"

	$ns tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$ns trace-queue $s1 $r1 [$ns openTrace 2.0 test_reno4]

	$ns run
}

proc test_reno4a {} {

	set ns [new Simulator]

	$ns create_testnet2
	$ns instvar s1 s2 r1 r2 s3 s4
	$ns queue-limit $r1 $r2 29
   
	set tcp1 [$ns create-connection TCP/Reno $s1 TCPSink/DelAck $r2 0]
	$tcp1 set window_ 40
	$tcp1 set maxcwnd_ 40

	set ftp1 [$tcp1 attach-source FTP]
	$ns at 0.0 "$ftp1 start"

	$ns tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$ns trace-queue $s1 $r1 [$ns openTrace 2.0 test_reno4a]

	$ns run
}

proc test_reno5 {} {

	set ns [new Simulator]

	$ns create_testnet
	$ns instvar s1 s2 r1 k1
	$ns queue-limit $r1 $k1 9
   
	set tcp1 [$ns create-connection TCP/Reno $s1 TCPSink $k1 0]
	$tcp1 set window_ 50
	$tcp1 set bugFix_ false
	set tcp2 [$ns create-connection TCP/Reno $s2 TCPSink $k1 1]
	$tcp2 set window_ 20
	$tcp2 set bugFix_ false

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns at 1.0 "$ftp1 start"
	$ns at 1.0 "$ftp2 start"

	$ns tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 10.0 test_reno5]

	$ns run
}

proc test_telnet {} {

	set ns [new Simulator]

	$ns create_testnet
	$ns instvar s1 s2 r1 k1
	$ns queue-limit $r1 $k1 8   
	$ns queue-limit $k1 $r1 8   
   
	set tcp1 [$ns create-connection TCP/Reno $s1 TCPSink $k1 0]
	set tcp2 [$ns create-connection TCP/Reno $s2 TCPSink $k1 1]
	set tcp3 [$ns create-connection TCP/Reno $s2 TCPSink $k1 2]

	set telnet1 [$tcp1 attach-source TELNET]; $telnet1 set interval_ 1
	set telnet2 [$tcp2 attach-source TELNET]; $telnet2 set interval_ 0 
	# Interval 0 designates the tcplib telnet interarrival distribution
	set telnet3 [$tcp3 attach-source TELNET]; $telnet3 set interval_ 0 

	$ns at 0.0 "$telnet1 start"
	$ns at 0.0 "$telnet2 start"
	$ns at 0.0 "$telnet3 start"

	$ns tcpDump $tcp1 5.0

	# trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 50.0 test_telnet]

	# use a different seed each time
	puts seed=[$ns random 0] 

	$ns run
}

proc test_delayed {} {

	set ns [new Simulator]

	$ns create_testnet
	$ns instvar s1 s2 r1 k1

	set tcp1 [$ns create-connection TCP $s1 TCPSink/DelAck $k1 0]
	$tcp1 set window_ 50

	set ftp1 [$tcp1 attach-source FTP]
	$ns at 1.0 "$ftp1 start"

	$ns tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 4.0 test_delayed]

	$ns run
}

proc test_phase {} {

	set ns [new Simulator]

	$ns create_testnet
	$ns instvar s1 s2 r1 k1
	$ns delay $s2 $r1 3ms
	$ns delay $r1 $s2 3ms
	$ns queue-limit $r1 $k1 16
	$ns queue-limit $k1 $r1 100

	set tcp1 [$ns create-connection TCP $s1 TCPSink $k1 0]
	$tcp1 set window_ 32 
	set tcp2 [$ns create-connection TCP $s2 TCPSink $k1 1]
	$tcp2 set window_ 32 

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns at 5.0 "$ftp1 start"
	$ns at 1.0 "$ftp2 start"

	$ns tcpDump $tcp1 5.0

	# trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 25.0 test_phase]

	$ns run
}

proc test_phase1 {} {

	set ns [new Simulator]

	$ns create_testnet
	$ns instvar s1 s2 r1 k1
	$ns delay $s2 $r1 9.5ms
	$ns delay $r1 $s2 9.5ms
	$ns queue-limit $r1 $k1 16
	$ns queue-limit $k1 $r1 100

	set tcp1 [$ns create-connection TCP $s1 TCPSink $k1 0]
	$tcp1 set window_ 32 
	set tcp2 [$ns create-connection TCP $s2 TCPSink $k1 1]
	$tcp2 set window_ 32 

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns at 5.0 "$ftp1 start"
	$ns at 1.0 "$ftp2 start"

	$ns tcpDump $tcp1 5.0

	# trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 25.0 test_phase1]

	$ns run
}

proc test_phase2 {} {

	set ns [new Simulator]

	$ns create_testnet
	$ns instvar s1 s2 r1 k1
	$ns delay $s2 $r1 3ms
	$ns delay $r1 $s2 3ms
	$ns queue-limit $r1 $k1 16
	$ns queue-limit $k1 $r1 100

	set tcp1 [$ns create-connection TCP $s1 TCPSink $k1 0]
	$tcp1 set window_ 32 
	$tcp1 set overhead_ 0.01
	set tcp2 [$ns create-connection TCP $s2 TCPSink $k1 1]
	$tcp2 set window_ 32 
	$tcp2 set overhead_ 0.01

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns at 5.0 "$ftp1 start"
	$ns at 1.0 "$ftp2 start"

	$ns tcpDump $tcp1 5.0

	# trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace 25.0 test_phase2]

	$ns run
}

proc test_timers {} {

	set ns [new Simulator]

	$ns create_testnet
	$ns instvar s1 s2 r1 k1
	$ns queue-limit $r1 $k1 2
	$ns queue-limit $k1 $r1 100

	set tcp1 [$ns create-connection TCP $s1 TCPSink/DelAck $k1 0]
	$tcp1 set window_ 4
	# look up the sink and set its delay interval
	[$k1 agent [$tcp1 dst-port]] set interval_ 100ms
	set tcp2 [$ns create-connection TCP $s2 TCPSink/DelAck $k1 1]
	$tcp2 set window_ 32 
	# look up the sink and set its delay interval
	[$k1 agent [$tcp2 dst-port]] set interval_ 100ms

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns at 1.0 "$ftp1 start"
	$ns at 1.3225 "$ftp2 start"

	$ns tcpDump $tcp1 5.0

	$ns trace-queue $r1 $k1 [$ns openTrace 10.0 test_timers]

	$ns run
}

proc printpkts { label tcp } {
	puts "tcp $label total_packets_acked [$tcp set ack_]"
}

#XXX Still unfinished in ns-2
proc printdrops { label link } {
	puts "link $label total_drops [$link stat 0 drops]"
	puts "link $label total_packets [$link stat 0 packets]"
	puts "link $label total_bytes [$link stat 0 bytes]"
}

proc printstop { stoptime } {
	puts "stop-time $stoptime"
}

proc test_stats {} {

	set ns [new Simulator]

	$ns create_testnet
	$ns instvar s1 s2 r1 k1
	$ns delay $s2 $r1 200ms
	$ns delay $r1 $s2 200ms
	$ns queue-limit $r1 $k1 10
	$ns queue-limit $k1 $r1 10

	set stoptime 10.1 

	set tcp1 [$ns create-connection TCP $s1 TCPSink $k1 0]
	$tcp1 set window_ 30
	set tcp2 [$ns create-connection TCP $s2 TCPSink $k1 1]
	$tcp2 set window_ 30

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns at 1.0 "$ftp1 start"
	$ns at 1.0 "$ftp2 start"

	$ns tcpDumpAll $tcp1 5.0 tcp1
	$ns tcpDumpAll $tcp2 5.0 tcp2

	$ns at $stoptime "printstop $stoptime"
	$ns at $stoptime "printpkts 1 $tcp1"
	#XXX Awaiting completion of link stats
	#$ns at $stoptime "printdrops 1 [$ns link $r1 $k1]"

	# trace only the bottleneck link
	$ns trace-queue $r1 $k1 [$ns openTrace $stoptime test_stats]

	$ns run
}


if { $argc != 1 } {
	puts stderr {usage: ns test-suite.tcl [ tahoe1 tahoe2 ... reno reno2 ... ]}
	exit 1
}
if { "[info procs test_$argv]" != "test_$argv" } {
	puts stderr "test-suite.tcl: no such test: $argv"
}
test_$argv


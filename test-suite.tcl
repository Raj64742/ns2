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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/test-suite.tcl,v 1.15 1998/05/06 18:15:10 kfall Exp $
#
#
# This test suite reproduces most of the tests from the following note:
# Floyd, S., Simulator Tests. July 1995.  
# URL ftp://ftp.ee.lbl.gov/papers/simtests.ps.Z.
#
# To run all tests:  test-all
#
# To run individual tests:
# ns test-suite.tcl tahoe1
# ns test-suite.tcl tahoe2
# ...
#
# To view a list of available tests to run with this script:
# ns test-suite.tcl
#
# Much more extensive test scripts are available in tcl/test
# This script is a simplified version of tcl/test/test-suite-routed.tcl

Class TestSuite

TestSuite instproc init {} {
	$self instvar ns_ net_ defNet_ test_ topo_ node_ testName_
	set ns_ [new Simulator]
	# trace-all is only used in more extensive test suites
	# $ns_ trace-all [open all.tr w]
	if {$net_ == ""} {
		set net_ $defNet_
	}
	if ![Topology/$defNet_ info subclass Topology/$net_] {
		global argv0
		puts "$argv0: cannot run test $test_ over topology $net_"
		exit 1
	}

	set topo_ [new Topology/$net_ $ns_]
	foreach i [$topo_ array names node_] {
		# This would be cool, but lets try to be compatible
		# with test-suite.tcl as far as possible.
		#
		# $self instvar $i
		# set $i [$topo_ node? $i]
		#
		set node_($i) [$topo_ node? $i]
	}

	if {$net_ == $defNet_} {
		set testName_ "$test_"
	} else {
		set testName_ "$test_:$net_"
	}

	# XXX
	if [info exists node_(k1)] {
		set blink [$ns_ link $node_(r1) $node_(k1)]
	} else {
		set blink [$ns_ link $node_(r1) $node_(r2)] 
	}
	$blink trace-dynamics $ns_ stdout
}

TestSuite instproc finish file {
	global env quiet

	#
	# we don't bother checking for the link we're interested in
	# since we know only such events are in our trace file
	#
	set perlCode {
		sub BEGIN { $c = 0; @p = @a = @d = @lu = @ld = (); }
		/^[\+-] / && do {
			if ($F[4] eq 'tcp') {
 				push(@p, $F[1], ' ',		\
					$F[7] + ($F[10] % 90) * 0.01, "\n");
			} elsif ($F[4] eq 'ack') {
 				push(@a, $F[1], ' ',		\
					$F[7] + ($F[10] % 90) * 0.01, "\n");
			}
			$c = $F[7] if ($c < $F[7]);
			next;
		};
		/^d / && do {
			push(@d, $F[1], ' ',		\
					$F[7] + ($F[10] % 90) * 0.01, "\n");
			next;
		};
		/link-down/ && push(@ld, $F[1]);
		/link-up/ && push(@lu, $F[1]);
		sub END {
			print "\"packets\n", @p, "\n";
			# insert dummy data sets
			# so we get X's for marks in data-set 4
			print "\"skip-1\n0 1\n\n\"skip-2\n0 1\n\n";
			#
			# Repeat the first line twice in the drops file because
			# often we have only one drop and xgraph won't print
			# marks for data sets with only one point.
			#
			print "\n", '"drops', "\n", @d[0..3], @d;
			# To plot acks, uncomment the following line
			# print "\n", '"acks', "\n", @a;
			$c++;
			foreach $i (@ld) {
				print "\n\"link-down\n$i 0\n$i $c\n";
			}
			foreach $i (@lu) {
				print "\n\"link-up\n$i 0\n$i $c\n";
			}
		}
	}

	set f [open temp.rands w]
	puts $f "TitleText: $file"
	puts $f "Device: Postscript"	
    exec perl -ane $perlCode out.tr >@ $f
	close $f
	if {$quiet == "false"} {
	  if {[info exists env(DISPLAY)] && ![info exists env(NOXGRAPH)]} {
	    exec xgraph -display $env(DISPLAY) -bb -tk -nl -m -x time -y packet temp.rands &
	  } else {
	    puts stderr "output trace is in temp.rands"
	  }
	}
	
	exit 0
}

#
# Arrange for tcp source stats to be dumped for $tcpSrc every
# $interval seconds of simulation time
#
TestSuite instproc tcpDump { tcpSrc interval } {
	global quiet
	$self instvar dump_inst_ ns_
	if ![info exists dump_inst_($tcpSrc)] {
		set dump_inst_($tcpSrc) 1
		$ns_ at 0.0 "$self tcpDump $tcpSrc $interval"
		return
	}
	$ns_ at [expr [$ns_ now] + $interval] "$self tcpDump $tcpSrc $interval"
	set report [$ns_ now]/cwnd=[format "%.4f" [$tcpSrc set cwnd_]]/ssthresh=[$tcpSrc set ssthresh_]/ack=[$tcpSrc set ack_]
	if {$quiet == "false"} {
		puts $report
	}
}

TestSuite instproc tcpDumpAll { tcpSrc interval label } {
	$self instvar dump_inst_ ns_
	if ![info exists dump_inst_($tcpSrc)] {
		set dump_inst_($tcpSrc) 1
		puts $label/window=[$tcpSrc set window_]/packetSize=[$tcpSrc set packetSize_]/bugFix=[$tcpSrc set bugFix_]	
		$ns_ at 0.0 "$self tcpDumpAll $tcpSrc $interval $label"
		return
	}
	$ns_ at [expr [$ns_ now] + $interval] "$self tcpDumpAll $tcpSrc $interval $label"
	puts $label/time=[$ns_ now]/cwnd=[format "%.4f" [$tcpSrc set cwnd_]]/ssthresh=[$tcpSrc set ssthresh_]/ack=[$tcpSrc set ack_]/rtt=[$tcpSrc set rtt_]	
}

TestSuite instproc openTrace { stopTime testName } {
	$self instvar ns_
	exec rm -f out.tr temp.rands
	set traceFile [open out.tr w]
	puts $traceFile "v testName $testName"
	$ns_ at $stopTime \
		"close $traceFile ; $self finish $testName"
	return $traceFile
}

TestSuite instproc traceQueues { node traceFile } {
	$self instvar ns_
	foreach nbr [$node neighbors] {
		$ns_ trace-queue $node $nbr $traceFile
		[$ns_ link $node $nbr] trace-dynamics $ns_ $traceFile
	}
}

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> \[<topologies>\]"
	puts stderr "Valid tests are:\t[get-subclasses TestSuite Test/]"
	puts stderr "Valid Topologies are:\t[get-subclasses SkelTopology Topology/]"
	exit 1
}

proc isProc? {cls prc} {
	if [catch "Object info subclass $cls/$prc" r] {
		global argv0
		puts stderr "$argv0: no such $cls: $prc"
		usage
	}
}

proc get-subclasses {cls pfx} {
	set ret ""
	set l [string length $pfx]

	set c $cls
	while {[llength $c] > 0} {
		set t [lindex $c 0]
		set c [lrange $c 1 end]
		if [string match ${pfx}* $t] {
			lappend ret [string range $t $l end]
		}
		eval lappend c [$t info subclass]
	}
	set ret
}

TestSuite proc runTest {} {
	global argc argv quiet

	set quiet false
	switch $argc {
		1 {
			set test $argv
			isProc? Test $test

			set topo ""
		}
		2 {
			set test [lindex $argv 0]
			isProc? Test $test

			set topo [lindex $argv 1]
			if {$topo == "QUIET"} {
				set quiet true
				set topo ""
			} else {
				isProc? Topology $topo
			}
		}
		3 {
			set test [lindex $argv 0]
			isProc? Test $test

			set topo [lindex $argv 1]
			isProc? Topology $topo

			set extra [lindex $argv 2]
			if {$extra == "QUIET"} {
				set quiet true
			}
		}
		default {
			usage
		}
	}
	set t [new Test/$test $topo]
	$t run
}

# Skeleton topology base class
Class SkelTopology

SkelTopology instproc init {} {
    $self next
}

SkelTopology instproc node? n {
    $self instvar node_
    if [info exists node_($n)] {
	set ret $node_($n)
    } else {
	set ret ""
    }
    set ret
}

SkelTopology instproc add-fallback-links {ns nodelist bw delay qtype args} {
   $self instvar node_
    set n1 [lindex $nodelist 0]
    foreach n2 [lrange $nodelist 1 end] {
	if ![info exists node_($n2)] {
	    set node_($n2) [$ns node]
	}
	$ns duplex-link $node_($n1) $node_($n2) $bw $delay $qtype
	foreach opt $args {
	    set cmd [lindex $opt 0]
	    set val [lindex $opt 1]
	    if {[llength $opt] > 2} {
		set x1 [lindex $opt 2]
		set x2 [lindex $opt 3]
	    } else {
		set x1 $n1
		set x2 $n2
	    }
	    $ns $cmd $node_($x1) $node_($x2) $val
	    $ns $cmd $node_($x2) $node_($x1) $val
	}
	set n1 $n2
    }
}


Class NodeTopology/4nodes -superclass SkelTopology

# Create a simple four node topology:
#
#	   s1
#	     \ 
#  8Mb,5ms \   0.8Mb,100ms
#	        r1 --------- k1
#  8Mb,5ms /
#	     /
#	   s2

NodeTopology/4nodes instproc init ns {
    $self next

    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]
}

#
# Links1 uses 8Mb, 5ms feeders, and a 800Kb 100ms bottleneck.
# Queue-limit on bottleneck is 6 packets.
#
Class Topology/net0 -superclass NodeTopology/4nodes
Topology/net0 instproc init ns {
    $self next $ns
    $self instvar node_
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 100ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 6
    $ns queue-limit $node_(k1) $node_(r1) 6
    if {[$class info instprocs config] != ""} {
	$self config $ns
    }
}

#
# Links1 uses 10Mb, 5ms feeders, and a 1.5Mb 100ms bottleneck.
# Queue-limit on bottleneck is 23 packets.
#

Class Topology/net1 -superclass NodeTopology/4nodes
Topology/net1 instproc init ns {
    $self next $ns
    $self instvar node_
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 5ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 5ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 1.5Mb 100ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 23
    $ns queue-limit $node_(k1) $node_(r1) 23
    if {[$class info instprocs config] != ""} {
	$self config $ns
    }
}

Class NodeTopology/6nodes -superclass SkelTopology

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

NodeTopology/6nodes instproc init ns {
    $self next

    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(r2) [$ns node]
    set node_(s3) [$ns node]
    set node_(s4) [$ns node]
}

Class Topology/net2 -superclass NodeTopology/6nodes
Topology/net2 instproc init ns {
    $self next $ns

    $self instvar node_
    Queue/RED set drop-rand_ true
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 25
    $ns queue-limit $node_(r2) $node_(r1) 25
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
    if {[$class info instprocs config] != ""} {
	$self config $ns
    }
}

# Definition of test-suite tests

Class Test/tahoe1 -superclass TestSuite
Test/tahoe1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	tahoe
	$self next
}
Test/tahoe1 instproc run {} {
	$self instvar ns_ node_ testName_

	# Set up TCP connection
	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 50
	
	# Set up FTP source
	set ftp1 [$tcp1 attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	#
	# Actually, we now trace all activity at the node around the
	# bottleneck link.  This allows us to track acks, as well
	# packets taking any alternate paths around the bottleneck
	# link.
	#
	$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
	$ns_ run
}

Class Test/tahoe2 -superclass TestSuite
Test/tahoe2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	tahoe2
	$self next
}
Test/tahoe2 instproc run {} {
	$self instvar ns_ node_ testName_

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 14
	
	set ftp1 [$tcp1 attach-source FTP]
	$ns_ at 1.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
	$ns_ run
}

Class Test/tahoe3 -superclass TestSuite
Test/tahoe3 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	tahoe3
	$self next
}
Test/tahoe3 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(k1) 8   
	$ns_ queue-limit $node_(k1) $node_(r1) 8   

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 100
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 16

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 0.5 "$ftp2 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 8.0 $testName_]
	$ns_ run
}

Class Test/tahoe4 -superclass TestSuite
Test/tahoe4 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	tahoe4
	$self next
}
Test/tahoe4 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ delay $node_(s2) $node_(r1) 200ms
	$ns_ delay $node_(r1) $node_(s2) 200ms
	$ns_ queue-limit $node_(r1) $node_(k1) 11
	$ns_ queue-limit $node_(k1) $node_(r1) 11  

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 30
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 30

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns_ at 0.0 "$ftp1 start"
	$ns_ at 0.0 "$ftp2 start"

	$self tcpDump $tcp1 5.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 25.0 $testName_]
	$ns_ run
}

Class Test/no_bug -superclass TestSuite
Test/no_bug instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net1
	set test_	no_bug
	$self next
}
Test/no_bug instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ delay $node_(s1) $node_(r1) 3ms
	$ns_ delay $node_(r1) $node_(s1) 3ms

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 50
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 50

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 1.75 "$ftp2 produce 100"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 6.0 $testName_]
	$ns_ run
}

Class Test/bug -superclass TestSuite
Test/bug instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net1
	set test_	bug
	$self next
}
Test/bug instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ delay $node_(s1) $node_(r1) 3ms
	$ns_ delay $node_(r1) $node_(s1) 3ms

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 50
	$tcp1 set bugFix_ false
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 50
	$tcp2 set bugFix_ false

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 1.75 "$ftp2 produce 100"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 6.0 $testName_]
	$ns_ run
}

Class Test/reno1 -superclass TestSuite
Test/reno1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	reno1
	$self next
}
Test/reno1 instproc run {} {
	$self instvar ns_ node_ testName_

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 14

	set ftp1 [$tcp1 attach-source FTP]
	$ns_ at 1.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
	$ns_ run
}

Class Test/reno -superclass TestSuite
Test/reno instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	reno
	$self next
}
Test/reno instproc run {} {
	$self instvar ns_ node_ testName_

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 28
	$tcp1 set maxcwnd_ 14

	set ftp1 [$tcp1 attach-source FTP]
	$ns_ at 1.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
	$ns_ run
}

Class Test/renoA -superclass TestSuite
Test/renoA instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	renoA
	$self next
}
Test/renoA instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(k1) 8

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 28
	set tcp2 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 1]
	$tcp2 set window_ 4
	set tcp3 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 2]
	$tcp3 set window_ 4

	set ftp1 [$tcp1 attach-source FTP]
	$ns_ at 1.0 "$ftp1 start"
	set ftp2 [$tcp2 attach-source FTP]
	$ns_ at 1.2 "$ftp2 produce 7"
	set ftp3 [$tcp3 attach-source FTP]
	$ns_ at 1.2 "$ftp3 produce 7"

	$self tcpDump $tcp1 1.0
	$self tcpDump $tcp2 1.0
	$self tcpDump $tcp3 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
	$ns_ run
}

Class Test/reno2 -superclass TestSuite
Test/reno2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	reno2
	$self next
}
Test/reno2 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(k1) 9

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 50
	set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 20

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 1.0 "$ftp2 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ run
}

Class Test/reno3 -superclass TestSuite
Test/reno3 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	reno3
	$self next
}
Test/reno3 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(k1) 8
	$ns_ queue-limit $node_(k1) $node_(r1) 8

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 100
	set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 16

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 0.5 "$ftp2 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 8.0 $testName_]
	$ns_ run
}

Class Test/reno4 -superclass TestSuite
Test/reno4 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net2
	set test_	reno4
	$self next
}
Test/reno4 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(r2) 29

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink/DelAck $node_(r2) 0]
	$tcp1 set window_ 80
	$tcp1 set maxcwnd_ 40

	set ftp1 [$tcp1 attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(s1) [$self openTrace 2.0 $testName_]
	$ns_ run
}

Class Test/reno4a -superclass TestSuite
Test/reno4a instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net2
	set test_	reno4a
	$self next
}
Test/reno4a instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(r2) 29

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink/DelAck $node_(r2) 0]
	$tcp1 set window_ 40
	$tcp1 set maxcwnd_ 40

	set ftp1 [$tcp1 attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(s1) [$self openTrace 2.0 $testName_]
	$ns_ run
}

Class Test/reno5 -superclass TestSuite
Test/reno5 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	reno5
	$self next
}
Test/reno5 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(k1) 9

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 50
	$tcp1 set bugFix_ false
	set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 20
	$tcp2 set bugFix_ false

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 1.0 "$ftp2 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ run
}

Class Test/telnet -superclass TestSuite
Test/telnet instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	telnet
	$self next
}
Test/telnet instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(k1) 8
	$ns_ queue-limit $node_(k1) $node_(r1) 8

	set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(k1) 0]
	set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(k1) 1]
	set tcp3 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(k1) 2]

	set telnet1 [$tcp1 attach-source Telnet]; $telnet1 set interval_ 1
	set telnet2 [$tcp2 attach-source Telnet]; $telnet2 set interval_ 0
	# Interval 0 designates the tcplib telnet interarrival distribution
	set telnet3 [$tcp3 attach-source Telnet]; $telnet3 set interval_ 0

	$ns_ at 0.0 "$telnet1 start"
	$ns_ at 0.0 "$telnet2 start"
	$ns_ at 0.0 "$telnet3 start"

	$self tcpDump $tcp1 5.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 50.0 $testName_]

	# use a different seed each time
	#puts seed=[$ns_ random 0]

	$ns_ run
}

Class Test/delayed -superclass TestSuite
Test/delayed instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	delayed
	$self next
}
Test/delayed instproc run {} {
	$self instvar ns_ node_ testName_

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink/DelAck $node_(k1) 0]
	$tcp1 set window_ 50

	# lookup up the sink and set it's delay interval
	[$node_(k1) agent [$tcp1 dst-port]] set interval 100ms

	set ftp1 [$tcp1 attach-source FTP];
	$ns_ at 1.0 "$ftp1 start"

	$self tcpDump $tcp1 1.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 4.0 $testName_]
	$ns_ run
}

Class Test/phase -superclass TestSuite
Test/phase instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	phase
	$self next
}
Test/phase instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ delay $node_(s2) $node_(r1) 3ms
	$ns_ delay $node_(r1) $node_(s2) 3ms
	$ns_ queue-limit $node_(r1) $node_(k1) 16
	$ns_ queue-limit $node_(k1) $node_(r1) 100

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 32 
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 32 

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns_ at 5.0 "$ftp1 start"
	$ns_ at 1.0 "$ftp2 start"

	$self tcpDump $tcp1 5.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 25.0 $testName_]
	$ns_ run
}

Class Test/phase1 -superclass TestSuite
Test/phase1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	phase1
	$self next
}
Test/phase1 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ delay $node_(s2) $node_(r1) 9.5ms
	$ns_ delay $node_(r1) $node_(s2) 9.5ms
	$ns_ queue-limit $node_(r1) $node_(k1) 16
	$ns_ queue-limit $node_(k1) $node_(r1) 100

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 32 
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 32 

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns_ at 5.0 "$ftp1 start"
	$ns_ at 1.0 "$ftp2 start"

	$self tcpDump $tcp1 5.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 25.0 $testName_]
	$ns_ run
}

Class Test/phase2 -superclass TestSuite
Test/phase2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	phase2
	$self next
}
Test/phase2 instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ delay $node_(s2) $node_(r1) 3ms
	$ns_ delay $node_(r1) $node_(s2) 3ms
	$ns_ queue-limit $node_(r1) $node_(k1) 16
	$ns_ queue-limit $node_(k1) $node_(r1) 100

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 32 
	$tcp1 set overhead_ 0.01
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 32 
	$tcp2 set overhead_ 0.01

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns_ at 5.0 "$ftp1 start"
	$ns_ at 1.0 "$ftp2 start"

	$self tcpDump $tcp1 5.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 25.0 $testName_]
	$ns_ run
}

Class Test/timers -superclass TestSuite
Test/timers instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	timers
	$self next
}
Test/timers instproc run {} {
	$self instvar ns_ node_ testName_

	$ns_ queue-limit $node_(r1) $node_(k1) 2
	$ns_ queue-limit $node_(k1) $node_(r1) 100

	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink/DelAck $node_(k1) 0]
	$tcp1 set window_ 4
	# look up the sink and set its delay interval
	[$node_(k1) agent [$tcp1 dst-port]] set interval_ 100ms
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink/DelAck $node_(k1) 1]
	$tcp2 set window_ 4
	# look up the sink and set its delay interval
	[$node_(k1) agent [$tcp2 dst-port]] set interval_ 100ms

	set ftp1 [$tcp1 attach-source FTP]
	set ftp2 [$tcp2 attach-source FTP]

	$ns_ at 1.0 "$ftp1 start"
	$ns_ at 1.3225 "$ftp2 start"

	$self tcpDump $tcp1 5.0

	# Trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ run
}

Class Test/stats -superclass TestSuite
Test/stats instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net0
	set test_	stats
	$self next
}
Test/stats instproc printpkts { label tcp } {
	puts "tcp $label total_packets_acked [$tcp set ack_]"
}
#XXX Still unfinished in ns-2
Test/stats instproc printdrops { fid fmon } {
	set fcl [$fmon classifier]; # flow classifier
	#
	# look up the flow using the classifier.  Because we are
	# using a Fid classifier, the src/dst fields are not compared,
	# and can thus be just zero, as illustrated here.  The "auto"
	# indicates we don't already know which bucket in the classifier's
	# hash table to find the flow we're looking for.
	#
	set flow [$fcl lookup auto 0 0 $fid]
	puts "fid: $fid per-link total_drops [$flow set pdrops_]"
	puts "fid: $fid per-link total_packets [$flow set pdepartures_]"
	puts "fid: $fid per-link total_bytes [$flow set bdepartures_]"
	#
	# note there is much more date available in $flow and $fmon
	# that isn't being printed here.
	#
}
Test/stats instproc printstop { stoptime } {
	puts "stop-time $stoptime"
}
Test/stats instproc run {} {
	$self instvar ns_ node_ testName_ 

	$ns_ delay $node_(s2) $node_(r1) 200ms
	$ns_ delay $node_(r1) $node_(s2) 200ms
	$ns_ queue-limit $node_(r1) $node_(k1) 10
	$ns_ queue-limit $node_(k1) $node_(r1) 10

	set slink [$ns_ link $node_(r1) $node_(k1)]; # link to collect stats on
	set fmon [$ns_ makeflowmon Fid]
	$ns_ attach-fmon $slink $fmon

	set stoptime 10.1 

	set tcp0 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp0 set window_ 30
	set tcp1 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp1 set window_ 30

	set ftp0 [$tcp0 attach-source FTP]
	set ftp1 [$tcp1 attach-source FTP]

	$ns_ at 1.0 "$ftp0 start"
	$ns_ at 1.0 "$ftp1 start"

	$self tcpDumpAll $tcp0 5.0 tcp0
	$self tcpDumpAll $tcp1 5.00001 tcp1

	set almosttime [expr $stoptime - 0.001]
	$ns_ at $almosttime "$self printpkts 0 $tcp0"
	$ns_ at $almosttime "$self printpkts 1 $tcp1"
	$ns_ at $stoptime "$self printdrops 0 $fmon"
	$ns_ at $stoptime "$self printdrops 1 $fmon"

	# trace only the bottleneck link
	$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
	$ns_ run
}

TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:

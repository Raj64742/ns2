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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-tcp.tcl,v 1.2 1997/10/02 01:44:04 sfloyd Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-tcl.tcl
#

Class TestSuite

TestSuite instproc init {} {
	$self instvar ns_ net_ defNet_ test_ topo_ node_ testName_
	set ns_ [new Simulator]
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
		set node_($i) [$topo_ node? $i]
	}

	if {$net_ == $defNet_} {
		set testName_ "$test_"
	} else {
		set testName_ "$test_:$net_"
	}

        if [info exists node_(k1)] {
                set blink [$ns_ link $node_(r1) $node_(k1)]
        } else {
                set blink [$ns_ link $node_(r1) $node_(r2)]
        }
        $blink trace-dynamics $ns_ stdout
}

TestSuite instproc finish file {
	global env

	#
	# we don't bother checking for the link we're interested in
	# since we know only such events are in our trace file
	#
	set perlCode {
		sub BEGIN { $c = 0; @p = @a = @d = @lu = @ld = (); }
		/^[\+-] / && do {
			if ($F[4] eq 'tcp') {
 				push(@p, $F[1], ' ',		\
					$F[8] + ($F[10] % 90) * 0.01, "\n");
			} elsif ($F[4] eq 'ack') {
 				push(@a, $F[1], ' ',		\
					$F[8] + ($F[10] % 90) * 0.01, "\n");
			}
			$c = $F[8] if ($c < $F[8]);
			next;
		};
		/^d / && do {
			push(@d, $F[1], ' ',		\
					$F[8] + ($F[10] % 90) * 0.01, "\n");
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
	if {[info exists env(DISPLAY)] && ![info exists env(NOXGRAPH)]} {
	    exec xgraph -display $env(DISPLAY) -bb -tk -nl -m -x time -y packet temp.rands &
	} else {
	    puts stderr "output trace is in temp.rands"
	}
	
	exit 0
}

#
# Arrange for tcp source stats to be dumped for $tcpSrc every
# $interval seconds of simulation time
#
TestSuite instproc tcpDump { tcpSrc interval } {
	$self instvar dump_inst_ ns_
	if ![info exists dump_inst_($tcpSrc)] {
		set dump_inst_($tcpSrc) 1
		$ns_ at 0.0 "$self tcpDump $tcpSrc $interval"
		return
	}
	$ns_ at [expr [$ns_ now] + $interval] "$self tcpDump $tcpSrc $interval"
	puts [$ns_ now]/cwnd=[format "%.4f" [$tcpSrc set cwnd_]]/ssthresh=[$tcpSrc set ssthresh_]/ack=[$tcpSrc set ack_]
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
	global argc argv

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
			isProc? Topology $topo
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
Class Topology/net4 -superclass NodeTopology/4nodes
Topology/net4 instproc init ns {
    $self next $ns
    $self instvar node_
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 10ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 2
    $ns queue-limit $node_(k1) $node_(r1) 2
    if {[$class info instprocs config] != ""} {
	$self config $ns
    }
}

#
# Links1 uses 8Mb, 5ms feeders, and a 800Kb 100ms bottleneck.
# Queue-limit on bottleneck is 6 packets.
#
Class Topology/net3 -superclass NodeTopology/4nodes
Topology/net3 instproc init ns {
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

# Definition of test-suite tests

# This test shows two TCPs when one is ECN-capable and the other
# is not.

Class Test/ecn -superclass TestSuite
Test/ecn instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net3
	set test_	ecn_(one_with_ecn,_one_without)
	$self next
}
Test/ecn instproc run {} {
	$self instvar ns_ node_ testName_

	# Set up TCP connection
	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 30
	$tcp1 set ecn_ 1
	set ftp1 [$tcp1 attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"

	# Set up TCP connection
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 0]
	$tcp2 set window_ 20
	$tcp2 set ecn_ 0
	set ftp2 [$tcp2 attach-source FTP]
	$ns_ at 3.0 "$ftp2 start"

	$self tcpDump $tcp1 5.0
	$self tcpDump $tcp2 5.0

	$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ run
}

# This test shows the retransmit timeout value when the first packet
# of a connection is dropped, and the backoff of the retransmit timer
# as subsequent packets are dropped.
# (It looks like the retransmit timer is not backed off after the
# first drop.)
#
# This test also shows that once the retransmit timer is backed off,
# it is later un-backed.

Class Test/timers -superclass TestSuite
Test/timers instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	timers_(first_packet_dropped)
	$self next
}
Test/timers instproc run {} {
	$self instvar ns_ node_ testName_

	# Set up TCP connection
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 0]
	$tcp2 set window_ 3
	set ftp2 [$tcp2 attach-source FTP]
	$ns_ at 0.09 "$ftp2 start"

	# Set up TCP connection
	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 5
	set ftp1 [$tcp1 attach-source FTP]
	$ns_ at 0.0 "$ftp1 produce 1600"
	$ns_ at 25.3 "$ftp1 producemore 5"
	$ns_ at 25.7 "$ftp1 producemore 5" 
	$ns_ at 26.1 "$ftp1 producemore 5" 
	$ns_ at 26.5 "$ftp1 producemore 5" 
	$ns_ at 26.9 "$ftp1 producemore 5" 
	$ns_ at 28.8 "$ftp1 producemore 5" 


	$self tcpDump $tcp1 5.0

	$self traceQueues $node_(r1) [$self openTrace 30.0 $testName_]
	$ns_ run
}

#	Agent/TCP set tcpTick_ 0.5

TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:

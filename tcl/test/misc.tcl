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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/misc.tcl,v 1.3 1997/06/03 21:33:58 kannan Exp $
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

if [file exists redefines.tcl] {
	puts "sourcing redefines.tcl in [pwd]"
	source redefines.tcl
}

Class TestSuite

TestSuite instproc init {} {
	$self instvar ns_ net_ defNet_ test_ topo_ node_ testName_
	if [catch "$self get-simulator" ns_] {
	    set ns_ [new Simulator]
	}
	$ns_ trace-all [open all.tr w]
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
#	global env
#
#	THIS CODE IS NOW SUPERSEDED BY THE NEWER EXTERNAL DRIVERS,
#	raw2xg, and raw2gp, in ~ns/bin.  raw2xg generates output suitable
#	for xgraph, and raw2gp, that suitable for gnuplot.
#
#	#
#	# we don't bother checking for the link we're interested in
#	# since we know only such events are in our trace file
#	#
#	set perlCode {
#		sub BEGIN { $c = 0; @p = @d = @lu = @ld = (); }
#		/^[\+-] / && do {
#			push(@p, $F[1], ' ',		\
#					$F[7] + ($F[10] % 90) * 0.01, "\n") \
#				if ($F[4] eq 'tcp' || $F[4] eq 'ack');
#			$c = $F[7] if ($c < $F[7]);
#			next;
#		};
#		/^d / && do {
#			push(@d, $F[1], ' ',		\
#					$F[7] + ($F[10] % 90) * 0.01, "\n");
#			next;
#		};
#		/link-down/ && push(@ld, $F[1]);
#		/link-up/ && push(@lu, $F[1]);
#		sub END {
#			print "\"packets\n", @p, "\n";
#			# insert dummy data sets
#			# so we get X's for marks in data-set 4
#			print "\"skip-1\n0 1\n\n\"skip-2\n0 1\n\n";
#			#
#			# Repeat the first line twice in the drops file because
#			# often we have only one drop and xgraph won't print
#			# marks for data sets with only one point.
#			#
#			print "\n", '"drops', "\n", @d[0..3], @d;
#			$c++;
#			foreach $i (@ld) {
#				print "\n\"link-down\n$i 0\n$i $c\n";
#			}
#			foreach $i (@lu) {
#				print "\n\"link-up\n$i 0\n$i $c\n";
#			}
#		}
#	}
#
#	set f [open temp.rands w]
#	puts $f "TitleText: $file"
#	puts $f "Device: Postscript"
#	exec perl -ane $perlCode out.tr >@ $f
#	close $f
#	if [info exists env(DISPLAY)] {
#	    exec xgraph -display $env(DISPLAY) -bb -tk -nl -m -x time -y packet temp.rands &
#	} else {
#	    puts stderr "output trace is in temp.rands"
#	}
#	
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

	
### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:

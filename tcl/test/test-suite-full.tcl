#
# Copyright (c) 1997 The Regents of the University of California.
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
# this is still somewhat experimental,
# and should be considered of 'alpha-test'+ quality  (kfall@ee.lbl.gov)
#
# This file tests "fulltcp", a version of tcp reno implemented in the 
# simulator based on the BSD Net/3 code. 
#
# This test suite is based on test-suite-fulltcp.tcl from ns-1.


set dir [pwd]
catch "cd tcl/test"
source misc.tcl
source topologies.tcl
catch "cd $dir"

Trace set show_tcphdr_ 1 ; # needed to plot ack numbers for tracing 

TestSuite instproc finish testname {
	global env
	$self instvar ns_

	$ns_ halt

	set fname [pid]
	exec tclsh ../../bin/tcpfull-summarize.tcl out.tr $fname

	set outtype text
	if { [info exists env(NSOUT)] } {
		set outtype $env(NSOUT)
	} elseif { [info exists env(DISPLAY)] } {
		set outtype xgraph
	}

	if { $outtype != "text" } {
		if { $outtype == "gnuplot" } {
			# writing .gnuplot is really gross, but if we
			# don't do that it's tough to get gnuplot to
			# display our graph and hang around for more user input
			exec tclsh ../../bin/cplot.tcl $outtype $testname \
			  $fname.p "segments" \
			  $fname.acks "acks w/data" \
			  $fname.packs "pure acks" $fname.d "drops" > .gnuplot
			exec xterm -T "Gnuplot: $testname" -e gnuplot &
			exec sleep 1
			exec rm -f .gnuplot
		} else {
	  
			exec tclsh ../../bin/cplot.tcl $outtype $testname \
			  $fname.p "segments" \
			  $fname.acks "acks w/data" \
			  $fname.packs "pure acks" $fname.d "drops" | $outtype &
		}
		exec sleep 1
		exec rm -f $fname.p $fname.acks $fname.packs $fname.d
	} else {
		puts "output files are $fname.{p,packs,acks,d}"
	}
}

TestSuite instproc bsdcompat tcp {
	$tcp set segsperack_ 2
	$tcp set dupseg_fix_ false
	$tcp set dupack_reset_ true
	$tcp set bugFix_ false
	$tcp set data_on_syn_ false
	$tcp set tcpTick_ 0.5
}

# Definition of test-suite tests
Class Test/full -superclass TestSuite
Test/full instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ full
	$self next
}
Test/full instproc run {} {
	$self instvar ns_ node_ testName_

	set stopt 6.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$src set dst_ [$sink set addr_]
	$sink listen ; # will figure out who its peer is
	$src set window_ 100
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"

	# forward
	$self instvar direction_
	set direction_ forward
	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]  
	$ns_ run
}

Class Test/close -superclass TestSuite
Test/close instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ close
	$self next
}
Test/close instproc run {} {
	$self instvar ns_ node_ testName_

	set stopt 6.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$src set dst_ [$sink set addr_]
	$sink listen
	$src set window_ 100
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.0 "$ftp1 produce 50"
	$ns_ at 5.5 "$src close"

	# forward
	$self instvar direction_
	set direction_ forward

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/twoway -superclass TestSuite
Test/twoway instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ twoway
	$self next
}
Test/twoway instproc run {} {
	$self instvar ns_ node_ testName_

	set stopt 6.0	
	set startt 3.0

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$src set dst_ [$sink set addr_]
	$sink listen
	$sink set iss_ 2144
	$src set window_ 100
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"
	set ftp2 [$sink attach-source FTP]
	$ns_ at $startt "$ftp2 start"

	#forward
	$self instvar direction_
	set direction_ forward

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/twoway0 -superclass TestSuite
Test/twoway0 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ twoway0
	$self next
}
Test/twoway0 instproc run {} {
	$self instvar ns_ node_ testName_

	set stopt 6.0	
	set startt 0.0

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$src set dst_ [$sink set addr_]
	$sink listen
	$sink set iss_ 2144
	$src set window_ 100
	$sink set window_ 100
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"
	set ftp2 [$sink attach-source FTP]
	$ns_ at $startt "$ftp2 start"

	#bidirectional
	$self instvar direction_
	set direction_ bidirectional

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/twoway1 -superclass TestSuite
Test/twoway1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ twoway1
	$self next
}
Test/twoway1 instproc run {} {
	$self instvar ns_ node_ testName_

	set stopt 6.0	
	set startt 1.9

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$src set dst_ [$sink set addr_]
	$sink listen
	$sink set iss_ 2144
	$src set window_ 100
	$sink set window_ 100
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"
	set ftp2 [$sink attach-source FTP]
	$ns_ at $startt "$ftp2 start"

	#bidirectional
	$self instvar direction_
	set direction_ bidirectional

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/twoway_bsdcompat -superclass TestSuite
Test/twoway_bsdcompat instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ twoway_bsdcompat
	$self next
}
Test/twoway_bsdcompat instproc run {} {
	$self instvar ns_ node_ testName_

	set stopt 6.0	
	set startt 1.9

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$src set dst_ [$sink set addr_]
	$sink listen
	$src set window_ 100
	$sink set window_ 100
	$self bsdcompat $src
	$self bsdcompat $sink
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"
	set ftp2 [$sink attach-source FTP]
	$ns_ at $startt "$ftp2 start"

	#bidirectional
	    $self instvar direction_
        set direction_ bidirectional

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/oneway_bsdcompat -superclass TestSuite
Test/oneway_bsdcompat instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ oneway_bsdcompat
	$self next
}
Test/oneway_bsdcompat instproc run {} {
	$self instvar ns_ node_ testName_

	set stopt 6.0	
 	set startt 1.9

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$src set dst_ [$sink set addr_]
	$sink listen ; # will figure out who its peer is
	$src set window_ 100
	$sink set window_ 100
	$self bsdcompat $src
	$self bsdcompat $sink
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"

	#forward
	$self instvar direction_
	set direction_ forward

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/twowayrandom -superclass TestSuite
Test/twowayrandom instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ twowayrandom
	$self next
}
Test/twowayrandom instproc run {} {
	$self instvar ns_ node_ testName_

	set stopt 6.0	
	set startt [expr [ns-random 0] % 6]
	puts "second TCP starting at time $startt"

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$src set dst_ [$sink set addr_]
	$sink listen
	$sink set iss_ 2144
	$src set window_ 100
	$sink set window_ 100
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"
	set ftp2 [$sink attach-source FTP]
	$ns_ at $startt "$ftp2 start"

	# forward
	$self instvar direction_
	set direction_ forward

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/delack -superclass TestSuite
Test/delack instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ delack
	$self next
}
Test/delack instproc run {} {
	$self instvar ns_ node_ testName_

	set stopt 6.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$src set dst_ [$sink set addr_]
	$sink listen
	$src set window_ 100
	$sink set segsperack_ 2
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"

	#forward
	$self instvar direction_
	set direction_ forward

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

TestSuite runTest

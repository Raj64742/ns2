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
	set tmpnam /tmp/$fname
	exec ../../bin/getrc -s 2 -f 0 out.tr > $tmpnam
	exec tclsh ../../bin/tcpfull-summarize.tcl $tmpnam $fname
	exec tclsh ../../bin/tcpfull-summarize.tcl $tmpnam $fname.r reverse
	exec rm -f $tmpnam

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
			exec tclsh ../../bin/cplot.tcl $outtype $testname.forw \
			  $fname.p "segments" \
			  $fname.acks "acks w/data" \
			  $fname.packs "pure acks" $fname.d "drops" \
			  $fname.es "zero-len segments" \
			  $fname.ctrl "SYN or FIN" > .gnuplot
			exec xterm -T "Gnuplot: $testname" -e gnuplot &
			exec sleep 1
			exec rm -f .gnuplot
		} else {
	  
			exec tclsh ../../bin/cplot.tcl $outtype $testname.forw \
			  $fname.p "segments" \
			  $fname.acks "acks w/data" \
			  $fname.packs "pure acks" $fname.d "drops" \
			  $fname.es "zero-len segments" \
			  $fname.ctrl "SYN or FIN" | $outtype &

			exec tclsh ../../bin/cplot.tcl $outtype $testname.rev \
			  $fname.r.p "segments" \
			  $fname.r.acks "acks w/data" \
			  $fname.r.packs "pure acks" $fname.r.d "drops" \
			  $fname.r.es "zero-len segments" \
			  $fname.r.ctrl "SYN or FIN" | $outtype &
		}
		exec sleep 1
#		exec rm -f \
#			$fname.p $fname.acks $fname.packs $fname.d $fname.ctrl $fname.es
#		exec rm -f \
#			$fname.r.p $fname.r.acks $fname.r.packs $fname.r.d $fname.r.ctrl $fname.r.es
	} else {
		puts "output files are $fname.{p,packs,acks,d,ctrl,es}"
		puts "  and $fname.r.{p,packs,acks,d,ctrl,es}"
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

Class Test/iw=4 -superclass TestSuite
Test/iw=4 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ iw=4
	$self next
}
Test/iw=4 instproc run {} {
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
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.0 "$ftp1 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set windowInit_ 4
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/droppedsyn -superclass TestSuite
Test/droppedsyn instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ droppedsyn
	$self next
}
Test/droppedsyn instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 20.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "droppedsyn: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 1.0

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
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.7 "$ftp1 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/droppedfirstseg -superclass TestSuite
Test/droppedfirstseg instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ droppedfirstseg
	$self next
}
Test/droppedfirstseg instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 20.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "droppedfirstseg: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 3.0

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
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.7 "$ftp1 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/droppedsecondseg -superclass TestSuite
Test/droppedsecondseg instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ droppedsecondseg
	$self next
}
Test/droppedsecondseg instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 20.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "droppedsecondseg: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 4.0

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
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.7 "$ftp1 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/simul-open -superclass TestSuite
Test/simul-open instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ simul-open
	$self next
}
Test/simul-open instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 9.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "simul-open: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 20.0

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink; # this is bi-directional

	# set up TCP-level connections
	$src set dst_ [$sink set addr_]
	set ftp1 [$src attach-source FTP]
	set ftp2 [$sink attach-source FTP]
	$ns_ at 0.7 "$ftp1 start; $ftp2 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$sink set window_ 100
	$sink set delay_growth_ true
	$sink set tcpTick_ 0.500
	$sink set packetSize_ 1460

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/simul-close -superclass TestSuite
Test/simul-close instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ simul-close-maybe-buggy
	$self next
}
Test/simul-close instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 9.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink; # this is bi-directional

	# set up TCP-level connections
	$src set dst_ [$sink set addr_]
	set ftp1 [$src attach-source FTP]
	set ftp2 [$sink attach-source FTP]
	$ns_ at 0.7 "$ftp1 start"
	$ns_ at 0.9 "$ftp2 start"

	$ns_ at 1.6 "$src close; $sink close"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$sink set window_ 100
	$sink set delay_growth_ true
	$sink set tcpTick_ 0.500
	$sink set packetSize_ 1460

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/droppednearfin -superclass TestSuite
Test/droppednearfin instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ droppednearfin
	$self next
}
Test/droppednearfin instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "droppednearfin: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 7.0
	$errmodel set period_ 100.0

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
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.7 "$ftp1 start"
	$ns_ at 1.5 "$src close"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/droppedlastseg -superclass TestSuite
Test/droppedlastseg instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ droppedlastseg
	$self next
}
Test/droppedlastseg instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "droppedlastseg: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 9.0
	$errmodel set period_ 100.0

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
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.7 "$ftp1 start"
	$ns_ at 1.5 "$src close"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/droppedfin -superclass TestSuite
Test/droppedfin instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ droppedfin
	$self next
}
Test/droppedfin instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "droppedfin: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 9.0
	$errmodel set period_ 100.0

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
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.7 "$ftp1 start"
	$ns_ at 1.5 "$src close"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/smallpkts -superclass TestSuite
Test/smallpkts instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ smallpkts
	$self next
}
Test/smallpkts instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$sink set interval_ 0.2
	$ns_ connect $src $sink

	# set up TCP-level connections
	$src set dst_ [$sink set addr_]
	$sink listen
	set ftp1 [$src attach-source FTP]
	$ns_ at 0.5 "$src advance-bytes 30"
	$ns_ at 0.75 "$src advance-bytes 300"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#
# this test sets the receiver's notion of the mss larger than
# the sender and sets segsperack to 3.  So, only if there is
# > 3*4192 bytes accumulated would an ACK occur [i.e. never].
# So, because the
# delack timer is set for 200ms, the upshot here is that
# we see ACKs as pushed out by this timer only
#
Class Test/telnet -superclass TestSuite
Test/telnet instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ telnet(200ms-delack)
	$self next
}
Test/telnet instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$sink set interval_ 0.2; # generate acks/adverts each 200ms
	$sink set segsize_ 4192; # or wait up to 3*4192 bytes to ACK
	$sink set segsperack_ 3
	$ns_ connect $src $sink

	# set up TCP-level connections
	$src set dst_ [$sink set addr_]
	$sink listen
	set telnet1 [$src attach-source Telnet]
	$telnet1 set interval_ 0
	$ns_ at 0.5 "$telnet1 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#
# this test is the same as the last one, but changes the
# receiver's notion of the mss, delack interval, and segs-per-ack.
# The output indicates some places where ACKs are generated due
# to the timer and other are due to meeting the segs-per-ack limit
# before the timer
#
Class Test/telnet2 -superclass TestSuite
Test/telnet2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ telnet2(3segperack-600ms-delack)
	$self next
}
Test/telnet2 instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$sink set interval_ 0.6; # generate acks/adverts each 600ms
	$sink set segsize_ 536; # or wait up to 3*536 bytes to ACK
	$sink set segsperack_ 3
	$ns_ connect $src $sink

	# set up TCP-level connections
	$src set dst_ [$sink set addr_]
	$sink listen
	set telnet1 [$src attach-source Telnet]
	$telnet1 set interval_ 0
	$ns_ at 0.5 "$telnet1 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set segsize_ 536
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

TestSuite runTest

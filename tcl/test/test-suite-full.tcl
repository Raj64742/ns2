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
# this is still somewhat experimental,
# and should be considered of 'alpha-test' quality  (kfall@ee.lbl.gov)
#
# This file tests "fulltcp", a version of tcp reno implemented in the 
# simulator based on the BSD Net/3 code. 
#
# This test suite is based on test-suite-fulltcp.tcl from ns-1.

# The following line is needed to plot ack numbers for tracing 
Trace set show_tcphdr_ 1

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

TestSuite instproc finish_full {testname direction} {
	$self instvar ns_
	global env exit_scheduled

	set now [$ns_ now]
    if { ![info exists exit_scheduled] } {
        $ns_ at [expr $now + 0.1] "exit 0"
        set exit_scheduled true
    }
    
    set fname [ns-random 0]
    set ackstmp $fname.acks ; # data-full acks
    set packstmp $fname.packs; # pure acks
    set segstmp $fname.p
    set dropstmp $fname.d
    set grtmp $fname.xgr

    exec rm -f $grtmp $ackstmp $segstmp $dropstmp $packstmp
    set f [open $grtmp w]
    puts $f "TitleText: $testname"
    puts $f "Device: Postscript"

    exec touch $ackstmp $segstmp $dropstmp

	# search for SYN packet in "out.tr", and store source and dest IDs	
	# the forward path is defined by $direction 
	set str [exec awk { /0x2/ {print $9, $10; exit} } out.tr ]
	if {$direction=="forward"} {
		set source_ [lindex $str 0]
		set dest_ [lindex $str 1]
	} elseif {$direction=="reverse"} {
		set dest_ [lindex $str 0]
		set source_ [lindex $str 1]
	}
	
    #
    # $5: packet type
    # $6: packet length
    # $11: seq number
    # $13: ack number
    # $15: TCP header length
    #
	# XXX this is inefficient, scanning multiple times
    exec awk -v source_=$source_ -v dest_=$dest_ { 
        {
            if ($1 == "+" && ($5 == "tcp" || $5 == "ack") \
                && $6 != $15 && $9 == source_ && $10 == dest_)
                print $2, $11 + $6 - $15 
        }
    } out.tr > $segstmp
    # this part uses the "ackno" field of the tcp header (for fulltcp)
    #   ACKs with data
    exec awk -v source_=$source_ -v dest_=$dest_ { 
        {
            if ($1 == "+" && ($5 == "tcp" || $5 == "ack") \
                && $6 != $15 && $9 == dest_ && $10 == source_ )
                print $2, $13
        }
    } out.tr > $ackstmp
    #   pure ACKS
    exec awk -v source_=$source_ -v dest_=$dest_ { 
        {
            if ($1 == "+" && ($5 == "tcp" || $5 == "ack") \
                && $6 == $15 && $9 == dest_ && $10 == source_ )
                print $2, $13
        }
    } out.tr > $packstmp 
    exec awk -v source_=$source_ -v dest_=$dest_ {
        {
			if ($1 == "d" && $9 == source_ && $10 == dest_)
				print $2, $11 + $6 - $15
        }
    } out.tr > $dropstmp

    #
    # build seqno data set
    #
    puts $f \"seqno
    exec cat $segstmp >@ $f
    
    #
    # build acks dataset
    #
    puts $f [format "\n\"acks w/data"]
    exec cat $ackstmp >@ $f
    
    puts $f [format "\n\"pure acks"]
    exec cat $packstmp >@ $f
    
    #
    # build drops dataset
    #   
    puts $f [format "\n\"drops\n"]
    #
    # Repeat the first line twice in the drops file because
    # often we have only one drop and xgraph won't print marks
    # for data sets with only one point.
    # 
    exec head -1 $dropstmp >@ $f
    exec cat $dropstmp >@ $f
    flush $f
    close $f
	if [info exists env(DISPLAY)] {
    	exec xgraph -display $env(DISPLAY) -bb -nl -m -x time -y seqno $grtmp &
	} else {
	    puts stderr "output trace is in temp.rands"
	}
    exec /bin/sleep 1 
    exec rm -f $grtmp $ackstmp $segstmp $dropstmp $packstmp
	
}

TestSuite instproc openTrace { stopTime testName direction} {
	$self instvar ns_
	exec rm -f out.tr temp.rands
	set traceFile [open out.tr w]
	puts $traceFile "v testName $testName"
	$ns_ at $stopTime "close $traceFile; $self finish $testName $direction"  
	return $traceFile
}

TestSuite instproc finish {testName direction} {
		if {$direction=="forward"} {
			$self finish_full ${testName}_forward $direction
		} elseif {$direction=="reverse"} {
			$self finish_full ${testName}_reverse $direction
		} elseif {$direction=="bidirectional"} {
			$self finish_full ${testName}_forward forward
			$self finish_full ${testName}_reverse reverse
		} else {
			puts "Incorrect direction $direction specified"
		}
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
#  8Mb,10ms \   0.8Mb,90ms
#	        r1 --------- k1
#  8Mb,10ms /
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
# Note:  This is the only topology used in the ns-1 test suite.
# It differs slightly from the basic test-suite.tcl topology
# 
Class Topology/net0 -superclass NodeTopology/4nodes
Topology/net0 instproc init ns {
    $self next $ns
    $self instvar node_
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 10ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 10ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 90ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 16
    $ns queue-limit $node_(k1) $node_(r1) 16
    if {[$class info instprocs config] != ""} {
	$self config $ns
    }
}

# More topologies could be added here

TestSuite instproc bsdcompat tcp {
	$tcp set segsperack_ 2
	$tcp set dupseg_fix_ false
	$tcp set dupack_reset_ true
	$tcp set bugFix_ false
	$tcp set data_on_syn_ false
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

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_ forward]  
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

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_ forward]
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

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_ forward]
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

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_ bidirectional]
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

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_ bidirectional]
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

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_ bidirectional]
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

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_ forward]
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

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_ forward]
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

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_ forward]
    $ns_ run
}

# More tests could be added here

TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:

#
# Main test file for the router mechanisms simulation
#
Class TestSuite
source mechanisms.tcl
source sources.tcl


TestSuite instproc init {} {
        $self instvar ns_ defNet_ net_ test_ topo_ node_ testName_

        set ns_ [new Simulator]
        if {$net_ == ""} {
                set net_ $defNet_
        }
        if ![Topology/$defNet_ info subclass Topology/$net_] {
                global argv0
                puts stderr "$argv0: cannot run test $test_ over topology $net_"
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
}

TestSuite instproc finish file {
        global env
	$self instvar graphfile_ flowfile_
        set perlCode {
                sub BEGIN { $c = 0; @p = @a = @d = @lu = @ld = (); }
                /^[\+-] / && do {
                        if ($F[4] eq 'tcp') {
                                push(@p, $F[1], ' ',            \
                                        $F[7] + ($F[10] % 90) * 0.01, "\n");
                        } elsif ($F[4] eq 'ack') {
                                push(@a, $F[1], ' ',            \
                                        $F[7] + ($F[10] % 90) * 0.01, "\n");
                        }
                        $c = $F[7] if ($c < $F[7]);
                        next;
                };
                /^d / && do {
                        push(@d, $F[1], ' ',            \
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
        set f [open $graphfile_ w]
        puts $f "TitleText: $file"
        puts $f "Device: Postscript"
        exec perl -ane $perlCode $flowfile_ >@ $f
        close $f
        if [info exists env(DISPLAY)] {
            exec xgraph -display $env(DISPLAY) -bb -tk -nl -m -x time -y packet $graphfile_ &
        } else {
            puts "output trace is in temp.rands"
        }
        exit 0
}

#------------------------------------------------------------------
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

#
# create:
#
# S1		 S3
#   \		 /
#    \		/
#    R1========R2
#    /		\
#   /		 \
# S2		 S4
#
# - 10Mb/s, 3ms, drop-tail
# = 1.5Mb/s, 20ms, CBQ
#
Class NodeTopology/6nodes -superclass SkelTopology
NodeTopology/6nodes instproc init ns {
	$self next
	$self instvar node_

	set node_(s1) [$ns node]
	set node_(s2) [$ns node]
	set node_(s3) [$ns node]
	set node_(s4) [$ns node]

	set node_(r1) [$ns node]
	set node_(r2) [$ns node]
}

Class Topology/net2 -superclass NodeTopology/6nodes
Topology/net2 instproc init ns {
	$self next $ns
	$self instvar node_ cbqlink_ bandwidth_

	$ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
	$ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
	set cl [new Classifier/Hash/SrcDestFid 33]
	$ns simplex-link $node_(r1) $node_(r2) 1.5Mb 20ms "CBQ $cl"
	set cbqlink_ [$ns link $node_(r1) $node_(r2)]
	[$cbqlink_ queue] algorithm "formal"
	$ns simplex-link $node_(r2) $node_(r1) 1.5Mb 20ms DropTail
	set bandwidth_ 1500
	[[$ns link $node_(r2) $node_(r1)] queue] set limit_ 25
	$ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
	$ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail

	return $cbqlink_
}

#
# prints "time: $time class: $class bytes: $bytes" for the link.
#
TestSuite instproc linkDumpFlows { link interval stoptime } {
	$self instvar ns_ flowfile_
        set f [open $flowfile_ w]
        TestSuite instproc dump1 { file link interval } {
		$self instvar ns_ maxfid_
                $ns_ at [expr [$ns_ now] + $interval] \
			"$self dump1 $file $link $interval"
                for {set i 0} {$i <= $maxfid_} {incr i 1} {
  set bytes [$link stat $i bytes] ; # NOT IN NS2!!
                  if {$bytes > 0} {
                  	puts $file \
			    "time: [$ns_ now] class: $i bytes: $bytes $interval"     
                  }
                }
        }
        $ns_ at $interval "$self dump1 $f $link $interval"
        $ns_ at $stoptime "close $f"
}

Class Test/one -superclass TestSuite
Test/one instproc init topo {
        $self instvar net_ defNet_ test_
        set net_        $topo   
        set defNet_     net2
        set test_       reclass2
        $self next
	$self config
}

Test/one instproc config {} {
	$self instvar flowfile_ graphfile_ penaltyfile_ flowgraphfile_
	$self instvar goodflowfile_ badflowfile_

	set goodflowfile_ gflow.tr
	set badflowfile_ bflow.tr
	set flowfile_ ff.tr
	set graphfile_ reclass.xgr
	set penaltyfile_ reclassa.tr
	set flowgraphfile_ ff.xgr
}

#
# Create traffic.
#
Test/one instproc traffic1 {} {
    $self instvar node_ maxfid_
    $self new_tcp 1.0 $node_(s1) $node_(s3) 100 1 1 1000
    $self new_tcp 4.2 $node_(s2) $node_(s4) 100 2 0 50
#    new_cbr 18.4 $s1 $s4) 190 0.00003 3
    $self new_cbr 18.4 $node_(s1) $node_(s4) 190 0.003 3
    $self new_tcp 65.4 $node_(s1) $node_(s4) 4 4 0 2000
    $self new_tcp 100.2 $node_(s3) $node_(s1) 8 5 0 1000
    $self new_tcp 122.6 $node_(s1) $node_(s4) 4 6 0 512
    $self new_tcp 135.0 $node_(s4) $node_(s2) 100 7 0 1000
    $self new_tcp 162.0 $node_(s2) $node_(s3) 100 8 0 1000
    $self new_tcp 220.0 $node_(s1) $node_(s3) 100 9 0 512
    $self new_tcp 260.0 $node_(s3) $node_(s2) 100 10 0 512
    $self new_cbr 310.0 $node_(s2) $node_(s4) 190 0.1 11 
    $self new_tcp 320.0 $node_(s1) $node_(s4) 100 12 0 512
    $self new_tcp 350.0 $node_(s1) $node_(s3) 100 13 0 512
    $self new_tcp 370.0 $node_(s3) $node_(s2) 100 14 0 512
    $self new_tcp 390.0 $node_(s2) $node_(s3) 100 15 0 512
    $self new_tcp 420.0 $node_(s2) $node_(s4) 100 16 0 512
    $self new_tcp 440.0 $node_(s2) $node_(s4) 100 17 0 512
    set maxfid_ 17
}


Test/one instproc run {} {
    $self instvar ns_ net_ topo_
    $topo_ instvar cbqlink_ node_
    set cbqlink $cbqlink_

    # set stoptime 300.0
    set stoptime 100.0

	set rtt 0.06
	set mtu 512

	set rtm [new RTMechanisms $ns_ $cbqlink $rtt $mtu]

	$self instvar goodflowfile_
	set gfm [$rtm makeflowmon]
	set gflowf [open $goodflowfile_ w]
	$gfm set enable_in_ false	; # no per-flow arrival state
	$gfm set enable_out_ false	; # no per-flow departure state
	$gfm attach $gflowf

	$self instvar badflowfile_
	set bfm [$rtm makeflowmon]
	set bflowf [open $badflowfile_ w]
	$bfm attach $bflowf

	$rtm makeboxes $gfm $bfm 100 1000
	$rtm bindboxes

	set L1 [$ns_ link $node_(r1) $node_(r2)]
	$self linkDumpFlows $L1 1.0 $stoptime

	$self traffic1
	$ns_ at $stoptime "$self finish"

	ns-random 0
	$ns_ run
}

TestSuite proc usage {} {
        global argv0
        puts stderr "usage: ns $argv0 <tests> \[<topologies>\]"
        puts stderr "Valid tests are:\t[$self get-subclasses TestSuite Test/]"
        puts stderr "Valid Topologies are:\t[$self get-subclasses SkelTopology Topology/]"
        exit 1
}

TestSuite proc isProc? {cls prc} {
        if [catch "Object info subclass $cls/$prc" r] {
                global argv0
                puts stderr "$argv0: no such $cls: $prc"
                $self usage
        }
}

TestSuite proc get-subclasses {cls pfx} {
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
                        $self isProc? Test $test

                        set topo ""
                }
                2 {
                        set test [lindex $argv 0]
                        $self isProc? Test $test

                        set topo [lindex $argv 1]
                        $self isProc? Topology $topo
                }
                default {
                        $self usage
                }
        }
        set t [new Test/$test $topo]
        $t run
}

TestSuite runTest

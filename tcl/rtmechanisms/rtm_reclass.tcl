#
# Main test file for the router mechanisms simulation
#
Class TestSuite
source mechanisms.tcl
source sources.tcl
source rtm_plot.tcl

TestSuite instproc init {} {
        $self instvar ns_ defNet_ net_ test_ topo_ node_ testName_
	$self instvar scheduler_

        set ns_ [new Simulator]
	set scheduler_ [$ns_ set scheduler_]
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
TestSuite instproc linkDumpFlows { linkmon interval stoptime } {
	$self instvar ns_ linkflowfile_
        set f [open $linkflowfile_ w]
puts "linkDumpFlows: opening file $linkflowfile_, fdesc: $f"
        TestSuite instproc dump1 { file linkmon interval } {
		$self instvar ns_ linkmon_
                $ns_ at [expr [$ns_ now] + $interval] \
			"$self dump1 $file $linkmon $interval"
		foreach flow [$linkmon flows] {
			set bytes [$flow set bdepartures_]
			if {$bytes > 0} {
				puts $file \
			    "time: [$ns_ now] class: [$flow set flowid_] bytes: $bytes $interval"     
			}
		}
        }
        $ns_ at $interval "$self dump1 $f $linkmon $interval"
        $ns_ at $stoptime "flush $f"
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
	$self instvar linkflowfile_ linkgraphfile_
	$self instvar goodflowfile_ goodgraphfile_
	$self instvar badflowfile_ badgraphfile_
	$self instvar label_ post_

	set label_ RECLASS2

	set linkflowfile_ RECLASS2.tr
	set linkgraphfile_ RECLASS2.xgr

	set goodflowfile_ RECLASS2_gf.tr
	set goodgraphfile_ RECLASS2_gf.xgr

	set badflowfile_ RECLASS2_bf.tr
	set badgraphfile_ RECLASS2_bf.xgr

	set post_ [new PostProcess $label_ $linkflowfile_ $linkgraphfile_ \
		$goodflowfile_ $goodgraphfile_ \
		$badflowfile_ $badgraphfile_]

	$post_ set format_ "xgraph"
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
	set L1 [$rtm monitor-link]
	$self linkDumpFlows $L1 1.0 $stoptime

	$self traffic1
	$ns_ at $stoptime "$self finish"

	ns-random 0
	$ns_ run
}

Test/one instproc finish {} {
	$self instvar post_ scheduler_
	$scheduler_ halt
	set bandwidth 1500
	$post_ plot_bytes $bandwidth
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

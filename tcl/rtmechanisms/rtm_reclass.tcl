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
#        if ![Topology info subclass Topology/$net_] {
#                global argv0
#                puts stderr "$argv0: cannot run test $test_ over topology $net_"
#                exit 1
#        }
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
	set node_(s5) [$ns node]
	set node_(s6) [$ns node]

	set node_(r1) [$ns node]
	set node_(r2) [$ns node]
}

Class Topology/net2 -superclass NodeTopology/6nodes
Topology/net2 instproc init ns {
	$self next $ns
	$self instvar node_ cbqlink_ bandwidth_ rtt_

	$ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
	$ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
	set cl [new Classifier/Hash/SrcDestFid 33]
	$ns simplex-link $node_(r1) $node_(r2) 1.5Mb 30ms CBQ $cl
        set rtt_ 0.06
	set cbqlink_ [$ns link $node_(r1) $node_(r2)]
	[$cbqlink_ queue] algorithm "formal"
	$ns simplex-link $node_(r2) $node_(r1) 1.5Mb 30ms DropTail
	set bandwidth_ 1500
	[[$ns link $node_(r2) $node_(r1)] queue] set limit_ 25
	$ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
	$ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
	$ns duplex-link $node_(s5) $node_(r1) 10Mb 10ms DropTail
	$ns duplex-link $node_(s6) $node_(r2) 10Mb 1ms DropTail

	return $cbqlink_
}

Class Topology/net3 -superclass NodeTopology/6nodes
Topology/net3 instproc init ns {
	$self next $ns
	$self instvar node_ cbqlink_ bandwidth_ rtt_

	$ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
	$ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
	set cl [new Classifier/Hash/SrcDestFid 33]
	$ns simplex-link $node_(r1) $node_(r2) 1.5Mb 3ms CBQ $cl
        set rtt_ 0.006
	set cbqlink_ [$ns link $node_(r1) $node_(r2)]
	[$cbqlink_ queue] algorithm "formal"
	$ns simplex-link $node_(r2) $node_(r1) 1.5Mb 3ms DropTail
	set bandwidth_ 1500
	[[$ns link $node_(r2) $node_(r1)] queue] set limit_ 25
	$ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
	$ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
	$ns duplex-link $node_(s5) $node_(r1) 10Mb 10ms DropTail
	$ns duplex-link $node_(s6) $node_(r2) 10Mb 1ms DropTail

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

#----------------------

TestSuite instproc finish {} {
	$self instvar post_ scheduler_
	$scheduler_ halt
	set bandwidth 1500
	$post_ plot_bytes $bandwidth
}

TestSuite instproc config { name } {
	$self instvar linkflowfile_ linkgraphfile_
	$self instvar goodflowfile_ goodgraphfile_
	$self instvar badflowfile_ badgraphfile_
	$self instvar label_ post_

	set label_ $name

	set linkflowfile_ $name.tr
	set linkgraphfile_ $name.xgr

	set goodflowfile_ $name.gf.tr
	set goodgraphfile_ $name.gf.xgr

	set badflowfile_ $name.bf.tr
	set badgraphfile_ $name.bf.xgr

	set post_ [new PostProcess $label_ $linkflowfile_ $linkgraphfile_ \
		$goodflowfile_ $goodgraphfile_ \
		$badflowfile_ $badgraphfile_]

	$post_ set format_ "xgraph"
}

#
# Create traffic.
#
TestSuite instproc traffic1 {} {
    $self instvar node_ 
    $self new_Tcp 4.2 $node_(s2) $node_(s4) 100 2 0 50 reno 60000
    $self new_Cbr 18.4 $node_(s1) $node_(s4) 200 0.003 3 0
    $self new_Tcp 65.4 $node_(s1) $node_(s4) 2 4 0 1500 sack 2700
    $self new_Tcp 100.2 $node_(s3) $node_(s1) 8 5 0 1000 reno 0
    $self new_Tcp 122.6 $node_(s5) $node_(s4) 4 6 0 512 sack 4000
    $self new_Tcp 135.0 $node_(s4) $node_(s2) 100 7 0 1000 reno 0
    $self new_Tcp 162.0 $node_(s2) $node_(s6) 100 8 0 1000 sack 3300
    $self new_Tcp 220.0 $node_(s1) $node_(s3) 100 9 0 512 reno 3000
    $self new_Tcp 260.0 $node_(s3) $node_(s2) 100 10 0 512 sack 0
    $self new_Cbr 310.0 $node_(s2) $node_(s4) 190 0.1 11 0 
    $self new_Tcp 320.0 $node_(s1) $node_(s4) 100 12 0 1500 reno 500
    $self new_Tcp 350.0 $node_(s5) $node_(s6) 100 13 0 512 reno 1000
    $self new_Tcp 370.0 $node_(s3) $node_(s2) 100 14 0 1500 sack 0
    $self new_Tcp 390.0 $node_(s2) $node_(s3) 100 15 0 512 reno 0
    $self new_Tcp 420.0 $node_(s5) $node_(s6) 100 16 0 512 reno 0
    $self new_Tcp 440.0 $node_(s2) $node_(s4) 100 17 0 512  reno 0
    $self new_Tcp 22.0 $node_(s2) $node_(s6) 100 18 0 1500 sack 6000
    $self new_Tcp 3.3 $node_(s6) $node_(s2) 100 19 0 500 sack 0
    $self new_Tcp 28.0 $node_(s5) $node_(s4) 100 20 0 500 reno 8000
    $self new_Cbr 80.0 $node_(s4) $node_(s2) 200 0.5 21 0
    $self new_Tcp 1.0  $node_(s1) $node_(s3) 100 25 0 1500 reno 4000
}

#
# Create traffic for UNFRIENDLY test.
#
TestSuite instproc traffic2 {} {
    $self instvar node_
    $self new_Tcp 1.0  $node_(s1) $node_(s3) 50 1 0 1500 sack 0
    $self new_Tcp 2.2 $node_(s2) $node_(s4) 50 2 0 1500 sack 0
    # 66,666 Bps for CBR flow, 187,500 Bps for link.
#    $self new_Cbr 58.4 $node_(s1) $node_(s4) 200 0.003 3 0
    $self new_Cbr 58.4 $node_(s1) $node_(s4) 500 0.003 3 0
    $self new_Tcp 3.4 $node_(s1) $node_(s4) 50 4 0 1500 sack 0
    $self new_Tcp 34.2 $node_(s3) $node_(s1) 50 5 0 1500 sack 0
    $self new_Tcp 35.6 $node_(s5) $node_(s4) 50 6 0 1500 sack 0
    $self new_Tcp 36.0 $node_(s4) $node_(s2) 50 7 0 1500 sack 0
    $self new_Tcp 37.3 $node_(s2) $node_(s6) 50 8 0 1500 sack 0
    $self new_Tcp 38.0 $node_(s1) $node_(s3) 50 9 0 1500 sack 0
    $self new_Tcp 39.5 $node_(s3) $node_(s2) 50 10 0 1500 sack 0
    $self new_Tcp 35.6 $node_(s2) $node_(s6) 50 11 0 1500 sack 0
    $self new_Tcp 30.2 $node_(s1) $node_(s4) 50 12 0 1500 sack 0
    $self new_Tcp 31.3 $node_(s5) $node_(s6) 50 13 0 1500 sack 0
    $self new_Tcp 32.9 $node_(s3) $node_(s2) 50 14 0 1500 sack 0
    $self new_Tcp 33.8 $node_(s2) $node_(s3) 50 15 0 1500 sack 0
    $self new_Tcp 34.0 $node_(s5) $node_(s6) 50 16 0 1500 sack 0
    $self new_Tcp 35.5 $node_(s2) $node_(s4) 50 17 0 1500  sack 0
    $self new_Tcp 36.1 $node_(s1) $node_(s4) 50 18 0 1500 sack 0
    $self new_Tcp 45.6 $node_(s5) $node_(s4) 50 19 0 1500 sack 0
    $self new_Tcp 47.3 $node_(s2) $node_(s6) 50 20 0 1500 sack 0
#     $self new_Tcp 48.0 $node_(s1) $node_(s4) 50 21 0 1500 sack 0
#     $self new_Tcp 42.6 $node_(s5) $node_(s4) 50 22 0 1500 sack 0
#     $self new_Tcp 43.3 $node_(s2) $node_(s6) 50 23 0 1500 sack 0
#     $self new_Tcp 46.0 $node_(s1) $node_(s4) 50 24 0 1500 sack 0
#     $self new_Tcp 42.6 $node_(s5) $node_(s4) 50 25 0 1500 sack 0
#     $self new_Tcp 43.3 $node_(s2) $node_(s6) 50 26 0 1500 sack 0
#     $self new_Tcp 41.0 $node_(s1) $node_(s4) 50 27 0 1500 sack 0
#     $self new_Tcp 46.6 $node_(s5) $node_(s4) 50 28 0 1500 sack 0
#     $self new_Tcp 48.3 $node_(s2) $node_(s6) 50 29 0 1500 sack 0
#     $self new_Tcp 45.0 $node_(s1) $node_(s4) 50 30 0 1500 sack 0
#    $self new_Cbr 38.4 $node_(s2) $node_(s3) 200 0.006 31 0
#    $self new_Cbr 48.4 $node_(s5) $node_(s6) 200 0.004 32 0
#    $self new_Cbr 28.4 $node_(s2) $node_(s3) 200 0.005 33 0
}

#
# For a 1% drop rate.
#
TestSuite instproc traffic3 {} {
    $self instvar node_
#    $self new_Cbr 1.0 $node_(s2) $node_(s4) 1500 0.008 1 0
    $self new_Cbr 1.0 $node_(s2) $node_(s4) 1515 0.008 1 0
}

#
# Create traffic.
#
TestSuite instproc traffic4 {} {
    $self instvar node_
    $self new_Tcp 12.0  $node_(s1) $node_(s3) 50 1 0 1500 sack 0
    $self new_Tcp 5.2 $node_(s2) $node_(s4) 50 2 0 1500 sack 0
    $self new_Cbr 1.4 $node_(s1) $node_(s4) 300 0.003 3 0
    $self new_Tcp 17.4 $node_(s1) $node_(s4) 50 4 0 1500 sack 0
    $self new_Tcp 34.2 $node_(s3) $node_(s1) 50 5 0 1500 sack 0
    $self new_Tcp 35.6 $node_(s5) $node_(s4) 50 6 0 1500 sack 0
    $self new_Tcp 56.0 $node_(s4) $node_(s2) 50 7 0 1500 sack 0
    $self new_Tcp 37.3 $node_(s2) $node_(s6) 50 8 0 1500 sack 0
    $self new_Tcp 78.0 $node_(s1) $node_(s3) 50 9 0 1500 sack 0
    $self new_Tcp 39.5 $node_(s3) $node_(s2) 50 10 0 1500 sack 0
    $self new_Tcp 85.6 $node_(s2) $node_(s6) 50 11 0 1500 sack 0
    $self new_Tcp 30.2 $node_(s1) $node_(s4) 50 12 0 1500 sack 0
    $self new_Tcp 21.3 $node_(s5) $node_(s6) 50 13 0 1500 sack 0
    $self new_Tcp 32.9 $node_(s3) $node_(s2) 50 14 0 1500 sack 0
    $self new_Tcp 23.8 $node_(s2) $node_(s3) 50 15 0 1500 sack 0
    $self new_Tcp 34.0 $node_(s5) $node_(s6) 50 16 0 1500 sack 0
    $self new_Tcp 55.5 $node_(s2) $node_(s4) 50 17 0 1500  sack 0
    $self new_Tcp 36.1 $node_(s1) $node_(s4) 50 18 0 1500 sack 0
    $self new_Tcp 45.6 $node_(s5) $node_(s4) 50 19 0 1500 sack 0
    $self new_Tcp 47.3 $node_(s2) $node_(s6) 50 20 0 1500 sack 0
    $self new_Tcp 68.0 $node_(s1) $node_(s4) 50 21 0 1500 sack 0
    $self new_Tcp 42.6 $node_(s5) $node_(s4) 50 22 0 1500 sack 0
    $self new_Tcp 43.3 $node_(s2) $node_(s6) 50 23 0 1500 sack 0
    $self new_Tcp 46.0 $node_(s1) $node_(s4) 50 24 0 1500 sack 0
    $self new_Tcp 42.6 $node_(s5) $node_(s4) 50 25 0 1500 sack 0
    $self new_Tcp 43.3 $node_(s2) $node_(s6) 50 26 0 1500 sack 0
    $self new_Tcp 41.0 $node_(s1) $node_(s4) 50 27 0 1500 sack 0
    $self new_Tcp 46.6 $node_(s5) $node_(s4) 50 28 0 1500 sack 0
    $self new_Tcp 48.3 $node_(s2) $node_(s6) 50 29 0 1500 sack 0
    $self new_Tcp 45.0 $node_(s1) $node_(s4) 50 30 0 1500 sack 0
}



#
# Create traffic.
#
TestSuite instproc more_cbrs {} {
    $self instvar node_ 
    $self new_Cbr 105.0 $node_(s2) $node_(s4) 200 0.006 22 50000
    $self new_Cbr 234.0 $node_(s1) $node_(s3) 220 0.01 23 10000
    $self new_Cbr 277.0 $node_(s1) $node_(s3) 180 0.01 24 10000
    $self new_Cbr 283.0 $node_(s1) $node_(s3) 220 0.02 26 5000
    $self new_Cbr 289.0 $node_(s1) $node_(s3) 180 0.02 27 5000
}

#-----------------------

Class Test/one -superclass TestSuite
Test/one instproc init { topo name enable } {
        $self instvar net_ defNet_ test_ enable_
        set net_ $topo   
        set defNet_ net2
        set test_ $name
	set enable_ $enable
        $self next
	$self config $name
}

#
# For test in Figure 11 of the paper.
#
Test/one instproc run {} {
    $self instvar ns_ net_ topo_ enable_
    $topo_ instvar cbqlink_ node_ rtt_
    set cbqlink $cbqlink_

    set stoptime 600.0
#    set stoptime 100.0

	set mtu 1500

	set rtm [new RTMechanisms $ns_ $cbqlink $rtt_ $mtu $enable_]

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
	$self linkDumpFlows $L1 20.0 $stoptime

	$self traffic2
	$ns_ at $stoptime "$self finish"

	ns-random 0
	$ns_ run
}

#--------

Class Test/two -superclass TestSuite
Test/two instproc init { topo name enable } {
        $self instvar net_ defNet_ test_ enable_
        set net_ $topo   
        set defNet_ net2
        set test_ $name
	set enable_ $enable
        $self next
	$self config $name
}

#
# UNFRIENDLY test.
#
Test/two instproc run {} {
    $self instvar ns_ net_ topo_ enable_
    $topo_ instvar cbqlink_ node_ rtt_
    set cbqlink $cbqlink_

#    set stoptime 600.0
    set stoptime 100.0

	set mtu 1500

	set rtm [new RTMechanisms $ns_ $cbqlink $rtt_ $mtu $enable_]

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

	$self traffic2
#	$self traffic3
#        $self more_cbrs
	$ns_ at $stoptime "$self finish"

	ns-random 0
	$ns_ run
}

#--------

Class Test/three -superclass TestSuite
Test/three instproc init { topo name enable } {
        $self instvar net_ defNet_ test_ enable_
        set net_ $topo   
        set defNet_ net2
        set test_ $name
	set enable_ $enable
        $self next
	$self config $name
}

#
# UNFRIENDLY test.
#
Test/three instproc run {} {
    $self instvar ns_ net_ topo_ enable_
    $topo_ instvar cbqlink_ node_ rtt_
    set cbqlink $cbqlink_

#    set stoptime 600.0
    set stoptime 100.0

	set mtu 1500

	set rtm [new RTMechanisms $ns_ $cbqlink $rtt_ $mtu $enable_ ]

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

	$self traffic4
	$ns_ at $stoptime "$self finish"

	ns-random 0
	$ns_ run
}

#---------


TestSuite proc usage {} {
        global argv0
        puts stderr "usage: ns $argv0 <tests> \[enable|disable\] \[<topologies>\]"
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
	set enable true
        switch $argc {
                1 {
                        set test $argv
                        $self isProc? Test $test

                        set topo ""
                }
                2 {
                        set test [lindex $argv 0]
                        $self isProc? Test $test

			set enable [lindex $argv 1]
			if { $enable == "disable" } {
				set enable false
			} else {
				set enable true
			}

			set topo ""
                }
                3 {
                        set test [lindex $argv 0]
                        $self isProc? Test $test

                        set enable [lindex $argv 1]
			if { $enable == "disable" } {
				set enable false
			} else {
				set enable true
			}

                        set topo [lindex $argv 2]
                        $self isProc? Topology $topo

                }
                default {
                        $self usage
                }
        }
        set t [new Test/$test $topo $test $enable]
        $t run
}

TestSuite runTest

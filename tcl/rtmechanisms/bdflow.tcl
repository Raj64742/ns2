source bdflow_h.tcl
source Setred.v2.tcl
#
set flowfile fairflow.tr
set flowgraphfile fairflow.xgr
#set highspeed_bw 10Mb
#set bottleneck_bw 1.5Mb
set highspeed_bw 670Mb
set bottleneck_bw 100Mb
#------------------------------------------------------------------

#
# Create traffic.
#

# highspeed_bw = 10Mb
# bottleneck_bw = 1.5Mb
#
proc traffic {} {
  global s1 s2 r1 r2 s3 s4

  new_tcp 1.0 $s1 $s3 100 1 1 1000
  new_tcp 4.2 $s2 $s4 100 2 0 50
  new_cbr 18.4 $s1 $s4 190 0.003 3
  new_tcp 65.4 $s1 $s4 4 4 0 2000
  new_tcp 100.2 $s3 $s1 8 5 0 1000
  new_tcp 122.6 $s1 $s4 4 6 0 512
  new_tcp 135.0 $s4 $s2 100 7 0 1000
  new_tcp 162.0 $s2 $s3 100 8 0 1000
  new_tcp 220.0 $s1 $s3 100 9 0 512
  new_tcp 260.0 $s3 $s2 100 10 0 512
  new_cbr 310.0 $s2 $s4 190 0.1 11 
  new_tcp 320.0 $s1 $s4 100 12 0 512
  new_tcp 350.0 $s1 $s3 100 13 0 512
  new_tcp 370.0 $s3 $s2 100 14 0 512
  new_tcp 390.0 $s2 $s3 100 15 0 512
  new_tcp 420.0 $s2 $s4 100 16 0 512
  new_tcp 440.0 $s2 $s4 100 17 0 512
}

proc random_tcps {start stop n src dst win id size} {
    set maxrval [expr pow(2,31)]
    
    for { set i 0 } { $i < $n } { incr i } {
        set randomval [ns-random]
        set rtmp [expr $randomval / $maxrval]
        set start_time [expr $start + [expr $rtmp * [expr $stop - $start]]]
	new_tcp $start_time $src $dst $win $id 0 $size
	incr id
    }
}

proc random_cbrs {start stop n src dst size int id} {
    set maxrval [expr pow(2,31)]
    
    for { set i 0 } { $i < $n } { incr i } {
        set randomval [ns-random]
        set rtmp [expr $randomval / $maxrval]
        set start_time [expr $start + [expr $rtmp * [expr $stop - $start]]]
	new_cbr $start_time $src $dst $size $int $id
	incr id
    }
}

# highspeed_bw = 10Mb
# bottleneck_bw = 1.5Mb
#
proc traffic_LbwLcg {} {
    global s1 s2 r1 r2 s3 s4

    new_cbr 18.4 $s1 $s4 190 0.003 3

    random_tcps 0.0 500.0 1 $s1 $s3 100 100 1500
    random_tcps 250.0 500.0 1 $s2 $s4 100 101 1500

    random_tcps 0.0 500.0 1 $s2 $s3 100 200 1000

    random_tcps 0.0 500.0 1 $s1 $s3 100 300 512
    random_tcps 250.0 500.0 1 $s2 $s3 100 301 512

    random_tcps 0.0 500.0 1 $s4 $s1 100 302 512

    random_cbrs 0.0 500.0 1 $s1 $s3 190 0.1 400
}

# highspeed_bw = 10Mb
# bottleneck_bw = 1.5Mb
# 38 flows
#
proc traffic_LbwHcg {} {
    global s1 s2 r1 r2 s3 s4
    
    new_cbr 18.4 $s1 $s4 190 0.003 3

    random_tcps 0.0 500.0 3 $s1 $s3 100 100 1500
    random_tcps 125.0 500.0 3 $s1 $s4 100 106 1500
    random_tcps 250.0 500.0 3 $s2 $s3 100 112 1500
    random_tcps 375.0 500.0 4 $s2 $s4 100 117 1500

    random_tcps 0.0 500.0 1 $s3 $s1 100 122 1500

    random_tcps 0.0 500.0 1 $s2 $s4 8 123 1500

    random_tcps 0.0 500.0 1 $s1 $s3 100 200 1000
    random_tcps 250.0 500.0 1 $s2 $s3 100 201 1000

    random_tcps 0.0 500.0 1 $s3 $s1 100 202 1000

    random_tcps 0.0 500.0 1 $s2 $s3 4 203 1000

    random_tcps 0.0 500.0 1 $s1 $s3 100 300 512
    random_tcps 150.0 500.0 1 $s2 $s3 100 301 512

    random_tcps 0.0 500.0 1 $s4 $s1 100 304 512

    random_tcps 0.0 500.0 1 $s3 $s1 8 306 512

    random_cbrs 0.0 500.0 1 $s1 $s3 190 0.1 400

    random_cbrs 0.0 500.0 1 $s4 $s2 190 0.1 401
}

# highspeed_bw = 670Mb
# bottleneck_bw = 100Mb
# 605 flows
#
proc traffic_HbwLcg {} {
    global s1 s2 r1 r2 s3 s4

    new_cbr 18.4 $s1 $s4 190 0.00006 3

    random_tcps 0.0 500.0 2 $s1 $s3 100 100 4000
    random_tcps 125.0 500.0 2 $s1 $s4 100 103 4000
    random_tcps 250.0 500.0 2 $s2 $s3 100 106 4000
    random_tcps 375.0 500.0 2 $s2 $s4 100 109 4000

    random_tcps 0.0 500.0 3 $s1 $s3 100 200 2000
    random_tcps 125.0 500.0 3 $s1 $s4 100 204 2000
    random_tcps 250.0 500.0 3 $s2 $s3 100 208 2000
    random_tcps 375.0 500.0 3 $s2 $s4 100 212 2000

    random_tcps 0.0 500.0 80 $s1 $s3 100 300 1500
    random_tcps 125.0 500.0 80 $s1 $s4 100 385 1500
    random_tcps 250.0 500.0 80 $s2 $s3 100 465 1500
    random_tcps 375.0 500.0 80 $s2 $s4 100 545 1500

    random_tcps 0.0 500.0 10 $s3 $s1 100 625 1500
    random_tcps 125.0 500.0 10 $s3 $s2 100 635 1500
    random_tcps 250.0 500.0 10 $s4 $s1 100 645 1500
    random_tcps 375.0 500.0 10 $s4 $s2 100 655 1500

    random_tcps 0.0 500.0 2 $s1 $s3 8 665 1500
    random_tcps 125.0 500.0 2 $s1 $s4 4 667 1500
    random_tcps 250.0 500.0 2 $s2 $s3 8 669 1500
    random_tcps 375.0 500.0 2 $s2 $s4 4 671 1500

    random_tcps 0.0 500.0 10 $s1 $s3 100 700 1000
    random_tcps 125.0 500.0 10 $s2 $s3 100 710 1000
    random_tcps 250.0 500.0 10 $s1 $s4 100 720 1000
    random_tcps 375.0 500.0 10 $s2 $s4 100 730 1000

    random_tcps 0.0 500.0 4 $s3 $s1 100 740 1000
    random_tcps 125.0 500.0 4 $s3 $s2 100 744 1000
    random_tcps 250.0 500.0 4 $s4 $s1 100 748 1000
    random_tcps 375.0 500.0 4 $s4 $s2 100 752 1000

    random_tcps 0.0 500.0 2 $s1 $s3 4 756 1000
    random_tcps 125.0 500.0 2 $s1 $s4 8 758 1000
    random_tcps 250.0 500.0 2 $s2 $s3 4 760 1000
    random_tcps 375.0 500.0 2 $s2 $s4 8 762 1000

    random_tcps 0.0 500.0 20 $s1 $s3 100 800 512
    random_tcps 125.0 500.0 20 $s1 $s4 100 820 512
    random_tcps 250.0 500.0 20 $s2 $s3 100 840 512
    random_tcps 375.0 500.0 20 $s2 $s4 100 860 512

    random_tcps 0.0 500.0 6 $s3 $s1 100 880 512
    random_tcps 125.0 500.0 6 $s3 $s2 100 886 512
    random_tcps 250.0 500.0 6 $s4 $s1 100 892 512
    random_tcps 375.0 500.0 6 $s4 $s2 100 898 512

    random_tcps 0.0 500.0 2 $s1 $s3 8 914 512
    random_tcps 125.0 500.0 2 $s1 $s4 8 916 512
    random_tcps 250.0 500.0 2 $s2 $s3 4 918 512
    random_tcps 375.0 500.0 2 $s2 $s4 4 920 512

    random_cbrs 0.0 500.0 2 $s1 $s3 190 0.1 1000
    random_cbrs 125.0 500.0 2 $s1 $s4 190 0.1 1002
    random_cbrs 250.0 500.0 2 $s2 $s3 190 0.1 1004
    random_cbrs 375.0 500.0 2 $s2 $s4 190 0.1 1006

    random_cbrs 0.0 500.0 1 $s3 $s1 190 0.1 1008
    random_cbrs 125.0 500.0 1 $s3 $s2 190 0.1 1009
    random_cbrs 250.0 500.0 1 $s4 $s1 190 0.1 1010
    random_cbrs 375.0 500.0 1 $s4 $s2 190 0.1 1011
}

# highspeed_bw = 670Mb
# bottleneck_bw = 100Mb
#
proc traffic_HbwHcg {} {
    global s1 s2 r1 r2 s3 s4

    new_cbr 18.4 $s1 $s4 190 0.00005 3

    random_tcps 0.0 500.0 8 $s1 $s3 100 100 4000
    random_tcps 125.0 500.0 8 $s1 $s4 100 115 4000
    random_tcps 250.0 500.0 8 $s2 $s3 100 130 4000
    random_tcps 375.0 500.0 8 $s2 $s4 100 145 4000

    random_tcps 0.0 500.0 12 $s1 $s3 100 200 2000
    random_tcps 125.0 500.0 12 $s1 $s4 100 220 2000
    random_tcps 250.0 500.0 12 $s2 $s3 100 240 2000
    random_tcps 375.0 500.0 12 $s2 $s4 100 260 2000

    random_tcps 0.0 500.0 130 $s1 $s3 100 300 1500
    random_tcps 125.0 500.0 130 $s1 $s4 100 400 1500
    random_tcps 250.0 500.0 130 $s2 $s3 100 500 1500
    random_tcps 375.0 500.0 130 $s2 $s4 100 600 1500

    random_tcps 0.0 500.0 20 $s3 $s1 100 625 1500
    random_tcps 125.0 500.0 20 $s3 $s2 100 635 1500
    random_tcps 250.0 500.0 20 $s4 $s1 100 645 1500
    random_tcps 375.0 500.0 20 $s4 $s2 100 655 1500

    random_tcps 0.0 500.0 4 $s1 $s3 8 665 1500
    random_tcps 125.0 500.0 4 $s1 $s4 4 667 1500
    random_tcps 250.0 500.0 4 $s2 $s3 8 669 1500
    random_tcps 375.0 500.0 4 $s2 $s4 4 671 1500

    random_tcps 0.0 500.0 20 $s1 $s3 100 700 1000
    random_tcps 125.0 500.0 20 $s2 $s3 100 710 1000
    random_tcps 250.0 500.0 20 $s1 $s4 100 720 1000
    random_tcps 375.0 500.0 20 $s2 $s4 100 730 1000

    random_tcps 0.0 500.0 8 $s3 $s1 100 740 1000
    random_tcps 125.0 500.0 8 $s3 $s2 100 744 1000
    random_tcps 250.0 500.0 8 $s4 $s1 100 748 1000
    random_tcps 375.0 500.0 8 $s4 $s2 100 752 1000

    random_tcps 0.0 500.0 4 $s1 $s3 4 756 1000
    random_tcps 125.0 500.0 4 $s1 $s4 8 758 1000
    random_tcps 250.0 500.0 4 $s2 $s3 4 760 1000
    random_tcps 375.0 500.0 4 $s2 $s4 8 762 1000

    random_tcps 0.0 500.0 12 $s1 $s3 100 800 512
    random_tcps 125.0 500.0 12 $s1 $s4 100 820 512
    random_tcps 250.0 500.0 12 $s2 $s3 100 840 512
    random_tcps 375.0 500.0 12 $s2 $s4 100 860 512

    random_tcps 0.0 500.0 12 $s3 $s1 100 880 512
    random_tcps 125.0 500.0 12 $s3 $s2 100 886 512
    random_tcps 250.0 500.0 12 $s4 $s1 100 892 512
    random_tcps 375.0 500.0 12 $s4 $s2 100 898 512

    random_tcps 0.0 500.0 4 $s1 $s3 8 914 512
    random_tcps 125.0 500.0 4 $s1 $s4 8 916 512
    random_tcps 250.0 500.0 4 $s2 $s3 4 918 512
    random_tcps 375.0 500.0 4 $s2 $s4 4 920 512

    random_cbrs 0.0 500.0 4 $s1 $s3 190 0.0002 1000
    random_cbrs 125.0 500.0 4 $s1 $s4 190 0.0002 1002
    random_cbrs 250.0 500.0 4 $s2 $s3 190 0.0003 1004
    random_cbrs 375.0 500.0 4 $s2 $s4 190 0.0003 1006

    random_cbrs 0.0 500.0 2 $s3 $s1 190 0.1 1008
    random_cbrs 125.0 500.0 2 $s3 $s2 190 0.1 1009
    random_cbrs 250.0 500.0 2 $s4 $s1 190 0.1 1010
  random_cbrs 375.0 500.0 2 $s4 $s2 190 0.1 1011
}

proc add_traffic {} {
	traffic_HbwHcg
  puts "traffic created"
}

#------------------------------------------------------------------
proc create_testnet6 { queuetype }  {
    global ns s1 s2 r1 r2 s3 s4 highspeed_bw bottleneck_bw

    set s1 [$ns node]
    set s2 [$ns node]
    set r1 [$ns node]
    set r2 [$ns node]
    set s3 [$ns node]
    set s4 [$ns node]
    
    $ns duplex-link $s1 $r1 $highspeed_bw 2ms DropTail
    $ns duplex-link $s2 $r1 $highspeed_bw 3ms DropTail
    #$ns simplex-link $r1 $r2 $bottleneck_bw 20ms $queuetype
    #$ns simplex-link $r2 $r1 $bottleneck_bw 20ms $queuetype
    $ns simplex-link $r1 $r2 $bottleneck_bw 5ms $queuetype
    $ns simplex-link $r2 $r1 $bottleneck_bw 5ms $queuetype
    set redlink [$ns link $r1 $r2]
    [[$ns link $r2 $r1] queue] set limit_ 25
    [[$ns link $r1 $r2] queue] set limit_ 25
    $ns duplex-link $s3 $r2 $highspeed_bw 4ms DropTail
    $ns duplex-link $s4 $r2 $highspeed_bw 5ms DropTail
    
    return $redlink
}

proc finish_ns {} {
    global ns flowdesc
    $ns instvar scheduler_
    $scheduler_ halt
    puts "simulation complete"
    close $flowdesc
}

proc test {testname seed finishfile label createflows dump queue} {
    global ns s1 s2 r1 r2 s3 s4 r1fm flowgraphfile
   
    # set stoptime 300.0
    set stoptime 500.0

    # ? Final sims use 500.0 while paper mentions 300.0
    set queuetype $queue
    # Set queuesize 25 for Drop Tail, later set to 100 for RED
    set queuesize 25

    set ns [new Simulator]
    # makes simulation run faster
    $ns use-scheduler Calendar
    set redlink [create_testnet6 $queuetype]
    [$redlink queue] set limit_ $queuesize
    [[$ns link $r2 $r1] queue] set limit_ $queuesize
    if {$queuetype == "RED"} {
	set_Red $r1 $r2
	[$redlink queue] set limit_ 100
	new_tcp 50.2 $s1 $s3 100 20 0 1500
	new_tcp 50.2 $s1 $s3 100 21 0 1500
    }
    $createflows $redlink $dump $stoptime
    add_traffic
    new_tcp 50.2 $s1 $s3 100 18 0 1460
    new_tcp 50.5 $s1 $s3 100 19 0 1460

	for {set t 0} {$t < 500} {incr t 50} {
	  $ns at $t "puts time=$t"
	}
    $ns at $stoptime "$finishfile $testname $flowgraphfile.$label"
    $ns at $stoptime "finish"

    # trace only the bottleneck link
    #    [$ns link $r1 $r2] trace [openTrace $stoptime $testname]

    ns-random $seed

    $ns run
}

#------------------------------------------------------------------

# plot_dropsinpackets looks at every 100 drops,
# plot_dropsinpackets1 for each flow waits until "sufficient"
#   drops have accumulated
# unforced drops, packet drop metric, RED
proc test_unforced { seed } {
    global category ns_link queuetype
    set queuetype packets
    set ns_link(queue-in-bytes) false
    set category unforced
    #	test $seed plot_dropsinpackets1 u create_flowstats flowDump RED
    test unforced $seed plot_dropsinpackets u create_flowstats flowDump RED
}

# forced drops, byte drop metric, RED
proc test_forced { seed } {
    global category ns_link queuetype
    set queuetype packets
    set ns_link(queue-in-bytes) false
    set category forced
    test forced $seed plot_dropsinbytes f create_flowstats flowDump RED
}

# unforced drops, byte drop metric, RED
proc test_unforced1 { seed } {
    global category ns_link queuetype
    set queuetype packets
    set ns_link(queue-in-bytes) false
    set category unforced
    test unforced1 $seed plot_dropsinbytes u1 create_flowstats flowDump RED
}

# forced drops, packet drop metric, RED
proc test_forced1 { seed } {
    global category ns_link queuetype
    set queuetype packets
    set ns_link(queue-in-bytes) false
    set category forced
    test forced1 $seed plot_dropsinpackets f1 create_flowstats flowDump RED
}

# all drops, combined drop metric, RED
proc test_combined { seed } {
    global ns_link queuetype category
    set queuetype packets
    set ns_link(queue-in-bytes) false
    set category combined
    test combined $seed plot_dropscombined c create_flowstats1 flowDump1 RED
}

# byte drop metric, queue in packets, Drop-Tail
proc test_droptail1 { seed } {
    global queuetype category
    set queuetype packets
    set category forced
    test droptail1 $seed plot_dropsinbytes d1 create_flowstats flowDump DropTail
}

# packet drop metric, queue in packets, Drop-Tail
proc test_droptail2 { seed } {
    global queuetype
    set queuetype packets
    set category forced
    test droptail2 $seed plot_dropsinpackets d2 create_flowstats flowDump DropTail
}

# byte drop metric, queue in bytes, Drop-Tail
proc test_droptail3 { seed } {
    global ns_link queuetype category
    set queuetype bytes
    set ns_link(queue-in-bytes) true
    set ns_link(mean_pktsize) 512
    set category forced
    test droptail3 $seed plot_dropsinbytes d3 create_flowstats flowDump DropTail
}

# packet drop metric, queue in bytes, Drop-Tail
proc test_droptail4 { seed } {
    global ns_link queuetype category
    set queuetype bytes
    set ns_link(queue-in-bytes) true
    set ns_link(mean_pktsize) 512
    set category forced
    test droptail4 $seed plot_dropsinpackets d4 create_flowstats flowDump DropTail
}


if { $argc < 2 } {
        puts stderr {usage: ns $argv [ two ]}
        exit 1
} elseif { $argc == 2 } {
        set testname [lindex $argv 0]
	set seed [lindex $argv 1]
	puts stderr "testname: $testname"
	puts stderr "seed: $seed"
}
if { "[info procs test_$testname]" != "test_$testname" } {
        puts stderr "$testname: no such test: $testname"
}
test_$testname $seed

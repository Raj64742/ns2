source FlowsA.v2.tcl
source Setred.v2.tcl
#
set flowfile fairflow.tr
set flowgraphfile fairflow.xgr
#------------------------------------------------------------------

#
# Create traffic.
#
proc traffic1 {} {
    global s1 s2 r1 r2 s3 s4
    new_tcp 1.0 $s1 $s3 100 1 1 1000
    new_tcp 4.2 $s2 $s4 100 2 0 50
#    new_cbr 18.4 $s1 $s4 190 0.003 3
#    new_tcp 65.4 $s1 $s4 4 4 0 2000
#    new_tcp 100.2 $s3 $s1 8 5 0 1000
    new_tcp 122.6 $s1 $s4 4 6 0 512
    new_tcp 135.0 $s4 $s2 100 7 0 1000
#    new_tcp 162.0 $s2 $s3 100 8 0 1000
#    new_tcp 220.0 $s1 $s3 100 9 0 512
    new_tcp 260.0 $s3 $s2 100 10 0 512
#    new_cbr 310.0 $s2 $s4 190 0.1 11 
    new_tcp 320.0 $s1 $s4 100 12 0 512
#    new_tcp 350.0 $s1 $s3 100 13 0 512
    new_tcp 370.0 $s3 $s2 100 14 0 512
#    new_tcp 390.0 $s2 $s3 100 15 0 512
#    new_tcp 420.0 $s2 $s4 100 16 0 512
#    new_tcp 440.0 $s2 $s4 100 17 0 512
}

#------------------------------------------------------------------
proc create_testnet6 { queuetype }  {
    global ns s1 s2 r1 r2 s3 s4

    set s1 [$ns node]
    set s2 [$ns node]
    set r1 [$ns node]
    set r2 [$ns node]
    set s3 [$ns node]
    set s4 [$ns node]
    
    $ns duplex-link $s1 $r1 10Mb 2ms DropTail
    $ns duplex-link $s2 $r1 10Mb 3ms DropTail
    $ns simplex-link $r1 $r2 1.5Mb 20ms $queuetype
    $ns simplex-link $r2 $r1 1.5Mb 20ms $queuetype
    set redlink [$ns link $r1 $r2]
    [[$ns link $r2 $r1] queue] set limit_ 25
    [[$ns link $r1 $r2] queue] set limit_ 25    
    $ns duplex-link $s3 $r2 10Mb 4ms DropTail
    $ns duplex-link $s4 $r2 10Mb 5ms DropTail
    
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
    set queuetype $queue
    # Set queuesize 25 for Drop Tail, later set to 100 for RED
    #set queuesize 25
    set queuesize 100

    set ns [new Simulator]
    set redlink [create_testnet6 $queuetype]
    [$redlink queue] set limit_ $queuesize
    [[$ns link $r2 $r1] queue] set limit_ $queuesize
    if {$queuetype == "RED"} {
	set_Red $r1 $r2
	[$redlink queue] set limit_ 100
#	new_tcp 50.2 $s1 $s3 100 20 0 1500
#	new_tcp 50.2 $s1 $s3 100 21 0 1500
    }
    $createflows $redlink $dump $stoptime
    traffic1
    new_tcp 50.2 $s1 $s3 100 18 0 1460
    new_tcp 50.5 $s1 $s3 100 19 0 1460

##
    $ns at $stoptime "plot_dropave title"
##
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
#    test combined $seed plot_dropmetric c create_flowstats1 flowDump1 RED
}

# byte drop metric, queue in packets, Drop-Tail
proc test_droptail1 { seed } {
    global queuetype category
    set queuetype packets
    set category forced
    test droptail1 $seed plot_dropsinbytes d1 create_flowstats2 flowDump DropTail
}

# packet drop metric, queue in packets, Drop-Tail
proc test_droptail2 { seed } {
    global queuetype category
    set queuetype packets
    set category forced
    test droptail2 $seed plot_dropsinpackets d2 create_flowstats2 flowDump DropTail
}

# byte drop metric, queue in bytes, Drop-Tail
proc test_droptail3 { seed } {
    global ns_link queuetype category
    set queuetype bytes
    ##
    ## not implemented yet in ns-2
    set ns_link(queue-in-bytes) true
    set ns_link(mean_pktsize) 512
    set category forced
    test droptail3 $seed plot_dropsinbytes d3 create_flowstats2 flowDump DropTail
}

# packet drop metric, queue in bytes, Drop-Tail
proc test_droptail4 { seed } {
    global ns_link queuetype category
    set queuetype bytes
    ##
    ## not implemented yet in ns-2
    set ns_link(queue-in-bytes) true
    set ns_link(mean_pktsize) 512
    set category forced
    test droptail4 $seed plot_dropsinpackets d4 create_flowstats2 flowDump DropTail
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

source Flows.tcl
source Setred.tcl
source flowmon.tcl
source mechanisms.tcl
#
set flowfile Reclass2.tr
set graphfile Reclass2.xgr
set penaltyfile Reclass2a.tr
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
#    new_cbr 18.4 $s1 $s4 190 0.00003 3
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

#------------------------------------------------------------------

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
proc create_topology { tf af }  {
	global ns
	global s1 s2 s3 s4
	global r1 r2
	global bandwidth

	set s1 [$ns node]
	set s2 [$ns node]
	set s3 [$ns node]
	set s4 [$ns node]

	set r1 [$ns node]
	set r2 [$ns node]

	$ns duplex-link $s1 $r1 10Mb 2ms DropTail
	$ns duplex-link $s2 $r1 10Mb 3ms DropTail
	set cl [new Classifier/Hash/SrcDestFid 33]
	$ns simplex-link $r1 $r2 1.5Mb 20ms "CBQ $cl"
	set cbqlink [$ns link $r1 $r2]
	[$cbqlink queue] algorithm "formal"
	$ns simplex-link $r2 $r1 1.5Mb 20ms DropTail
	set bandwidth 1500
	[[$ns link $r2 $r1] queue] set limit_ 25
	$ns duplex-link $s3 $r2 10Mb 4ms DropTail
	$ns duplex-link $s4 $r2 10Mb 5ms DropTail

	$cbqlink trace $ns $tf	; # trace pkts
	[$ns link $r2 $r1] trace $ns $af ; # trace acks
	return $cbqlink
}

#
# prints "time: $time class: $class bytes: $bytes" for the link.
#
proc linkDumpFlows { link interval stoptime } {
        global flowfile ns
        set f [open $flowfile w]
        proc dump1 { file link interval } {
                global maxclass
                $ns at [expr [$ns now] + $interval] "dump1 $file $link $interval
"
                for {set i 0} {$i <= $maxclass} {incr i 1} {
                  set bytes [$link stat $i bytes]
                  if {$bytes > 0} {
                    puts $file "time: [$ns now] class: $i bytes: $bytes $interval"     
                  }
                }
        }
        $ns at $interval "dump1 $f $link $interval"
        $ns at $stoptime "close $f"
}

proc finish { tf ff } {
	global ns
	close $tf
	close $ff
	$ns instvar scheduler_
	$scheduler_ halt
	puts "simulation complete"
}

proc test {testname seed label} {
    global ns s1 s2 r1 r2 s3 s4 r1fm flowgraphfile bandwidth
    global flowfile graphfile
   
    # set stoptime 300.0
    set stoptime 100.0
    # Set queuesize 25 for Drop Tail, later set to 100 for RED
    set queuesize 25

    set ns [new Simulator]

        set rtt 0.06
	set mtu 512

	set tracef [open out.tr w]
	set ackf [open ack.tr w]
	set cbqlink [create_topology $tracef $ackf]

	set rtm [new RTMechanisms $ns $cbqlink $rtt $mtu]

	set gfm [$rtm makeflowmon]
	set gflowf [open gflow.tr w]
	$gfm set enable_in_ false	; # no per-flow arrival state
	$gfm set enable_out_ false	; # no per-flow departure state
	$gfm attach $gflowf

	set bfm [$rtm makeflowmon]
	set bflowf [open bflow.tr w]
	$bfm attach $bflowf

	$rtm makeboxes $gfm $bfm 100 1000
	$rtm bindboxes

    set L1 [$ns link $r1 $r2]
    linkDumpFlows $L1 1.0 $stoptime

    new_tcp 50.2 $s1 $s3 100 20 0 1500
    new_tcp 50.2 $s1 $s3 100 21 0 1500
#    $createflows $redlink $dump $stoptime
    traffic1
    new_tcp 50.2 $s1 $s3 100 18 0 1460
    new_tcp 50.5 $s1 $s3 100 19 0 1460

    $ns at $stoptime "plot_bytes NAME $flowfile $graphfile $bandwidth"
    $ns at $stoptime "finish $gflowf $bflowf"

    # trace only the bottleneck link
    #    [$ns link $r1 $r2] trace [openTrace $stoptime $testname]

    ns-random $seed

    $ns run

}

#------------------------------------------------------------------


test temp 24243 label


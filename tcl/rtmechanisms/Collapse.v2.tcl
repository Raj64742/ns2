source Setred.v2.tcl
#set packetsize 512
set packetsize 1500

# stats: total good packets received by sources.
# knob: arrival rate of CBR flow 
proc create_flowstats { redlink stoptime } {
    global ns r1 r2 r1fm flowfile
    
    set flowfile data.f
    set r1fm [$ns makeflowmon Fid]
    set flowdesc [open $flowfile w]
    $r1fm attach $flowdesc
    $ns attach-fmon $redlink $r1fm; # don't use edrop stats
    $ns at $stoptime "$r1fm dump; close $flowdesc"
}


#
# data.f:
# time fid c=forced/unforced type class src dest pktA byteA CpktD CbyteD
#    TpktA TbyteA TCpktD TCbyteD TpktD TbyteD
# A:arrivals D:drops C:category(forced/unforced) T:totals 
#
# print: class # arriving_packets # dropped_packets # 
#
proc finish_flowstats { infile outfile cbrs tcps } {
    set awkCode {
	BEGIN {
	    arrivals=0;
	    drops=0;
	    prev=-1;
	}
	{
	    if (prev == -1) {
		arrivals += $8;
                drops += $18;
		prev = $2;
	    }
	    else if ($2 == prev) {
                arrivals += $8;
                drops += $18;
	    }
	    else {
                printf "class %d arriving_pkts %d dropped_pkts %d\n", prev, arrivals, drops;
                prev = $2;
                arrivals = $8;
                drops = $18;
            }
 	}
	END {
	    printf "class %d arriving_pkts %d dropped_pkts %d\n", prev, arrivals, drops;
	}
    }
    puts $outfile "cbrs: $cbrs tcps: $tcps"
    exec awk $awkCode $infile >@ $outfile
}

proc printTcpPkts { tcp fid file } {
    puts $file "fid $fid total_packets_acked [$tcp set ack_]"
}

proc printCbrPkts { cbrsnk fid file } {
    puts $file "fid $fid total_packets_received [$cbrsnk set npkts_]"
}

proc printstop { stoptime file } {
    puts $file "stop-time $stoptime"
}

# Creates connection. First creates a source agent of type s_type and binds
# it to source.  Next creates a destination agent of type d_type and binds
# it to dest.  Finally creates bindings for the source and destination agents,
# connects them, and  returns a list of source agent and destination agent.
proc create-connection-list {s_type source d_type dest pktClass} {
    global ns
    set s_agent [new Agent/$s_type]
    set d_agent [new Agent/$d_type]
    $s_agent set fid_ $pktClass
    $d_agent set fid_ $pktClass
    $ns attach-agent $source $s_agent
    $ns attach-agent $dest $d_agent
    $ns connect $s_agent $d_agent
    
    return [list $s_agent $d_agent]
}

#
# create and schedule a cbr source/dst 
#
proc new_cbr { startTime source dest pktSize fid dump interval file stoptime } {
    global ns
    set cbrboth [create-connection-list CBR $source LossMonitor $dest $fid ]
    set cbr [lindex $cbrboth 0]
    $cbr set packetSize_ $pktSize
    $cbr set interval_ $interval
    set cbrsnk [lindex $cbrboth 1]
    $ns at $startTime "$cbr start"
    if {$dump == 1 } {
	puts $file "fid $fid packet-size $pktSize"
	$ns at $stoptime "printCbrPkts $cbrsnk $fid $file"
    }
}

#
# create and schedule a tcp source/dst
#
proc new_tcp { startTime source dest window fid dump size file stoptime } {
    global ns
    set tcp [$ns create-connection TCP/Sack1 $source TCPSink/Sack1/DelAck $dest $fid ]
    $tcp set window_ $window
#   $tcp set tcpTick_ 0.1
    $tcp set tcpTick_ 0.01
    if {$size > 0}  {$tcp set packetSize_ $size }
    set ftp [$tcp attach-source FTP]
    $ns at $startTime "$ftp start"
    $ns at $stoptime "printTcpPkts $tcp $fid $file"
    if {$dump == 1 } {puts $file "fid $fid packet-size [$tcp set packetSize_]"}
}

proc make_queue { cl qt qlim } {
        set q [new Queue/$qt]
        $q set limit_ $qlim
        $cl install-queue $q
}

proc create_flat { link qtype qlim number} {

        set topclass_ [new CBQClass]
        # (topclass_ doesn't have a queue)
        $topclass_ setparams none 0 0.98 auto 8 2 0
	$link insert $topclass_
	set share [expr 100. / $number ]
        for {set i 0} {$i < $number} {incr i 1} {
		set cls [new CBQClass]
                $cls setparams $topclass_ true .$share auto 1 1 0
		make_queue $cls $qtype $qlim
                $link insert $cls
                $link bind $cls $i
        }
} 


#
# Create a simple six node topology:
#
proc create_testnet5 { queuetype bandwidth } {
    global ns s1 s2 r1 r2 s3 s4
    set s1 [$ns node]
    set s2 [$ns node]
    set r1 [$ns node]
    set r2 [$ns node]
    set s3 [$ns node]
    set s4 [$ns node]
    
    $ns duplex-link $s1 $r1 10Mb 2ms DropTail
    $ns duplex-link $s2 $r1 10Mb 3ms DropTail
    $ns duplex-link $s3 $r2 10Mb 10ms DropTail
    $ns duplex-link $s4 $r2 $bandwidth 5ms DropTail

    $ns simplex-link $r1 $r2 1.5Mb 3ms $queuetype
    $ns simplex-link $r2 $r1 1.5Mb 3ms DropTail
    set redlink [$ns link $r1 $r2]
    [[$ns link $r2 $r1] queue] set limit_ 100
    [[$ns link $r1 $r2] queue] set limit_ 100
    return $redlink
}

proc finish_ns {f} {
    global ns
    $ns instvar scheduler_
    $scheduler_ halt
    close $f
    puts "simulation complete"
}

proc test_simple { interval bandwidth datafile scheduling cbrs tcps } {
    global ns s1 s2 r1 r2 s3 s4 flowfile packetsize
    set testname simple
    set stoptime 100.1
    set printtime 100.0
    set qtype RED
    set qlim 100
    
    set ns [new Simulator]
    
    if { $scheduling == "wrr" } {
	    set xlink [create_testnet5 CBQ/WRR $bandwidth]
#	    create_flat $xlink $qtype $qlim [expr $cbrs + $tcps]
	    create_flat $xlink DropTail $qlim [expr $cbrs + $tcps]
    } elseif { $scheduling == "fifo" } {
	    set xlink [create_testnet5 RED $bandwidth]
	    set_Red_Oneway $r1 $r2
    }
 
    create_flowstats $xlink $printtime
    set f [open $datafile w]
    
    $ns at $stoptime "printstop $printtime $f"
    for {set i 0} {$i < $cbrs} {incr i 1} {
      new_cbr 1.4 $s2 $s4 100 $i 1 $interval $f $printtime
    }
    for {set i $cbrs} {$i < $cbrs + $tcps} {incr i 1} {
       new_tcp 0.0 $s1 $s3 100 $i 1 $packetsize $f $printtime
    }
    $ns at $stoptime "finish_flowstats $flowfile $f $cbrs $tcps"
    $ns at $stoptime "finish_ns $f"
    puts seed=[ns-random 0]
    $ns run
}

#
# This is the main program: 
#

if { $argc < 2 || $argc > 6} {
    puts stderr {usage: ns $argv [ arguments ]}
    exit 1
} elseif { $argc == 2 } {
    set testname [lindex $argv 0]
    set interval [lindex $argv 1] 
    set bandwidth 128Kb
    set datafile Collapse.tr
    puts "interval: $interval"
} elseif { $argc == 3 } {
    set testname [lindex $argv 0]
    set interval [lindex $argv 1] 
    set bandwidth [lindex $argv 2]
    set datafile Fairness.tr
    puts "interval: $interval"
} elseif { $argc == 6 } {
    set testname [lindex $argv 0]
    set interval [lindex $argv 1]
    set bandwidth [lindex $argv 2]
    set scheduling [lindex $argv 3]
    set cbrs [lindex $argv 4]
    set tcps [lindex $argv 5]
    set datafile temp.tr
    puts "interval: $interval" 
}
if { "[info procs test_$testname]" != "test_$testname" } {
    puts stderr "$testname: no such test: $testname"
}
test_$testname $interval $bandwidth $datafile $scheduling $cbrs $tcps

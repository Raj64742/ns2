#
# Copyright (c) 1998 University of Southern California.
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 

# To run all tests: test-all-xcp
# to run individual test:
# ns test-suite-xcp.tcl simple-xcp

# To view a list of available test to run with this script:
# ns test-suite-xcp.tcl

# This test-suite validate xcp congestion control scenarios along with xcp-tcp mixed flows through routers.

if {![TclObject is-class Agent/TCP/XCP]} {
	puts "xcp module is not present; validation skipped"
	exit 2
}


Class TestSuite

proc usage {} {
    global argv0
    puts stderr "usage: ns $argv0 <tests> "
    puts "Valid Tests: simple-xcp xcp-tcp"
    exit 1
}


TestSuite instproc init {} {
    
    $self instvar ns_ rtg_ tracefd_ qType_ qSize_ BW_ delay_ \
	tracedFlows_
    set ns_ [new Simulator]

    set rtg_ [new RNG]
    $rtg_ seed 47294
}

TestSuite instproc create-Bottleneck {} {
    global R0 R1 Bottleneck rBottleneck l rl all_links
    $self instvar ns_ qType_ qSize_ BW_ delay_
    
    # create bottleneck nodes
    set R0 [$ns_ node]
    set R1 [$ns_ node]
    
    $ns_ duplex-link $R0 $R1 [set BW_]Mb [set delay_]ms $qType_
    $ns_ queue-limit $R0 $R1 $qSize_
    $ns_ queue-limit $R1 $R0 $qSize_

    # Give a global handle to the Bottleneck Queue to allow 
    # setting the RED paramters

    set  Bottleneck  [[$ns_ link $R0 $R1] queue] 
    set  rBottleneck [[$ns_ link $R1 $R0] queue]

    set  l  [$ns_ link $R0 $R1]  
    set  rl [$ns_ link $R1 $R0]

    set all_links "$l $rl "
}

TestSuite instproc create-sidelinks {numSideLinks deltaDelay} {
    global R0 R1 all_links n
    $self instvar ns_ BW_ delay_ qSize_ qType_
    
    set i 0
    while { $i < $numSideLinks } {
	#global n$i q$i rq$i l$i rl$i
	set n($i)  [$ns_ node]
	$ns_ duplex-link $n($i) $R0 [set BW_]Mb [expr $delay_ + $i * $deltaDelay]ms $qType_
	$ns_ queue-limit $n($i)  $R0  $qSize_
	$ns_ queue-limit $R0 $n($i)   $qSize_
	set  q$i   [[$ns_ link $n($i)  $R0] queue] 
	set  rq$i  [[$ns_ link $R0 $n($i)] queue]
	set  l$i    [$ns_ link $n($i)  $R0] 
	set  rl$i   [$ns_ link $R0 $n($i)]
	set all_links "$all_links [set l$i] [set rl$i] "
	incr i
    }

    set i 30
    while { $i < [expr 30+$numSideLinks] } {
	#global n$i q$i rq$i l$i rl$i
	set n($i)  [$ns_ node]
	$ns_ duplex-link $n($i) $R1 [set BW_]Mb [expr $delay_ + $i * $deltaDelay]ms $qType_
	$ns_ queue-limit [set n($i)]  $R1  $qSize_
	$ns_ queue-limit $R1 [set n($i)]   $qSize_
	set  q$i   [[$ns_ link [set n($i)]  $R1] queue] 
	set  rq$i  [[$ns_ link $R1 [set n($i)]] queue]
	set  l$i    [$ns_ link [set n($i)]  $R1] 
	set  rl$i   [$ns_ link $R1 [set n($i)]]
	set all_links "$all_links [set l$i] [set rl$i] "
	incr i
    }
}

TestSuite instproc post-process {what PlotTime} {
    global quiet
    $self instvar tracedFlows_
    exec rm -f temp.rands
    set f [open temp.rands w]
    
    foreach i $tracedFlows_ {
	exec rm -f temp.c 
        exec touch temp.c
	set result [exec awk -v PlotTime=$PlotTime -v what=$what {
	    {
		if (($6 == what) && ($1 > PlotTime)) {
		    print $1, $7 >> "temp.c";
		}
	    }
	} xcp$i.tr ]
	
	puts $f \"$what$i
	exec cat temp.c >@ $f
	puts $f "\n"
	flush $f
    }
    close $f
    if {$quiet == 0} {
	exec xgraph -nl -m -x time -y seqno temp.rands &
    }
}

TestSuite instproc finish {} {
    $self instvar ns_ tracefd_ tracedFlows_ 
    if [info exists tracedFlows_] {
	foreach i $tracedFlows_ {
	    global src$i
	    set file [set src$i tcpTrace_]
	    if {[info exists $file]} {
		flush [set $file]
		close [set $file]
	    }   
	}
    }

    if {[info exists tracefd_]} {
	flush $tracefd_
	close $tracefd_
    }
    $ns_ halt
}

Class GeneralSender  -superclass Agent 

#   otherparams are "startTime TCPclass .."
GeneralSender instproc init { ns id srcnode dstnode otherparams } {
    
    $self next
    $self instvar tcp_ id_ ftp_ snode_ dnode_ tcp_rcvr_ tcp_type_
    set id_ $id
    
    if { [llength $otherparams] > 1 } {
	set TCP [lindex $otherparams 1]
    } else { 
	set TCP "TCP/Reno"
    }
    if [string match {TCP/XCP*} $TCP] {
	set TCPSINK "XCPSink"
    } else {
	set TCPSINK "TCPSink"
    }
    
    if { [llength $otherparams] > 2 } {
	set traffic_type [lindex $otherparams 2]
    } else {
	set traffic_type FTP
    }
    
    set   tcp_type_ $TCP
    set   tcp_ [new Agent/$TCP]
    set   tcp_rcvr_ [new Agent/$TCPSINK]
    $tcp_ set  packetSize_ 1000   
    $tcp_ set  class_  $id

    switch -exact $traffic_type {
	FTP {
	    set traf_ [new Application/FTP]
	    $traf_ attach-agent $tcp_
	}
	EXP {
	    
	    set traf_ [new Application/Traffic/Exponential]
	    $traf_ set packetSize_ 1000
	    $traf_ set burst_time_ 250ms
	    $traf_ set idle_time_ 250ms
	    $traf_ set rate_ 10Mb
	    $traf_ attach-agent $tcp_
	}
	default {
	    puts "unsupported traffic\n"
	    exit 1
	}
    }
    $ns   attach-agent $srcnode $tcp_
    $ns   attach-agent $dstnode $tcp_rcvr_
    $ns   connect $tcp_  $tcp_rcvr_
    set   startTime [lindex $otherparams 0]
    $ns   at $startTime "$traf_ start"
    
    puts  "initialized Sender $id_ at $startTime"
}


GeneralSender instproc trace-xcp parameters {
    $self instvar tcp_ id_ tcpTrace_
    global ftracetcp$id_ 
    set ftracetcp$id_ [open  xcp$id_.tr  w]
    set tcpTrace_ [set ftracetcp$id_]
    $tcp_ attach-trace $tcpTrace_
    if { -1 < [lsearch $parameters cwnd]  } { $tcp_ tracevar cwnd_ }
    if { -1 < [lsearch $parameters seqno] } { $tcp_ tracevar t_seqno_ }
    if { -1 < [lsearch $parameters ackno] } { $tcp_ tracevar ack_ }
    if { -1 < [lsearch $parameters rtt]  } { $tcp_ tracevar rtt_ }
    if { -1 < [lsearch $parameters ssthresh]  } { $tcp_ tracevar ssthresh_ }
}


Class Test/simple-xcp -superclass TestSuite

Test/simple-xcp instproc init {} {
    $self instvar ns_ testName_ qType_ qSize_ BW_ delay_ nXCPs_ \
	SimStopTime_ tracefd_

    set tracefd_ [open temp.rands w]
    set testName_ simple-xcp
    set qType_  XCP
    set BW_     20; # in Mb/s
    set delay_  10; # in ms
    set  qSize_  [expr round([expr ($BW_ / 8.0) * 4 * $delay_ * 1.0])];#set buffer to the pipe size
    set SimStopTime_      30
    set nXCPs_            3
    
    $self next
}

Test/simple-xcp instproc run {} {
    global n all_links Bottleneck
    $self instvar ns_ tracefd_ SimStopTime_ nXCPs_ qSize_ delay_ rtg_

    set numsidelinks 3
    set deltadelay 0.0
    
    $self create-Bottleneck
    $self create-sidelinks $numsidelinks $deltadelay

    foreach link $all_links {
	set queue [$link queue]
	if {[$queue info class] == "Queue/XCP"} {
	    $queue set-link-capacity-Kbytes [expr [[$link set link_] set bandwidth_] / 8000];  
	}
    }

    # Create sources:
    set i 0
    while { $i < $nXCPs_  } {
	set StartTime [expr [$rtg_ integer 1000] * 0.001 * (0.01 * $delay_) + $i  * 0.0] 
	set src$i [new GeneralSender $ns_ $i [set n($i)] [set n([expr 30+$i])] "$StartTime TCP/XCP"]
    [[set src$i] set tcp_]  set  packetSize_ [expr 100 * ($i +10)]
    [[set src$i] set tcp_]  set  window_     [expr $qSize_]
    incr i
    }
    
    # trace bottleneck queue
    foreach queue_name "Bottleneck" {
	set queue [set "$queue_name"]
	if {[$queue info class] == "Queue/XCP"} {
	    $queue attach $tracefd_
	} else {
	    # do nothing for droptails
	}
    }
    $ns_ at $SimStopTime_ "$self finish"
    $ns_ run
}

Class Test/xcp-tcp -superclass TestSuite

Test/xcp-tcp instproc init {} {
    $self instvar ns_ testName_ qType_ qSize_ BW_ delay_ nXCPs_ \
	SimStopTime_ tracedFlows_
    
    set testName_ xcp-tcp
    set qType_  XCP
    set BW_     20; # in Mb/s
    set delay_  10; # in ms
    set  qSize_  [expr round([expr ($BW_ / 8.0) * 4 * $delay_ * 1.0])];#set buffer to the pipe size
    set SimStopTime_      30
    set nXCPs_            3
    set tracedFlows_       "0 1 2 3"
    
    $self next
}

Test/xcp-tcp instproc run {} {
    global src n all_links Bottleneck
    $self instvar ns_ tracefd_ SimStopTime_ nXCPs_ qSize_ delay_ rtg_ \
	tracedFlows_

    set numsidelinks 4
    set deltadelay 0.0
    
    $self create-Bottleneck
    $self create-sidelinks $numsidelinks $deltadelay

    foreach link $all_links {
	set queue [$link queue]
	if {[$queue info class] == "Queue/XCP"} {
	    $queue set-link-capacity-Kbytes [expr [[$link set link_] set bandwidth_] / 8000];  
	}
    }

    # Create sources:
    set i 0
    while { $i < $nXCPs_  } {
	set StartTime [expr [$rtg_ integer 1000] * 0.001 * (0.01 * $delay_) + $i  * 0.0] 
	set src$i [new GeneralSender $ns_ $i [set n($i)] [set n([expr 30+$i])] "$StartTime TCP/XCP"]
    [[set src$i] set tcp_]  set  packetSize_ [expr 100 * ($i +10)]
    [[set src$i] set tcp_]  set  window_     [expr $qSize_]
    incr i
    }
    
    set StartTime [expr [$rtg_ integer 1000] * 0.001 * (0.01 * $delay_) + $i  * 0.0] 
    set src$i [new GeneralSender $ns_ $i [set n($i)] [set n([expr 30+$i])] "$StartTime TCP"]
    [[set src$i] set tcp_]  set  packetSize_ [expr 100 * ($i +10)]
    [[set src$i] set tcp_]  set  window_     [expr $qSize_]
    
    
    # trace sources
    foreach i $tracedFlows_ {
	[set src$i] trace-xcp "cwnd seqno"
    }
    
    $ns_ at $SimStopTime_ "$self finish"
    $ns_ run

    #$self post-process cwnd_ 0.0
    $self post-process t_seqno_ 0.0
}

proc runtest {arg} {
    global quiet
    set quiet 0
    
    set b [llength $arg]
    if {$b == 1} {
	set test $arg
    } elseif {$b == 2} {
	set test [lindex $arg 0]
	if {[lindex $arg 1] == "QUIET"} {
	    set quiet 1
	}
    } else {
	usage
    }
    set t [new Test/$test]
    $t run
}

global argv arg0
runtest $argv














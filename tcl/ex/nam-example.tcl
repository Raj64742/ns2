#
# example of new ns support for nam trace, adapted from Kannan's srm2.tcl
#
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/ex/nam-example.tcl,v 1.6 1998/03/07 00:07:26 haoboy Exp $
#

if [string match {*.tcl} $argv0] {
	set prog [string range $argv0 0 [expr [string length $argv0] - 5]]
} else {
	set prog $argv0
}

if {[llength $argv] > 0} {
	set srmSimType [lindex $argv 0]
} else {
	set srmSimType Adaptive
}

#source ../mcast/srm-nam.tcl		;# to separate control messages.
#source ../mcast/srm-debug.tcl		;# to trace delay compute fcn. details.
ns-random 1
Simulator set NumberInterfaces_ 1
Simulator set EnableMcast_ 1
set ns [new Simulator]

#$ns trace-all [open out.tr w]
$ns namtrace-all [open out.nam w]
set srmStats [open srm-stats.tr w]

# define color index
$ns color 0 red
$ns color 1 blue
$ns color 2 chocolate
$ns color 3 red
$ns color 4 brown
$ns color 5 tan
$ns color 6 gold
$ns color 7 black

# create node with given shape and color
set n(0) [$ns node]
$n(0) color "black" 
$n(0) shape "circle"
set n(1) [$ns node]
$n(1) color "blue" 
$n(1) shape "circle"
set n(2) [$ns node]
$n(2) shape "circle" 
$n(2) color "chocolate"
set n(3) [$ns node]
$n(3) shape "circle" 
$n(3) color "gold"
set n(4) [$ns node]
$n(4) shape "circle" 
$n(4) color "tan"
set n(5) [$ns node]
$n(5) shape "circle" 
$n(5) color "red"

# create links and layout
$ns duplex-link $n(0) $n(1) 1.5Mb 10ms DropTail
$ns duplex-link-op $n(0) $n(1) orient right
$ns duplex-link $n(1) $n(2) 1.5Mb 10ms DropTail
$ns duplex-link-op $n(1) $n(2) orient right
$ns duplex-link $n(2) $n(3) 1.5Mb 10ms DropTail
$ns duplex-link-op $n(2) $n(3) orient right
$ns duplex-link $n(3) $n(4) 1.5Mb 10ms DropTail
$ns duplex-link-op $n(3) $n(4) orient right-up
$ns duplex-link $n(3) $n(5) 1.5Mb 10ms DropTail
$ns duplex-link-op $n(3) $n(5) orient right-down

$ns queue-limit $n(0) $n(1) 2	;# q-limit is 1 more than max #packets in q.
$ns queue-limit $n(1) $n(0) 2

# set routing
set group 0x8000
#XXX do *NOT* use DM here, it won't work with rtmodel!
#set mh [$ns mrtproto DM {}]
#$ns at 0.0 "$ns run-mcast"
set cmc [$ns mrtproto CtrMcast {}]
$ns at 0.3 "$cmc switch-treetype $group"

# set group members
set fid  0
for {set i 0} {$i <= 5} {incr i} {
	set srm($i) [new Agent/SRM/$srmSimType]
	$srm($i) set dst_ $group
	$srm($i) set fid_ [incr fid]
	$srm($i) trace $srmStats
	$ns at 1.0 "$srm($i) start"

	$ns attach-agent $n($i) $srm($i)
	$ns add-agent-trace $srm($i) srm($i)
	$ns monitor-agent-trace $srm($i) ;# turn nam monitor on from the start
	$srm($i) tracevar C1_
	$srm($i) tracevar C2_
}

# set traffic source
set packetSize 800
set s [new Agent/CBR]
$s set interval_ 0.02
$s set packetSize_ $packetSize
$s set fid_ 0
$srm(0) traffic-source $s
$srm(0) set packetSize_ $packetSize
$ns at 3.5 "$srm(0) start-source"

$ns at 3.5 "$n(0) color red"
$ns at 3.5 "$ns trace-annotate \"node 0 changed color\""

# add/remove node marks (concentric circles around nodes)
$ns at 4.0 "$n(0) add-mark m1 blue circle"
$ns at 4.0 "$ns trace-annotate \"node 0 added one mark\""
$ns at 4.5 "$n(0) add-mark m2 purple hexagon"
$ns at 4.5 "$ns trace-annotate \"node 0 added second mark\""

$ns at 5.0 "$n(0) delete-mark m1"
$ns at 5.0 "$ns trace-annotate \"node 0 deleted one mark\""
$ns at 5.5 "$n(0) delete-mark m2"
$ns at 5.5 "$ns trace-annotate \"node 0 deleted second mark\""

# Fake a dropped packet by incrementing seqno.
#$ns rtmodel-at 3.519 down $n(0) $n(1)	;# this ought to drop exactly one
#$ns rtmodel-at 3.621 up   $n(0) $n(1)	;# data packet?
$ns rtmodel Deterministic {2.61 0.98 0.02} $n(0) $n(1)

$ns at 3.7 "$n(0) color cyan"
$ns at 3.7 "$ns trace-annotate \"node 0 changed color again\""
$ns at 10.0 "$ns trace-annotate \"simu finished\""
$ns at 10.0 "finish"

proc finish {} {
    global s srm
    $s stop

    global ns srmStats
    $ns flush-trace		;# NB>  Did not really close out.tr...:-)
    close $srmStats

    global argv0
    if [string match {*.tcl} $argv0] {
	set prog [string range $argv0 0 [expr [string length $argv0] - 5]]
    } else {
	set prog $argv0
    }

    for {set i 0} {$i <= 5} {incr i} {
	    $ns delete-agent-trace $srm($i)
    }

    puts "running nam..."
    exec nam -f dynamic-nam.conf out.nam &
    exit 0
}

$ns at 0.0 "$ns set-animation-rate 0.1ms"
$ns run

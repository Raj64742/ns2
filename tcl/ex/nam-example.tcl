#
# example of new ns support for nam trace, adapted from Kannan's srm2.tcl
#
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/ex/nam-example.tcl,v 1.1 1997/09/12 01:32:32 haoboy Exp $
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

source ../mcast/srm-nam.tcl		;# to separate control messages.
#source ../mcast/srm-debug.tcl		;# to trace delay compute fcn. details.
ns-random 1
Simulator set NumberInterfaces_ 1
set ns [new MultiSim]

$ns trace-all [open out.tr w]
$ns namtrace-all [open nam.tr w]
set srmStats [open srm-stats.tr w]

# define color index
$ns color 0 red
$ns color 1 blue
$ns color 2 chocolate
$ns color 3 red
$ns color 4 brown
$ns color 5 tan
$ns color 6 gold

# create node with given shape and color
set n(0) [$ns node circle red]
set n(1) [$ns node circle blue]
set n(2) [$ns node circle chocolate]
set n(3) [$ns node circle gold]
set n(4) [$ns node circle tan]
set n(5) [$ns node circle red]

# create links and layout
$ns duplex-link $n(0) $n(1) 1.5Mb 10ms DropTail right
$ns duplex-link $n(1) $n(2) 1.5Mb 10ms DropTail right
$ns duplex-link $n(2) $n(3) 1.5Mb 10ms DropTail right
$ns duplex-link $n(3) $n(4) 1.5Mb 10ms DropTail right-up
$ns duplex-link $n(3) $n(5) 1.5Mb 10ms DropTail right-down

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
    $srm($i) add-agent-trace srm($i)
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

$ns at 3.5 "$n(0) change-color red"

# Fake a dropped packet by incrementing seqno.
#$ns rtmodel-at 3.519 down $n(0) $n(1)	;# this ought to drop exactly one
#$ns rtmodel-at 3.621 up   $n(0) $n(1)	;# data packet?
$ns rtmodel Deterministic {2.61 0.98 0.02} $n(0) $n(1)

$ns at 3.7 "$n(0) change-color cyan"
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
	    $srm($i) delete-agent-trace
    }

#    puts "converting output to nam format..."
#    exec awk -f nstonam.awk out.tr > $prog-nam.tr 
#    exec rm -f out
    #XXX
#    puts "running nam..."
#    exec nam $prog-nam &
    exit 0
}

$ns run

#
# simple 8 node star topology, runs for 50s, tests Adaptive timers.
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

Simulator set NumberInterfaces_ 1
set ns [new MultiSim]
$ns trace-all [open out.tr w]

# make the nodes
set nmax 8
for {set i 0} {$i <= $nmax} {incr i} {
	set n($i) [$ns node]
}

# now the links
for {set i 1} {$i <= $nmax} {incr i} {
	$ns duplex-link $n($i) $n(0) 1.5Mb 10ms DropTail
}

# configure multicast
set group 0x8000
set cmc [$ns mrtproto CtrMcast {}]
$ns at 0.3 "$cmc switch-treetype $group"
#$ns mrtproto DM {}

# now the agents
set srmStats [open srmStats.tr w]
set srmEvents [open srmEvents.tr w]

set fid 0
for {set i 1} {$i <= $nmax} {incr i} {
	set srm($i) [new Agent/SRM/$srmSimType]
	$srm($i) set dst_ $group
	$srm($i) set fid_ [incr fid]
	$srm($i) log $srmStats
	$srm($i) trace $srmEvents
	$ns at 1.0 "$srm($i) start"

	$ns attach-agent $n($i) $srm($i)
}

#$srm(4) set C1_ 0.5
#$srm(4) set C2_ 1.0

# And, finally, attach a data source to srm(1)
set packetSize 800
set s [new Agent/CBR]
$s set interval_ 0.02
$s set packetSize_ $packetSize
$s set fid_ 0
$srm(1) traffic-source $s
$srm(1) set packetSize_ $packetSize
$ns at 3.5 "$srm(1) start-source"

# Drop a packet every 0.5 secs. starting at 3.52s.
# Drops packets from the source so all receivers see the loss.
$ns rtmodel Deterministic {3.021 0.498 0.002} $n(0) $n(1)

$ns at 50 "finish $s"

proc finish src {
	$src stop

	global ns srmStats srmEvents
	$ns flush-trace		;# NB>  Did not really close out.tr...:-)
	close $srmStats
	close $srmEvents
	exit 0
}

set ave [open ave.tr w]
proc doDump {now i tag} {
	global srm ave
	puts $ave [list $now $i					\
			[$srm($i) set stats_(ave-dup-${tag})]	\
			[$srm($i) set stats_(ave-${tag}-delay)]]
}
proc dumparams intvl {
	global ns nmax
	set now [$ns now]

	doDump $now 1 rep
	for {set i 2} { $i <= $nmax} {incr i} {
		doDump $now $i req
	}
	$ns at [expr $now + $intvl] "dumparams $intvl"
}
$ns at 4.0 "dumparams 0.5"

$ns run

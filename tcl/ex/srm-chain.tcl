if [string match {*.tcl} $argv0] {
	set prog [string range $argv0 0 [expr [string length $argv0] - 5]]
} else {
	set prog $argv0
}

if {[llength $argv] > 0} {
	set srmSimType [lindex $argv 0]
} else {
	set srmSimType Deterministic
}

source ../mcast/srm-nam.tcl		;# to separate control messages.
#source ../mcast/srm-debug.tcl		;# to trace delay compute fcn. details.

Simulator set NumberInterfaces_ 1
set ns [new MultiSim]
$ns trace-all [open out.tr w]

# make the nodes
set nmax 5
for {set i 0} {$i <= $nmax} {incr i} {
	set n($i) [$ns node]
}

# now the links
set chainMax [expr $nmax - 1]
set j 0
for {set i 1} {$i <= $chainMax} {incr i} {
	$ns duplex-link $n($i) $n($j) 1.5Mb 10ms DropTail
	set j $i
}
$ns duplex-link $n([expr $nmax - 2]) $n($nmax) 1.5Mb 10mx DropTail

$ns queue-limit $n(0) $n(1) 2	;# q-limit is 1 more than max #packets in q.
$ns queue-limit $n(1) $n(0) 2

set group 0x8000
#set mh [$ns mrtproto CtrMcast {}]
#$ns at 0.3 "$mh switch-treetype $group"
set mh [$ns mrtproto DM {}]

# now the multicast, and the agents
set srmStats [open srmStats.tr w]
set srmEvents [open srmEvents.tr w]

set fid 0
for {set i 0} {$i <= 5} {incr i} {
	set srm($i) [new Agent/SRM/$srmSimType]
	$srm($i) set dst_ $group
	$srm($i) set fid_ [incr fid]
	#$srm($i) trace $srmEvents
	#$srm($i) log $srmStats
	$ns at 1.0 "$srm($i) start"
	$ns attach-agent $n($i) $srm($i)
}

# Attach a data source to srm(1)
set packetSize 800
set s [new Agent/CBR]
$s set interval_ 0.02
$s set packetSize_ $packetSize
$srm(0) traffic-source $s
$srm(0) set packetSize_ $packetSize
$s set fid_ 0
$ns at 3.5 "$srm(0) start-source"

$ns rtmodel-at 3.519 down $n(0) $n(1)	;# this ought to drop exactly one
$ns rtmodel-at 3.521 up   $n(0) $n(1)	;# data packet?

proc distDump {} {
	global srm
	foreach i [array names srm] {
	    puts "distances [$srm($i) distances?]"
	}
	puts "---"
}
$ns at 3.3 "distDump"

proc finish src {
	$src stop

	global prog ns srmStats srmEvents
	$ns flush-trace		;# NB>  Did not really close out.tr...:-)
	close $srmStats
	close $srmEvents

	puts "converting output to nam format..."
	exec awk -f ../nam-demo/nstonam.awk out.tr > $prog-nam.tr 
	#XXX
	puts "running nam..."
	exec nam $prog-nam &
	exit 0
}
$ns at 4.0 "finish $s"
$ns run

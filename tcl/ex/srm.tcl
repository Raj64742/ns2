if [string match {*.tcl} $argv0] {
	set prog [string range $argv0 0 [expr [string length $argv0] - 5]]
} else {
	set prog $argv0
}

source ../mcast/srm-nam.tcl		;# to separate control messages.
#source ../mcast/srm-debug.tcl		;# to trace delay compute fcn. details.

Simulator set NumberInterfaces_ 1
set ns [new MultiSim]
#$ns trace-all [open out.tr w]

# make the nodes
for {set i 0} {$i <= 3} {incr i} {
	set n($i) [$ns node]
}

# now the links
for {set i 1} {$i <= 3} {incr i} {
	$ns duplex-link $n($i) $n(0) 1.5Mb 10ms DropTail
}

set group 0x8000
set mh [$ns mrtproto DM {}]
#$ns at 0.3 "$mh switch-treetype $group"	;# for centralised multicast

# now the multicast, and the agents
set srmStats [open srmStats.tr w]
set srmEvents [open srmEvents.tr w]

set fid 0
foreach i [array names n] {
	set srm($i) [new Agent/SRM]
	$srm($i) set dst_ $group
	$srm($i) set fid_ [incr fid]
	$srm($i) log $srmStats
	$srm($i) trace $srmEvents
	$ns at 1.0 "$srm($i) start"

	$ns attach-agent $n($i) $srm($i)
}

# Attach a data source to srm(1)
set packetSize 210
set s0 [new Agent/CBR/UDP]
set exp0 [new Traffic/Expoo]
$exp0 set packet-size $packetSize
$exp0 set burst-time 500ms
$exp0 set idle-time 500ms
$exp0 set rate 100k
$s0 set fid_ 0
$s0 attach-traffic $exp0

$srm(0) traffic-source $s0
$srm(0) set packetSize_ $packetSize	;# so repairs are correct

$ns at 0.5 "$srm(0) start; $srm(0) start-source"
$ns at 1.0 "$srm(1) start"
$ns at 1.1 "$srm(2) start"
$ns at 1.2 "$srm(3) start"

proc distDump interval {
	global ns srm
	foreach i [array names srm] {
		set dist [$srm($i) distances?]
		if {$dist != ""} {
			puts "[format %7.4f [$ns now]] distances $dist"
		}
	}
	$ns at [expr [$ns now] + $interval] "distDump $interval"
}
$ns at 0.0 "distDump 1"

proc finish src {
	$src stop

	global prog ns srmStats srmEvents
	$ns flush-trace
	close $srmStats
	close $srmEvents

	puts "converting output to nam format..."
	exec awk -f ../nam-demo/nstonam.awk out.tr > $prog-nam.tr 
	#XXX
	puts "running nam..."
	exec nam $prog-nam &
	exit 0
}
$ns at 5.0 "finish $s0"

#$ns gen-map
$ns run

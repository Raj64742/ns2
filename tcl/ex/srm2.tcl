source ../mcast/srm.tcl
source ../mcast/srm-debug.tcl
set etrace_ [open srm-ev.tr w]

#source ../mcast/srm-debug++.tcl
#set etrace1_ [open srm-timers.tr w]

Simulator set NumberInterfaces_ 1
set ns [new MultiSim]
#set cmc [new CtrMcastComp $ns]
#$ns at 0.3 "$cmc switch-treetype 0x8000"
$ns at 0.0 "$ns run-mcast"

$ns trace-all [open out.tr w]
set srmStats [open srm-stats.tr w]

set group 0x8000
set fid  0
for {set i 0} {$i <= 5} {incr i} {
    set n($i) [$ns node]
    #new CtrMcast $ns $n($i) $cmc {}
    new DM $ns $n($i)

    set srm($i) [new Agent/SRM/Deterministic]
    $srm($i) set dst_ $group
    $srm($i) set fid_ [incr fid]
    $srm($i) trace $srmStats
    $ns at 1.0 "$srm($i) start"

    $ns attach-agent $n($i) $srm($i)
}

$ns duplex-link $n(0) $n(1) 1.5Mb 10ms DropTail
$ns duplex-link $n(1) $n(2) 1.5Mb 10ms DropTail
$ns duplex-link $n(2) $n(3) 1.5Mb 10ms DropTail
$ns duplex-link $n(3) $n(4) 1.5Mb 10ms DropTail
$ns duplex-link $n(3) $n(5) 1.5Mb 10ms DropTail

$ns queue-limit $n(0) $n(1) 2	;# q-limit is 1 more than max #packets in q.
$ns queue-limit $n(1) $n(0) 2

set packetSize 800
set s [new Agent/CBR]
$s set interval_ 0.02
$s set packetSize_ $packetSize
$srm(0) traffic-source $s
$srm(0) set packetSize_ $packetSize
$s set fid_ 0

$ns at 3.5 "$srm(0) start-source"


# Fake a dropped packet by incrementing seqno.
#$ns at 1.6 "$s1 set seqno 0"	;# need to figure out how to do this.
#
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

proc finish {} {
    global s
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

    puts "converting output to nam format..."
    exec awk -f ../nam-demo/nstonam.awk out.tr > $prog-nam.tr 
    exec rm -f out
    #XXX
    puts "running nam..."
    exec nam $prog-nam &
    exit 0
}

$ns at 4.0 "finish"
$ns run

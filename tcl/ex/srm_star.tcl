source ../mcast/srm-debug.tcl
set etrace_ [open srm-ev.tr w]

#source ../mcast/srm-debug++.tcl
#set etrace1_ [open srm-timers.tr w]

Simulator set NumberInterfaces_ 1
set ns [new MultiSim]
set cmc [new CtrMcastComp $ns]
$ns at 0.3 "$cmc switch-treetype 0x8000"

$ns trace-all [open out.tr w]
set srmStats [open srm-stats.tr w]

set nmax 15
set group 0x8000
set fid  -1
set n(0) [$ns node]
new CtrMcast $ns $n(0) $cmc {}		;# XXX Why?

for {set i 1} {$i <= $nmax} {incr i} {
    set n($i) [$ns node]
    new CtrMcast $ns $n($i) $cmc {}

    set srm($i) [new Agent/SRM/Probabilistic]
    $srm($i) set dst_ $group
    $srm($i) set fid_ [incr fid]
    $srm($i) trace $srmStats

    $ns attach-agent $n($i) $srm($i)
    $ns duplex-link $n($i) $n(0) 1.5Mb 10ms DropTail
}

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

set packetSize 800
set s [new Agent/CBR]
$s set interval_ 0.02
$s set packetSize_ $packetSize
$s set fid_ [incr fid]
$srm(1) traffic-source $s
$srm(1) set packetSize_ $packetSize
$ns at 4.0 "$srm(1) start-source"

# Fake a dropped packet by incrementing seqno.
#$ns at 1.6 "$s1 set seqno 0" 
$ns rtmodel-at 4.019 down $n(0) $n(1)	;# this ought to drop exactly one
$ns rtmodel-at 4.031 up   $n(0) $n(1)	;# data packet?

foreach i [array names srm] {
    $ns at 1.0 "$srm($i) start"
}
$ns at 9.0 "finish"

$ns run

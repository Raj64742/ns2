#source ../mcast/srm-debug.tcl
#set etrace_ [open srm-ev.tr w]

proc distDump { interval } {
    global ns
    global srm0 srm1 srm2 srm3
    foreach srm [list $srm0 $srm1 $srm2 $srm3] {
	set dist [$srm distances?]
	if {$dist != ""} {
	    puts "[format "%7.4f" [$ns now]] distances $dist"
	}
    }

    $ns at [expr [$ns now] + $interval] "distDump $interval"
}

#Simulator set NumberInterfaces_ 1   ;# because the Central. mcast only works
				     # with interfaces.
set ns [new MultiSim]
#set cmc [new CtrMcastComp $ns]	    ;# Use centralised multicast in spt mode.
#$ns at 0.3 "$cmc switch-treetype 0x8000"
$ns at 0.0 "$ns run-mcast"

set f [open out.tr w]
$ns trace-all $f

set srmStats [open srm-stats.tr w]

set fid 0			    ;# so it looks "cute" in nam.
for {set i 0} {$i < 4} {incr i} {
    set n [$ns node]
#    new "CtrMcast" $ns $n $cmc {}
    new DM $ns $n

    set srm [new Agent/SRM]
    $srm set dst_ 0x8000
    $srm set fid_ [incr fid]
    $srm trace $srmStats

    $ns attach-agent $n $srm

    set n$i   $n
    set srm$i $srm
}

$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n0 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n0 $n3 1.5Mb 10ms DropTail

set packetSize 210

set s0 [new Agent/CBR/UDP]
set exp0 [new Traffic/Expoo]
$exp0 set packet-size $packetSize
$exp0 set burst-time 500ms
$exp0 set idle-time 500ms
$exp0 set rate 100k
$s0 set fid_ 0
$s0 attach-traffic $exp0

$srm0 traffic-source $s0
$srm0 set packetSize_ $packetSize	;# so repairs are correct

$ns at 0.5 "$srm0 start; $srm0 start-source"
$ns at 1.0 "$srm1 start"
$ns at 1.1 "$srm2 start"
$ns at 1.2 "$srm3 start"

$ns at 0.0 "distDump 1"
$ns at 5.0 "finish"

proc finish {} {
    global s0
    $s0 stop

    global argv0
    global ns f srmStats
    $ns flush-trace
    close $f
    close $srmStats

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

#$ns gen-map
$ns run

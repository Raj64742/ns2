## Centralized Multicast Module Examples
## o. Use only source specific tree. 
## o. Note that when all receivers leave a group, the group tree type will be
## reset to the default, RP tree.
## o. RP related settings are not required.
##
## joining & pruning test(s)
#          |3|
#           |
#  |0|-----|1|
#           |
#          |2|

set ns [new MultiSim]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out-cmcast.tr w]
$ns trace-all $f

$ns duplex-link-of-interfaces $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n1 $n3 1.5Mb 10ms DropTail

set ctrmcastcomp [new CtrMcastComp $ns]

set ctrmcast0 [new CtrMcast $ns $n0 $ctrmcastcomp [list]]
set ctrmcast1 [new CtrMcast $ns $n1 $ctrmcastcomp [list]]
set ctrmcast2 [new CtrMcast $ns $n2 $ctrmcastcomp [list]]
set ctrmcast3 [new CtrMcast $ns $n3 $ctrmcastcomp [list]]

# Create a sender and receivers
set cbr1 [new Agent/CBR]
$ns attach-agent $n2 $cbr1
$cbr1 set dst_ 0x8003

set rcvr0 [new Agent/Null]
$ns attach-agent $n0 $rcvr0
set rcvr1 [new Agent/Null]
$ns attach-agent $n1 $rcvr1
set rcvr2  [new Agent/Null]
$ns attach-agent $n2 $rcvr2
set rcvr3 [new Agent/Null]
$ns attach-agent $n3 $rcvr3

# Switch group 0x8003 to source specific tree at the beginning
# Default tree type is RP tree (shared tree)
$ns at 0 "$ctrmcastcomp switch-treetype 0x8003"

# Group events
$ns at 0.2 "$cbr1 start"
$ns at 0.3 "$n1 join-group  $rcvr1 0x8003"
$ns at 0.4 "$n0 join-group  $rcvr0 0x8003"
$ns at 0.6 "$n3 join-group  $rcvr3 0x8003"
$ns at 0.6.5 "$n2 join-group  $rcvr2 0x8003"

$ns at 0.7 "$n0 leave-group $rcvr0 0x8003"
$ns at 0.8 "$n2 leave-group  $rcvr2 0x8003"

# Re-compute the entire mcast routes. 
# In this case, nothing is changed
$ns at 0.85 "$ctrmcastcomp compute-mroutes"
$ns at 0.9 "$n3 leave-group  $rcvr3 0x8003"
$ns at 1.0 "$n1 leave-group $rcvr1 0x8003"



$ns at 1.1 "finish"

proc finish {} {
        global ns
        $ns flush-trace
        exec awk -f ../../nam-demo/nstonam.awk out-cmcast.tr > cmcast-nam.tr
        # exec rm -f out
        #XXX
        puts "running nam..."
        exec ./nam cmcast-nam &
        exit 0
}

$ns run



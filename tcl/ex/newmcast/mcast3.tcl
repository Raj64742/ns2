## Using Multicast in ns
##
## joining & pruning test(s) 
#          |3|
#           |
#  |0|-----|1|
#           |
#          |2|

set ns [new Simulator -multicast on]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

$ns color 2 black
$ns color 1 blue
$ns color 0 yellow
$ns color 30 purple
$ns color 31 green

set f [open out-mc3.tr w]
$ns trace-all $f
set nf [open out-mc3.nam w]
$ns namtrace-all $nf

$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n3 1.5Mb 10ms DropTail

$ns duplex-link-op $n0 $n1 orient right
$ns duplex-link-op $n1 $n2 orient right-up
$ns duplex-link-op $n1 $n3 orient right-down

$ns duplex-link-op $n0 $n1 queuePos 0.5
$ns duplex-link-op $n1 $n0 queuePos 0.5
$ns duplex-link-op $n3 $n1 queuePos 0.5

### Start multicast configuration: 4 mproto options
### CtrMcast : centralized multicast
### DM       : static DVMRP (can't adapt to link up/down or node up/down)
### dynamicDM: dynamic DVMRP 
### pimDM    : PIM dense mode

### Uncomment following lines to change default
#DM set PruneTimeout 0.3               ;# default 0.5 (sec)

set mproto DM
set mrthandle [$ns mrtproto $mproto  {}]

if {$mproto == "CtrMcast"} {
    $mrthandle set_c_rp [list $n2 $n3]
}
### End of multicast configuration

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


$ns at 0.2 "$cbr1 start"
$ns at 0.3 "$n1 join-group  $rcvr1 0x8003"
$ns at 0.4 "$n0 join-group  $rcvr0 0x8003"
if {$mproto == "CtrMcast"} {
    $ns at 0.45 "$mrthandle switch-treetype 0x8003"
}
$ns at 0.5 "$n3 join-group  $rcvr3 0x8003"
$ns at 0.6.5 "$n2 join-group  $rcvr2 0x8003"

$ns at 0.7 "$n0 leave-group $rcvr0 0x8003"
$ns at 0.8 "$n2 leave-group  $rcvr2 0x8003"

$ns at 0.9 "$n3 leave-group  $rcvr3 0x8003"
$ns at 1.0 "$n1 leave-group $rcvr1 0x8003"


$ns at 1.1 "finish"

proc finish {} {
        global ns
        $ns flush-trace

        puts "running nam..."
        exec nam out-mc3 &
        exit 0
}

$ns run



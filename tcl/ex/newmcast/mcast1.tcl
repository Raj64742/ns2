## Using Multicast in ns
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


### Start multicast configuration: 4 mproto options
### CtrMcast : centralized multicast
### DM       : static DVMRP (can't adapt to link up/down or node up/down)
### dynamicDM: dynamic DVMRP 
###      dynamic DM set periodic 0: does triggered parent-child table update
###      dynamic DM set periodic 1: does periodic parent-child table update
### pimDM    : PIM dense mode

### Uncomment following lines to change default
#DM set PruneTimeout 0.3               ;# default 0.5 (sec)
#dynamicDM set periodic 1              ;# default = 0 (sec) 
#dynamicDM set ReportRouteTimeout 0.5  ;# default 1 (sec)

set mproto dynamicDM
set mrthandle [$ns mrtproto $mproto  {}]
if {$mrthandle != 0} {
    $mrthandle set_c_rp [list $n2 $n3]
    $mrthandle set_c_bsr [list $n1:0]
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
if {$mrthandle != 0} {
    $ns at 0.45 "$mrthandle switch-treetype 0x8003"
}
$ns at 0.5 "$n3 join-group  $rcvr3 0x8003"
$ns at 0.6.5 "$n2 join-group  $rcvr2 0x8003"

$ns at 0.7 "$n0 leave-group $rcvr0 0x8003"
$ns at 0.8 "$n2 leave-group  $rcvr2 0x8003"
if {$mrthandle != 0} {
    #$ns at 0.85 "$mrthandle compute-mroutes"
}
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
        exec nam cmcast-nam &
        exit 0
}

$ns run



set ns [new Simulator]
$ns multicast

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
set n4 [$ns node]
set n5 [$ns node]

$ns color 2 black
$ns color 1 blue
$ns color 0 yellow
# prune/graft packets
$ns color 30 purple
$ns color 31 green

set f [open out-mc2.tr w]
$ns trace-all $f
set nf [open out-mc2.nam w]
$ns namtrace-all $nf

$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n0 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n3 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n4 1.5Mb 10ms DropTail
$ns duplex-link $n2 $n4 1.5Mb 10ms DropTail
$ns duplex-link $n2 $n5 1.5Mb 10ms DropTail

$ns duplex-link-op $n0 $n1 orient left-down
$ns duplex-link-op $n0 $n2 orient down-right
$ns duplex-link-op $n1 $n3 orient left-down
$ns duplex-link-op $n1 $n4 orient right-down
$ns duplex-link-op $n2 $n4 orient left-down
$ns duplex-link-op $n2 $n5 orient right-down

$ns rtproto Session
### Start multicast configuration
set mproto CtrMcast
set mrthandle [$ns mrtproto $mproto  {}]
### End of multicast  config

set udp0 [new Agent/UDP]
$udp0 set dst_ 0x8002
$udp0 set class_ 0
$ns attach-agent $n0 $udp0
set cbr0 [new Application/Traffic/CBR]
$cbr0 attach-agent $udp0
#$cbr0 set interval_ 37.5ms

set rcvr3 [new Agent/LossMonitor]
set rcvr4 [new Agent/LossMonitor]
set rcvr5 [new Agent/LossMonitor]
$ns attach-agent $n3 $rcvr3
$ns attach-agent $n4 $rcvr4
$ns attach-agent $n5 $rcvr5

$ns at 0.2 "$n3 join-group $rcvr3 0x8002"
$ns at 0.4 "$n4 join-group $rcvr4 0x8002"
$ns at 0.6 "$n3 leave-group $rcvr3 0x8002"
$ns at 0.7 "$n5 join-group $rcvr5 0x8002"
$ns at 0.8 "$n3 join-group $rcvr3 0x8002"

#### Link between n0 & n1 down at 1.0, up at 1.2
$ns rtmodel-at 1.0 down $n0 $n1
$ns rtmodel-at 1.2 up $n0 $n1
####
set mmonitor1 [new McastMonitor]
set chan1 [open udp.tr w]
$mmonitor1 attach $chan1
$mmonitor1 trace-topo
$mmonitor1 filter Common ptype_ 2
$ns at 0.1 "$mmonitor1 print-trace"

set mmonitor2 [new McastMonitor]
set chan2 [open encap.tr w]
$mmonitor2 attach $chan2
$mmonitor2 trace-topo
$mmonitor2 filter Common ptype_ 32
$ns at 0.1 "$mmonitor2 print-trace"

####
$ns at 0.1 "$cbr0 start"
 
$ns at 1.6 "finish"

proc finish {} {
        #puts "converting output to nam format..."
        global ns chan1 chan2
        close $chan1
        close $chan2
        $ns flush-trace
        puts "running nam..."
        exec nam out-mc2 &
        exit 0
}

$ns run

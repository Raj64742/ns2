# tem.tcl - test emulation
set ns [new Simulator]
$ns use-scheduler RealTime
set livenet [new Network/Pcap/Live]
set a1 [new Agent/Tap]
$a1 network $livenet

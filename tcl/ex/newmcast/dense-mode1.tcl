set ns [new MultiSim]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out-dm1.tr w]
$ns trace-all $f

$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link $n1 $n3 1.5Mb 10ms DropTail

set dm0 [new DM $ns $n0]
set dm1 [new DM $ns $n1]
set dm2 [new DM $ns $n2]
set dm3 [new DM $ns $n3]

$ns at 0.0 "$ns run-mcast"

set cbr0 [new Agent/CBR]
$ns attach-agent $n1 $cbr0
$cbr0 set dst_ 0x8001
 
set cbr1 [new Agent/CBR]
$cbr1 set dst_ 0x8002
$cbr1 set class_ 1
$ns attach-agent $n3 $cbr1

set rcvr [new Agent/LossMonitor]
$ns attach-agent $n2 $rcvr
$ns at 1.2 "$n2 join-group $rcvr 0x8002"
$ns at 1.25 "$n2 leave-group $rcvr 0x8002"
$ns at 1.3 "$n2 join-group $rcvr 0x8002"
$ns at 1.35 "$n2 join-group $rcvr 0x8001"
 
$ns at 1.0 "$cbr0 start"
$ns at 1.1 "$cbr1 start"
 
set tcp [new Agent/TCP]
set sink [new Agent/TCPSink]
$ns attach-agent $n0 $tcp
$ns attach-agent $n3 $sink
$ns connect $tcp $sink
set ftp [new Source/FTP]
$ftp set agent_ $tcp
$ns at 1.2 "$ftp start"


$ns at 1.7 "finish"

proc finish {} {
        puts "converting output to nam format..."
        global ns
        $ns flush-trace
        exec awk -f ../nam-demo/nstonam.awk out-dm1.tr > dense-mode1-nam.tr
        exec rm -f out
        #XXX
        puts "running nam..."
        exec ./nam dense-mode1-nam &
        exit 0
}

$ns run


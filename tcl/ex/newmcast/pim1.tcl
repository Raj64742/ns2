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

set f [open out-pim1.tr w]
$ns trace-all $f

$ns duplex-link-of-interfaces $n0 $n1 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n1 $n2 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n1 $n3 1.5Mb 10ms DropTail

set pim0 [new PIM $ns $n0 [list 1 1 0]]
set pim1 [new PIM $ns $n1 [list 0 1 0]]
set pim2 [new PIM $ns $n2 [list 1]]
set pim3 [new PIM $ns $n3 [list 0 1 0]]

PIM config $ns
puts "config done.. "

Agent/Message instproc handle msg {
        $self instvar node_
        puts "@@@@@@@@@node [$node_ id] agent $self rxvd msg $msg. @@@@@@@@"
}

set rcvr0 [new Agent/Message]
$ns attach-agent $n0 $rcvr0
set rcvr1 [new Agent/Message]
$ns attach-agent $n1 $rcvr1
set rcvr2  [new Agent/Message]
$ns attach-agent $n2 $rcvr2
set rcvr3 [new Agent/Message]
$ns attach-agent $n3 $rcvr3

set sender0 [new Agent/CBR]
$sender0 set dst_ 0x8003
$sender0 set class_ 2
$ns attach-agent $n0 $sender0

$ns at 241 "$sender0 start"
$ns at 241.95 "$n0 join-group $rcvr0 0x8003"
$ns at 242.05 "$n1 join-group $rcvr1 0x8003"
$ns at 242.15 "$n3 join-group $rcvr3 0x8003"

#$ns at 242.0 "$sender0 send \"pkt1\""
#$ns at 242.1 "$sender0 send \"pkt2\""
#$ns at 242.12 "$sender0 send \"pkt2\""
#$ns at 242.14 "$sender0 send \"pkt2\""
#$ns at 242.2 "$sender0 send \"pkt3\""
#$ns at 242.3 "$sender0 send \"pkt4\""
#$ns at 242.4 "$sender0 send \"pkt5\""
#$ns at 242.5 "$sender0 send \"pkt6\""

$ns at 243 "finish"

$ns at 0.0 "$ns run-mcast"

proc finish {} {
        global ns
        $ns flush-trace
        exec awk -f ../nam-demo/nstonam.awk out-pim1.tr > pim1-nam.tr
        # exec rm -f out
        #XXX
        puts "running nam..."
        exec ./nam pim1-nam &
        exit 0
}

$ns run


# tem.tcl - test emulation
set me [exec hostname]
set ns [new Simulator]
$ns use-scheduler RealTime

set livenet1 [new Network/Pcap/Live]
$livenet1 set promisc_ true

set livenet2 [new Network/Pcap/Live]
$livenet2 set promisc_ true

set nd1 [$livenet1 open] ;# open some appropriate device (devname is optional)
set nd2 [$livenet2 open] ;# open some appropriate device (devname is optional)
puts "ok, got network devices $nd1 and $nd2"

set f1len [$livenet1 filter "tcp and src $me"]
set f2len [$livenet2 filter "tcp and dst $me"]

puts "filter 1 len: $f1len, filter 2 len: $f2len"

set a1 [new Agent/Tap]
set a2 [new Agent/Tap]
puts "install nets into taps..."
$a1 network $livenet1
$a2 network $livenet2

set node0 [$ns node]
set node1 [$ns node]

$ns duplex-link $node0 $node1 8Mb 5ms DropTail
$ns attach-agent $node0 $a1
$ns attach-agent $node1 $a2
$ns connect $a1 $a2

puts "let's rock"
$ns run

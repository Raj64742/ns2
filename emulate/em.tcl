# tem.tcl - test emulation
puts "make sim.."
set ns [new Simulator]
puts "change to RT scheduler..."
$ns use-scheduler RealTime
puts "allocate net..."
set livenet [new Network/Pcap/Live]
$livenet set promisc_ true
puts "opening network"
set netdev [$livenet open] ;# open some appropriate device (devname is optional)
#$livenet filter "src pig or dst pig"
puts "ok, got network $netdev"
puts "allocate tap agent ..."
set a1 [new Agent/Tap]
puts "install net into tap..."
$a1 network $livenet
puts "let's rock"
$ns run

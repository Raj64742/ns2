# em2.tcl - test emulation
# be careful here, as the tap agents will
# send out whatever they are given
#
set dotrace 1
set stoptime 20.0

set me [exec hostname]
set ns [new Simulator]
set bpfilter "src bit or dst bit"

if { $dotrace } {
	set allchan [open em-all.tr w]
	$ns trace-all $allchan
	set namchan [open em.nam w]
	$ns namtrace-all $namchan
}

$ns use-scheduler RealTime

#
# allocate a BPF type network object and a raw-IP object
#
set bpf0 [new Network/Pcap/Live]
set bpf1 [new Network/Pcap/Live]
$bpf0 set promisc_ true
$bpf1 set promisc_ true

set ipnet [new Network/IP]

set nd0 [$bpf0 open fxp0] ;# open some appropriate device (devname is optional)
set nd1 [$bpf1 open fxp1] ;# open some appropriate device (devname is optional)
$ipnet open

puts "bpf0 on dev $nd0, bpf1 on dev $nd1"

set f0len [$bpf0 filter $bpfilter]
set f1len [$bpf1 filter $bpfilter]

puts "filter lengths: $f0len (bpf0), $f1len (bpf1)"
puts "dev $nd0 has address [$bpf0 linkaddr]"
puts "dev $nd1 has address [$bpf1 linkaddr]"

set a0 [new Agent/Tap]
set a1 [new Agent/Tap]
set a2 [new Agent/Tap]
puts "install nets into taps..."
$a0 network $bpf0
$a1 network $bpf1
$a2 network $ipnet

set node0 [$ns node]
set node1 [$ns node]
set node2 [$ns node]

$ns simplex-link $node0 $node2 8Mb 5ms DropTail
$ns simplex-link $node1 $node2 8Mb 5ms DropTail

#
# attach-agent sets "target" of the agents to the node entrypoint
$ns attach-agent $node0 $a0
$ns attach-agent $node1 $a1
$ns attach-agent $node2 $a2

# let's detach the things
set null [$ns set nullAgent_]
###$a1 target $null
###$a1 target $null

$ns connect $a0 $a2
$ns connect $a1 $a2

puts "scheduling termination at t=$stoptime"
$ns at $stoptime "exit 0"

puts "let's rock"
$ns run

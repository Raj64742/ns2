### This simulation is an example of combination of wired and wireless 
### topologies.


#                   0    o W1(0.0.0)                 WIRED NODES
#                        |
#                   1    o W2 (0.1.0)
#                       / \
#                      /   \                    
#--*--*--*--*--*-  2  o     o  3  base-stn nodes  --*-*-*-*-*-*-*-
#         (2.0.0)   BS1      BS2 (2.0.0)              
#                       o 5
#               4  o    WL2 (2.0.2)  
#         (2.0.1) WL1             
#                       o 6 (2.0.3)        WIRELESS NODES
#                      WL3
#
# node 4 and 6 communicate thru node 5 who can hear the base-station node BS1. TCP flows are established between node 0 and node 6.
#
#===============================================================
#options
set opt(chan)		Channel/WirelessChannel
set opt(prop)		Propagation/TwoRayGround
set opt(netif)		Phy/WirelessPhy
set opt(mac)		Mac/802_11
set opt(ifq)		Queue/DropTail/PriQueue
set opt(ll)		LL
set opt(ant)            Antenna/OmniAntenna
set opt(x)		670	;# X & Y dimension of the topography
set opt(y)		670     ;# hard wired for now...
##set opt(cp)		"../mobility/scene/cbr-3-test" ;# connection pattern file
set opt(cp)             ""
set opt(sc)		"../mobility/scene/scen-3-test" ;# scenario file
set opt(rp)             dsr      ;# available routing proto:dsdv/dsr
set opt(ifqlen)		50	   ;# max packet in ifq
set opt(seed)		0.0
set opt(stop)		500.0	   ;# simulation time
set opt(cc)             "off"
set opt(tr)		wired-and-wireless-out.tr	;# trace file
set opt(ftp1-start)     200.0
set opt(ftp2-start)     160.0
set opt(cbr-start)      240.0

# =================================================================

## Default settings
set num_wired_nodes    2
set num_bs_nodes       2
set num_wireless_nodes 3

set opt(nn)            5            ;# total number of wireless nodes
#==================================================================


# Other class settings

set AgentTrace			ON
set RouterTrace			ON
set MacTrace			OFF

LL set mindelay_		50us
LL set delay_			25us

Agent/Null set sport_		0
Agent/Null set dport_		0

Agent/CBR set sport_		0
Agent/CBR set dport_		0

Agent/TCPSink set sport_	0
Agent/TCPSink set dport_	0

Agent/TCP set sport_		0
Agent/TCP set dport_		0
Agent/TCP set packetSize_	1460

Queue/DropTail/PriQueue set Prefer_Routing_Protocols    1

# unity gain, omni-directional antennas
# set up the antennas to be centered in the node and 1.5 meters above it
Antenna/OmniAntenna set X_ 0
Antenna/OmniAntenna set Y_ 0
Antenna/OmniAntenna set Z_ 1.5
Antenna/OmniAntenna set Gt_ 1.0
Antenna/OmniAntenna set Gr_ 1.0

# Initialize the SharedMedia interface with parameters to make
# it work like the 914MHz Lucent WaveLAN DSSS radio interface
Phy/WirelessPhy set CPThresh_ 10.0
Phy/WirelessPhy set CSThresh_ 1.559e-11
Phy/WirelessPhy set RXThresh_ 3.652e-10
Phy/WirelessPhy set Rb_ 2*1e6
Phy/WirelessPhy set Pt_ 0.2818
Phy/WirelessPhy set freq_ 914e+6 
Phy/WirelessPhy set L_ 1.0

# ======================================================================
source ../lib/ns-wireless-mip.tcl

# intial setup - set addressing to hierarchical

set ns [new Simulator]
$ns set-address-format hierarchical

set namtrace [open wired-wireless-out.nam w]
$ns namtrace-all $namtrace
set trace [open wired-wireless-out.tr w]
$ns trace-all $trace

AddrParams set domain_num_ 3
lappend cluster_num 2 1 1
AddrParams set cluster_num_ $cluster_num
lappend eilastlevel 1 1 4 1
AddrParams set nodes_num_ $eilastlevel

# setup the wired nodes
set temp {0.0.0 0.1.0}
for {set i 0} {$i < $num_wired_nodes} {incr i} {
    set W($i) [$ns node [lindex $temp $i]] 
}


### setup base stations & wireless nodes

# Create common objects & other stuff for wireless topology
#source ../lib/ns-bsnode.tcl
#source ../mobility/com.tcl
#source ../mobility/dsr.tcl

if { $opt(x) == 0 || $opt(y) == 0 } {
	puts "No X-Y boundary values given for wireless topology\n"
}
set chan        [new $opt(chan)]
set prop        [new $opt(prop)]
set topo	[new Topography]
set tracefd	[open $opt(tr) w]

#setup topography and propagation model
$topo load_flatgrid $opt(x) $opt(y)
$prop topography $topo

# Create God
create-god $opt(nn)

# create base stations

set temp {1.0.0 1.0.1 1.0.2 1.0.3}
set BS(0) [create-base-station-node [lindex $temp 0]]
set BS(1) [create-base-station-node 2.0.0]

#provide some co-ord (fixed) to base stations
$BS(0) set X_ 1.0
$BS(0) set Y_ 2.0
$BS(0) set Z_ 0.0

$BS(1) set X_ 650.0
$BS(1) set Y_ 600.0
$BS(1) set Z_ 0.0


#create some mobilenodes in the same domain as BS_0
for {set j 0} {$j < $num_wireless_nodes} {incr j} {
    set node_($j) [ $opt(rp)-create-mobile-node $j [lindex $temp \
	    [expr $j+1]] ]
    $node_($j) base-station [AddrParams set-hieraddr \
	    [$BS(0) node-addr]]
}

if { $opt(x) == 0 || $opt(y) == 0 } {
	usage $argv0
	exit 1
}

if {$opt(seed) > 0} {
	puts "Seeding Random number generator with $opt(seed)\n"
	ns-random $opt(seed)
}

#
# Source the Connection and Movement scripts
#
if { $opt(cp) == "" } {
	puts "*** NOTE: no connection pattern specified."
        set opt(cp) "none"
} else {
	puts "Loading connection pattern..."
	source $opt(cp)
}

if { $opt(sc) == "" } {
	puts "*** NOTE: no scenario file specified."
        set opt(sc) "none"
} else {
	puts "Loading scenario file..."
	source $opt(sc)
	puts "Load complete..."
}


#create links between wired and BS nodes

$ns duplex-link $W(0) $W(1) 5Mb 2ms DropTail
$ns duplex-link $W(1) $BS(0) 5Mb 2ms DropTail
$ns duplex-link $W(1) $BS(1) 5Mb 2ms DropTail

$ns duplex-link-op $W(0) $W(1) orient down
$ns duplex-link-op $W(1) $BS(0) orient left-down
$ns duplex-link-op $W(1) $BS(1) orient right-down


# setup TCP connections
set tcp1 [new Agent/TCP]
$tcp1 set class_ 2
set sink1 [new Agent/TCPSink]
$ns attach-agent $node_(0) $tcp1
$ns attach-agent $W(0) $sink1
$ns connect $tcp1 $sink1
set ftp1 [new Application/FTP]
$ftp1 attach-agent $tcp1
$ns at $opt(ftp1-start) "$ftp1 start"

set tcp2 [new Agent/TCP]
$tcp2 set class_ 2
set sink2 [new Agent/TCPSink]
$ns attach-agent $W(1) $tcp2
$ns attach-agent $node_(2) $sink2
$ns connect $tcp2 $sink2
set ftp2 [new Application/FTP]
$ftp2 attach-agent $tcp2
$ns at $opt(ftp2-start) "$ftp2 start"

set udp_(0) [new Agent/UDP]
$ns_ attach-agent $node_(0) $udp_(0)
set null_(0) [new Agent/Null]
$ns_ attach-agent $W(0) $null_(0)
set cbr_(0) [new Application/Traffic/CBR]
$cbr_(0) set packetSize_ 512
$cbr_(0) set interval_ 4.0
$cbr_(0) set random_ 1
$cbr_(0) set maxpkts_ 10000
$cbr_(0) attach-agent $udp_(0)
$ns_ connect $udp_(0) $null_(0)
$ns_ at $opt(cbr-start) "$cbr_(0) start"

#
# Tell all the nodes when the simulation ends
#
for {set i 0} {$i < $num_wireless_nodes } {incr i} {
    $ns_ at $opt(stop).0000010 "$node_($i) reset";
}
$ns_ at $opt(stop).0000010 "$BS(0) reset";
$ns_ at $opt(stop).0000010 "$BS(1) reset";

$ns_ at $opt(stop).21 "finish"
$ns_ at $opt(stop).20 "puts \"NS EXITING...\" ; "

proc finish {} {
	global ns_ trace namtrace
	$ns_ flush-trace
	close $namtrace
	close $trace

	#puts "running nam..."
	#exec nam out.nam &
        puts "Finishing ns.."
	exit 0
}






puts $tracefd "M 0.0 nn $opt(nn) x $opt(x) y $opt(y) rp $opt(rp)"
puts $tracefd "M 0.0 sc $opt(sc) cp $opt(cp) seed $opt(seed)"
puts $tracefd "M 0.0 prop $opt(prop) ant $opt(ant)"

puts "Starting Simulation..."
$ns_ run


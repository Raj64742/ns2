# ======================================================================
# Default Script Options
# ======================================================================
set opt(chan)		Channel
set opt(prop)		Propagation/TwoRayGround
#set opt(netif)		NetIf/WaveLAN
set opt(netif)		Phy/WirelessPhy
set opt(mac)		Mac/802_11
set opt(ifq)		Queue/DropTail/PriQueue
set opt(ll)		LL
set opt(ant)            Antenna/OmniAntenna

set opt(x)		670		;# X dimension of the topography
set opt(y)		670		;# Y dimension of the topography
set opt(cp)		"../mobility/scene/cbr-50-10-4-512"		;# connection pattern file
set opt(sc)		"../mobility/scene/scen-670x670-50-600-20-0"		;# scenario file
#set opt(sc)		"../mobility/scene/scen-3-test"
#set opt(cp)		"../mobility/scene/cbr-3-test"

set opt(ifqlen)		50		;# max packet in ifq
set opt(nn)		50		;# number of nodes
set opt(seed)		0.0
set opt(stop)		900.0		;# simulation time
set opt(tr)		out.tr		;# trace file

# ======================================================================

set AgentTrace			ON
set RouterTrace			ON
set MacTrace			OFF

LL set delay_			5us
LL set bandwidth_		0	;# not used
LL set off_prune_		0	;# not used
LL set off_CtrMcast_		0	;# not used

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


#source cmu/scripts/mobile_node.tcl
source ../mobility/tora.tcl
#source cmu/scripts/cmu-trace.tcl
source ../lib/ns-bsnode.tcl
source ../mobility/com.tcl
# ======================================================================

proc usage { argv0 }  {
	puts "Usage: $argv0"
	puts "\tmandatory arguments:"
	puts "\t\t\[-cp conn pattern\] \[-sc scenario\] \[-x MAXX\] \[-y MAXY]\]"
	puts "\toptional arguments:"
	puts "\t\t\[-nn nodes\] \[-seed seed\] \[-stop sec\] \[-tr tracefile\]\n"
}


proc getopt {argc argv} {
	global opt
	lappend optlist cp nn seed sc stop tr x y

	for {set i 0} {$i < $argc} {incr i} {
		set arg [lindex $argv $i]
		if {[string range $arg 0 0] != "-"} continue

		set name [string range $arg 1 end]
		set opt($name) [lindex $argv [expr $i+1]]
	}
}


#proc cmu-trace { ttype atype nodeid } {
#	global ns_ tracefd

#	if { $tracefd == "" } {
#		return ""
#	}
#	set T [new CMUTrace/$ttype $atype]
#	$T target [$ns_ set nullAgent_]
#	$T attach $tracefd
#	$T set src_ $nodeid
#	return $T
#}


#proc create-god { nodes } {
#	global ns_ god_ tracefd
#
#	set godtrace     [new Trace/Generic]
#	$godtrace target [$ns_ set nullAgent_]
#	$godtrace attach $tracefd
#
#	set god_        [new God]
#	$god_ num_nodes $nodes
#	$god_ tracetarget $godtrace
#}


# ======================================================================
# Main Program
# ======================================================================
getopt $argc $argv

if { $opt(x) == 0 || $opt(y) == 0 } {
	usage $argv0
	exit 1
}

if {$opt(seed) > 0} {
	puts "Seeding Random number generator with $opt(seed)\n"
	ns-random $opt(seed)
}

#
# Initialize Global Variables
#
set ns_		[new Simulator]
set chan	[new $opt(chan)]
set prop	[new $opt(prop)]
set topo	[new Topography]
set tracefd	[open $opt(tr) w]

#$topo load_flatgrid [expr $opt(x)./100] [expr $opt(y)./100] 100

$topo load_flatgrid $opt(x) $opt(y)

$prop topography $topo

#
# Create God
#
create-god $opt(nn)

#
#  Create the specified number of nodes $opt(nn) and "attach" them
#  the channel.
#

for {set i 0} {$i < $opt(nn) } {incr i} {
	create-mobile-node $i
}

#
# Source the Connection and Movement scripts
#
if { $opt(cp) == "" } {
	puts "*** NOTE: no connection pattern specified."
} else {
	source $opt(cp)
}

if { $opt(sc) == "" } {
	puts "*** NOTE: no scenario file specified."
} else {
	source $opt(sc)
}

#source cmu/tora/comm-box.tcl

$ns_ at $opt(stop) "puts \"NS EXITING...\" ; exit"

puts "Starting Simulation..."

$ns_ run









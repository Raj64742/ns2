# To run all tests: test-all-wireless-diffusion
# to run individual test:
# ns test-suite-wireless-diffusion.tcl diff-rate-default
# ns test-suite-wireless-diffusion.tcl diff-rate-other
# ns test-suite-wireless-diffusion.tcl diff-prob
# ns test-suite-wireless-diffusion.tcl omnimcast
# ns test-suite-wireless-diffusion.tcl flooding
#
# To view a list of available test to run with this script:
# ns test-suite-wireless-diffusion.tcl


# ======================================================================
# Define options
# ======================================================================
global opt
set opt(chan)		Channel/WirelessChannel
set opt(prop)		Propagation/TwoRayGround
set opt(netif)		Phy/WirelessPhy
set opt(mac)		Mac/802_11
set opt(ifq)		Queue/DropTail/PriQueue
set opt(ll)		LL
set opt(ant)            Antenna/OmniAntenna

set opt(ifqlen)		50		;# max packet in ifq
set opt(seed)		1
set opt(tr)		estudy.tr	;# trace file
set opt(nam)		temp.rands
set opt(lm)             "off"           ;# log movement
set opt(engmodel)       EnergyModel     ;
set opt(initeng)        1.1               ;# Initial energy in Joules
set opt(txPower)        0.660;
set opt(rxPower)        0.395;
set opt(idlePower)      0.035;
set opt(x)		800	;# X dimension of the topography
set opt(y)		800     ;# Y dimension of the topography
set opt(nn)		30	;# number of nodes
set opt(stop)		25	;# simulation time
set opt(prestop)        23      ;# time to prepare to stop

# ======================================================================

LL set mindelay_		50us
LL set delay_			25us
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

# ======================================================================

proc usage {}  {
        global argv0
	puts stderr "usage: ns $argv0 <tests> "
	puts "Valid <tests> : diff-rate-default diff-rate-other diff-prob omnicast flooding "
        exit 1
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

Class TestSuite

Class Test/diff-rate-default -superclass TestSuite
Class Test/diff-rate-other -superclass TestSuite
Class Test/diff-prob -superclass TestSuite
Class Test/omnimcast -superclass TestSuite
Class Test/flooding -superclass TestSuite

TestSuite instproc init {} {
global opt 
$self instvar ns_ topo  god_

set ns_		[new Simulator]

puts "Seeding Random number generator with $opt(seed)\n"
ns-random $opt(seed)

set topo	[new Topography]

#set god_         [new God]

set tracefd	[open $opt(tr) w]
set namtrace    [open $opt(nam) w]

$topo load_flatgrid $opt(x) $opt(y)

$ns_ trace-all $tracefd
$ns_ namtrace-all-wireless $namtrace $opt(x) $opt(y)

set god_ [create-god $opt(nn)]
}


Test/diff-rate-default instproc init {} {
   global opt
   $self instvar ns_ topo god_

    set opt(god)            on
    set opt(traf)	"sk-30-3-3-1-1-6-64.tcl"      ;# traffic file
    set opt(topo)	"scen-800x800-30-500-1.0-1"   ;# topology file
    set opt(onoff)          ""                        ;# node on-off
    set opt(adhocRouting)   DIFFUSION/RATE       
    set opt(enablePos)      "true"
    set opt(enableNeg)      "true"
    set opt(subTxType)      BROADCAST
    set opt(orgTxType)      UNICAST
    set opt(posType)        ALL
    set opt(posNodeType)    INTM
    set opt(negWinType)     TIMER
    set opt(negThrType)     ABSOLUTE
    set opt(negMaxType)     FIXED
    set opt(suppression)    "true"
    set opt(duplicate)      "enable-duplicate"

   $self next
}
Test/diff-rate-default instproc run {} {
   global opt
   $self instvar ns_ topo god_

    $god_ $opt(god)
    $god_ allow_to_stop
    $god_ num_data_types 1
    
    $ns_ node-config -adhocRouting $opt(adhocRouting) \
		 -llType $opt(ll) \
		 -macType $opt(mac) \
		 -ifqType $opt(ifq) \
		 -ifqLen $opt(ifqlen) \
		 -antType $opt(ant) \
		 -propType $opt(prop) \
		 -phyType $opt(netif) \
		 -channelType $opt(chan) \
		 -topoInstance $topo \
		 -agentTrace ON \
                 -routerTrace ON \
                 -macTrace ON \
		 -energyModel $opt(engmodel) \
		 -initialEnergy $opt(initeng) \
		 -txPower  $opt(txPower) \
		 -rxPower  $opt(rxPower) \
		 -idlePower  $opt(idlePower)

			 
    for {set i 0} {$i < $opt(nn) } {incr i} {
	set node_($i) [$ns_ node $i]
	$node_($i) random-motion 0		;# disable random motion
        $god_ new_node $node_($i)
    }

    if { $opt(topo) == "" } {
	puts "*** NOTE: no topology file specified."
        set opt(topo) "none"
    } else {
	puts "Loading topology file..."
	source $opt(topo)
	puts "Load complete..."
    }

    if { $opt(onoff) == "" } {
	puts "*** NOTE: no node-on-off file specified."
        set opt(onoff) "none"
    } else {
	puts "Loading node on-off file..."
	source $opt(onoff)
	puts "Load complete..."
    }

    if { $opt(traf) == "" } {
	puts "*** NOTE: no traffic file specified."
        set opt(traf) "none"
    } else {
	puts "Loading traffic file..."
	source $opt(traf)
	puts "Load complete..."
    }

#
# Tell all the nodes when the simulation ends
#

$ns_ at $opt(prestop) "$ns_ prepare-to-stop"
for {set i 0} {$i < $opt(nn) } {incr i} {
    $ns_ at $opt(stop).000000001 "$node_($i) reset";
}
$ns_ at $opt(stop).00000001 "$self finish; puts \"NS EXITING...\" ; $ns_ halt"


# Define node initial position in nam

for {set i 0} {$i < $opt(nn)} {incr i} {

    # 20 defines the node size in nam, must adjust it according to your scenario
    # The function must be called after mobility model is defined

    $ns_ initial_node_pos $node_($i) 20
}

$ns_ run
}


Test/diff-rate-other instproc init {} {
   global opt
   $self instvar ns_ topo god_

    set opt(god)            on
    set opt(traf)	"sk-30-3-3-1-1-6-64.tcl"      ;# traffic file
    set opt(topo)	"scen-800x800-30-500-1.0-1"   ;# topology file
    set opt(onoff)      "onoff-30-3-3-1-1-500-10.tcl" ;# node on-off
    set opt(adhocRouting)   DIFFUSION/RATE       
    set opt(enablePos)      "true"
    set opt(enableNeg)      "true"
    set opt(subTxType)      BROADCAST
    set opt(orgTxType)      UNICAST
    set opt(posType)        LAST
    set opt(posNodeType)    END
    set opt(negWinType)     COUNTER
    set opt(negThrType)     ABSOLUTE
    set opt(negMaxType)     FIXED
    set opt(suppression)    "false"
    set opt(duplicate)      "disable-duplicate"

   $self next
}
Test/diff-rate-other instproc run {} {
   global opt
   $self instvar ns_ topo god_

    $god_ $opt(god)
    $god_ allow_to_stop
    $god_ num_data_types 1
    
    $ns_ node-config -adhocRouting $opt(adhocRouting) \
		 -llType $opt(ll) \
		 -macType $opt(mac) \
		 -ifqType $opt(ifq) \
		 -ifqLen $opt(ifqlen) \
		 -antType $opt(ant) \
		 -propType $opt(prop) \
		 -phyType $opt(netif) \
		 -channelType $opt(chan) \
		 -topoInstance $topo \
		 -agentTrace ON \
                 -routerTrace ON \
                 -macTrace ON \
		 -energyModel $opt(engmodel) \
		 -initialEnergy $opt(initeng) \
		 -txPower  $opt(txPower) \
		 -rxPower  $opt(rxPower) \
		 -idlePower  $opt(idlePower)

			 
    for {set i 0} {$i < $opt(nn) } {incr i} {
	set node_($i) [$ns_ node $i]
	$node_($i) random-motion 0		;# disable random motion
        $god_ new_node $node_($i)
    }

    if { $opt(topo) == "" } {
	puts "*** NOTE: no topology file specified."
        set opt(topo) "none"
    } else {
	puts "Loading topology file..."
	source $opt(topo)
	puts "Load complete..."
    }

    if { $opt(onoff) == "" } {
	puts "*** NOTE: no node-on-off file specified."
        set opt(onoff) "none"
    } else {
	puts "Loading node on-off file..."
	source $opt(onoff)
	puts "Load complete..."
    }

    if { $opt(traf) == "" } {
	puts "*** NOTE: no traffic file specified."
        set opt(traf) "none"
    } else {
	puts "Loading traffic file..."
	source $opt(traf)
	puts "Load complete..."
    }

#
# Tell all the nodes when the simulation ends
#

$ns_ at $opt(prestop) "$ns_ prepare-to-stop"
for {set i 0} {$i < $opt(nn) } {incr i} {
    $ns_ at $opt(stop).000000001 "$node_($i) reset";
}
$ns_ at $opt(stop).00000001 "$self finish; puts \"NS EXITING...\" ; $ns_ halt"


# Define node initial position in nam

for {set i 0} {$i < $opt(nn)} {incr i} {

    # 20 defines the node size in nam, must adjust it according to your scenario
    # The function must be called after mobility model is defined

    $ns_ initial_node_pos $node_($i) 20
}

$ns_ run
}


Test/diff-prob instproc init {} {
   global opt
   $self instvar ns_ topo god_

    set opt(god)            on
    set opt(traf)	"sk-30-1-1-1-1-6-64.tcl"      ;# traffic file
    set opt(topo)	"scen-800x800-30-500-1.0-1"   ;# topology file
    set opt(onoff)          ""                        ;# node on-off
    set opt(adhocRouting)   DIFFUSION/PROB       
    set opt(enablePos)      "true"
    set opt(enableNeg)      "true"
    set opt(duplicate)      "enable-duplicate"

   $self next
}
Test/diff-prob instproc run {} {
   global opt
   $self instvar ns_ topo god_

    $god_ $opt(god)
    $god_ allow_to_stop
    $god_ num_data_types 1
    
    $ns_ node-config -adhocRouting $opt(adhocRouting) \
		 -llType $opt(ll) \
		 -macType $opt(mac) \
		 -ifqType $opt(ifq) \
		 -ifqLen $opt(ifqlen) \
		 -antType $opt(ant) \
		 -propType $opt(prop) \
		 -phyType $opt(netif) \
		 -channelType $opt(chan) \
		 -topoInstance $topo \
		 -agentTrace ON \
                 -routerTrace ON \
                 -macTrace ON \
		 -energyModel $opt(engmodel) \
		 -initialEnergy $opt(initeng) \
		 -txPower  $opt(txPower) \
		 -rxPower  $opt(rxPower) \
		 -idlePower  $opt(idlePower)

			 
    for {set i 0} {$i < $opt(nn) } {incr i} {
	set node_($i) [$ns_ node $i]
	$node_($i) random-motion 0		;# disable random motion
        $god_ new_node $node_($i)
    }

    if { $opt(topo) == "" } {
	puts "*** NOTE: no topology file specified."
        set opt(topo) "none"
    } else {
	puts "Loading topology file..."
	source $opt(topo)
	puts "Load complete..."
    }

    if { $opt(onoff) == "" } {
	puts "*** NOTE: no node-on-off file specified."
        set opt(onoff) "none"
    } else {
	puts "Loading node on-off file..."
	source $opt(onoff)
	puts "Load complete..."
    }

    if { $opt(traf) == "" } {
	puts "*** NOTE: no traffic file specified."
        set opt(traf) "none"
    } else {
	puts "Loading traffic file..."
	source $opt(traf)
	puts "Load complete..."
    }

#
# Tell all the nodes when the simulation ends
#

$ns_ at $opt(prestop) "$ns_ prepare-to-stop"
for {set i 0} {$i < $opt(nn) } {incr i} {
    $ns_ at $opt(stop).000000001 "$node_($i) reset";
}
$ns_ at $opt(stop).00000001 "$self finish; puts \"NS EXITING...\" ; $ns_ halt"


# Define node initial position in nam

for {set i 0} {$i < $opt(nn)} {incr i} {

    # 20 defines the node size in nam, must adjust it according to your scenario
    # The function must be called after mobility model is defined

    $ns_ initial_node_pos $node_($i) 20
}

$ns_ run
}


Test/omnimcast instproc init {} {
   global opt
   $self instvar ns_ topo god_

    set opt(god)            on
    set opt(traf)	"sk-30-3-3-1-1-6-64.tcl"      ;# traffic file
    set opt(topo)	"scen-800x800-30-500-1.0-1"   ;# topology file
    set opt(onoff)          ""                        ;# node on-off
    set opt(adhocRouting)   OMNIMCAST       
    set opt(duplicate)      "enable-duplicate"

   $self next
}
Test/omnimcast instproc run {} {
   global opt
   $self instvar ns_ topo god_

    $god_ $opt(god)
    $god_ allow_to_stop
    $god_ num_data_types 1
    
    $ns_ node-config -adhocRouting $opt(adhocRouting) \
		 -llType $opt(ll) \
		 -macType $opt(mac) \
		 -ifqType $opt(ifq) \
		 -ifqLen $opt(ifqlen) \
		 -antType $opt(ant) \
		 -propType $opt(prop) \
		 -phyType $opt(netif) \
		 -channelType $opt(chan) \
		 -topoInstance $topo \
		 -agentTrace ON \
                 -routerTrace ON \
                 -macTrace ON \
		 -energyModel $opt(engmodel) \
		 -initialEnergy $opt(initeng) \
		 -txPower  $opt(txPower) \
		 -rxPower  $opt(rxPower) \
		 -idlePower  $opt(idlePower)

			 
    for {set i 0} {$i < $opt(nn) } {incr i} {
	set node_($i) [$ns_ node $i]
	$node_($i) random-motion 0		;# disable random motion
        $god_ new_node $node_($i)
    }

    if { $opt(topo) == "" } {
	puts "*** NOTE: no topology file specified."
        set opt(topo) "none"
    } else {
	puts "Loading topology file..."
	source $opt(topo)
	puts "Load complete..."
    }

    if { $opt(onoff) == "" } {
	puts "*** NOTE: no node-on-off file specified."
        set opt(onoff) "none"
    } else {
	puts "Loading node on-off file..."
	source $opt(onoff)
	puts "Load complete..."
    }

    if { $opt(traf) == "" } {
	puts "*** NOTE: no traffic file specified."
        set opt(traf) "none"
    } else {
	puts "Loading traffic file..."
	source $opt(traf)
	puts "Load complete..."
    }

#
# Tell all the nodes when the simulation ends
#

$ns_ at $opt(prestop) "$ns_ prepare-to-stop"
for {set i 0} {$i < $opt(nn) } {incr i} {
    $ns_ at $opt(stop).000000001 "$node_($i) reset";
}
$ns_ at $opt(stop).00000001 "$self finish; puts \"NS EXITING...\" ; $ns_ halt"


# Define node initial position in nam

for {set i 0} {$i < $opt(nn)} {incr i} {

    # 20 defines the node size in nam, must adjust it according to your scenario
    # The function must be called after mobility model is defined

    $ns_ initial_node_pos $node_($i) 20
}

$ns_ run
}


Test/flooding instproc init {} {
   global opt
   $self instvar ns_ topo god_

    set opt(god)            on
    set opt(traf)	"sk-30-3-3-1-1-6-64.tcl"      ;# traffic file
    set opt(topo)	"scen-800x800-30-500-1.0-1"   ;# topology file
    set opt(onoff)          ""                        ;# node on-off
    set opt(adhocRouting)   FLOODING       
    set opt(duplicate)      "enable-duplicate"

   $self next
}
Test/flooding instproc run {} {
   global opt
   $self instvar ns_ topo god_

    $god_ $opt(god)
    $god_ allow_to_stop
    $god_ num_data_types 1
    
    $ns_ node-config -adhocRouting $opt(adhocRouting) \
		 -llType $opt(ll) \
		 -macType $opt(mac) \
		 -ifqType $opt(ifq) \
		 -ifqLen $opt(ifqlen) \
		 -antType $opt(ant) \
		 -propType $opt(prop) \
		 -phyType $opt(netif) \
		 -channelType $opt(chan) \
		 -topoInstance $topo \
		 -agentTrace ON \
                 -routerTrace ON \
                 -macTrace ON \
		 -energyModel $opt(engmodel) \
		 -initialEnergy $opt(initeng) \
		 -txPower  $opt(txPower) \
		 -rxPower  $opt(rxPower) \
		 -idlePower  $opt(idlePower)

			 
    for {set i 0} {$i < $opt(nn) } {incr i} {
	set node_($i) [$ns_ node $i]
	$node_($i) random-motion 0		;# disable random motion
        $god_ new_node $node_($i)
    }

    if { $opt(topo) == "" } {
	puts "*** NOTE: no topology file specified."
        set opt(topo) "none"
    } else {
	puts "Loading topology file..."
	source $opt(topo)
	puts "Load complete..."
    }

    if { $opt(onoff) == "" } {
	puts "*** NOTE: no node-on-off file specified."
        set opt(onoff) "none"
    } else {
	puts "Loading node on-off file..."
	source $opt(onoff)
	puts "Load complete..."
    }

    if { $opt(traf) == "" } {
	puts "*** NOTE: no traffic file specified."
        set opt(traf) "none"
    } else {
	puts "Loading traffic file..."
	source $opt(traf)
	puts "Load complete..."
    }

#
# Tell all the nodes when the simulation ends
#

$ns_ at $opt(prestop) "$ns_ prepare-to-stop"
for {set i 0} {$i < $opt(nn) } {incr i} {
    $ns_ at $opt(stop).000000001 "$node_($i) reset";
}
$ns_ at $opt(stop).00000001 "$self finish ; puts \"NS EXITING...\" ; $ns_ halt"


# Define node initial position in nam

for {set i 0} {$i < $opt(nn)} {incr i} {

    # 20 defines the node size in nam, must adjust it according to your scenario
    # The function must be called after mobility model is defined

    $ns_ initial_node_pos $node_($i) 20
}

$ns_ run
}


proc finish {} {

    set ns_ [Simulator instance]
    set god_ [God instance]

    global quiet

    $ns_ terminate-all-agents 
    $god_ dump_num_send

    $ns_ flush-trace
    if { !$quiet } {
            puts "running nam..."
            exec nam temp.rands &
    }

    exit 0
}




proc runtest {arg} {
        global quiet
        set quiet 0

	set b [llength $arg]
	if {$b == 1} {
	        set test $arg
        } elseif {$b == 2} {
	        set test [lindex $arg 0]
            if {[lindex $arg 1] == "QUIET"} {
	        set quiet 1
            }
        } else {
	        usage
        }
	switch $test {
	        diff-rate-default -
	        diff-rate-other -
	        diff-prob -
	        omnimcast -
		flooding {
                     set t [new Test/$test]
                }
                default {
	             stderr "Unknown test $test"
		     exit 1
                }
         }
	 $t run
}

global argv arg0 
runtest $argv









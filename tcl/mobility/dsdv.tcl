# ======================================================================
# Default Script Options
# ======================================================================
Agent/DSDV set sport_        0
Agent/DSDV set dport_        0
Agent/DSDV set wst0_         6        ;# As specified by Pravin
Agent/DSDV set perup_       15        ;# As given in the paper (update period)
Agent/DSDV set use_mac_      0        ;# Performance suffers with this on
Agent/DSDV set be_random_    1        ;# Flavor the performance numbers :)
Agent/DSDV set alpha_        0.875    ;# 7/8, as in RIP(?)
Agent/DSDV set min_update_periods_ 3  ;# Missing perups before linkbreak
Agent/DSDV set myaddr_       0        ;# My address
Agent/DSDV set verbose_      0        ;# 
Agent/DSDV set trace_wst_    0        ;# 



#Class Agent/DSDV

set opt(ragent)		Agent/DSDV
set opt(pos)		NONE			;# Box or NONE

if { $opt(pos) == "Box" } {
	puts "*** DSDV using Box configuration..."
}

# ======================================================================
Agent instproc init args {
        eval $self next $args
}       

Agent/DSDV instproc init args {
        eval $self next $args
}       

# ===== Get rid of the warnings in bind ================================

# ======================================================================

proc create-routing-agent { node id } {
	global ns_ ragent_ tracefd opt

	#
	#  Create the Routing Agent and attach it to port 255.
	#
	#set ragent_($id) [new $opt(ragent) $id]
	set ragent_($id) [new $opt(ragent)]
	set ragent $ragent_($id)
	$ragent set myaddr_ $id
	$node attach $ragent 255

	$ragent set target_ [$node set ifq_(0)]	;# ifq between LL and MAC
        
        # XXX FIX ME XXX
        # Where's the DSR stuff?
	#$ragent ll-queue [$node get-queue 0]    ;# ugly filter-queue hack
	$ns_ at 0.0 "$ragent_($id) start-dsdv"	;# start updates

	#
	# Drop Target (always on regardless of other tracing)
	#
	set drpT [cmu-trace Drop "RTR" $node]
	$ragent drop-target $drpT
	
	#
	# Log Target
	#
	set T [new Trace/Generic]
	$T target [$ns_ set nullAgent_]
	$T attach $tracefd
	$T set src_ $id
	$ragent tracetarget $T
}


proc dsdv-create-mobile-node { id } {
	global ns_ chan prop topo tracefd opt node_
	global chan prop tracefd topo opt
	
	set node_($id) [new Node/MobileNode]

	set node $node_($id)
	$node random-motion 0		;# disable random motion
	$node topography $topo

	#
	# This Trace Target is used to log changes in direction
	# and velocity for the mobile node.
	#
	set T [new Trace/Generic]
	$T target [$ns_ set nullAgent_]
	$T attach $tracefd
	$T set src_ $id
	$node log-target $T

	$node add-interface $chan $prop $opt(ll) $opt(mac)	\
		$opt(ifq) $opt(ifqlen) $opt(netif) $opt(ant)

	#
	# Create a Routing Agent for the Node
	#
	create-routing-agent $node $id

	# ============================================================

	if { $opt(pos) == "Box" } {
		#
		# Box Configuration
		#
		set spacing 200
		set maxrow 7
		set col [expr ($id - 1) % $maxrow]
		set row [expr ($id - 1) / $maxrow]
		$node set X_ [expr $col * $spacing]
		$node set Y_ [expr $row * $spacing]
		$node set Z_ 0.0
		$node set speed_ 0.0

		$ns_ at 0.0 "$node_($id) start"
	}
}

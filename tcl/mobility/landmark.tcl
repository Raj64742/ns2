# ======================================================================
# Default Script Options
# ======================================================================
Agent/landmark set sport_        0
Agent/landmark set dport_        0
Agent/landmark set be_random_    1        ;# Flavor the performance numbers :)
Agent/landmark set verbose_      0        ;# 
Agent/landmark set trace_wst_    0        ;# 
Agent/landmark set debug_        1


#Class Agent/DSDV

set opt(ragent)		Agent/landmark
set opt(pos)		NONE			;# Box or NONE

if { $opt(pos) == "Box" } {
	puts "*** Landmark using Box configuration..."
}

# ======================================================================
Agent instproc init args {
        eval $self next $args
}       

Agent/landmark instproc init args {
        eval $self next $args
}       

# ===== Get rid of the warnings in bind ================================

# ======================================================================

proc create-landmark-agent { node id tag_dbase } {
    global ns_ ragent_ tracefd opt

    #
    #  Create the Routing Agent and attach it to port 255.
    #
    #set ragent_($id) [new $opt(ragent) $id]
    set ragent_($id) [new $opt(ragent)]
    set ragent $ragent_($id)

    ## setup address (supports hier-addr) for Landmark agent and mobilenode
    set addr [$node node-addr]
    $ragent addr $addr
    $node addr $addr
    
    $node attach $ragent 255

    ##$ragent set target_ [$node set ifq_(0)]	;# ifq between LL and MAC
    
    # Add a pointer to node so that agents can get location information
    $ragent node $node
        
    # XXX FIX ME XXX
    # Where's the DSR stuff?
    #$ragent ll-queue [$node get-queue 0]    ;# ugly filter-queue hack
    $ns_ at 0.0 "$ragent_($id) start"	;# start updates
    $ns_ at $opt(stop) "$ragent_($id) dumprtab"
    $ns_ at $opt(stop) "$ragent_($id) print-nbrs"

    
    # Unicast advertisement once the hierarchy is built
#    if {$opt(adverts-type) == "Unicast"} {
#	    $ragent unicast-adverts
#    }


#    if {$opt(adverts-type) == "HardState"} {
#	    $ragent hard-state-adverts
#    }

     if {$opt(update-period) > 0} {
 	    $ragent set-update-period $opt(update-period)
     }

#    if {$opt(update-timeout) > 0} {
#	    $ragent set-update-timeout $opt(update-timeout)
#    }

    #
    # Set-up link to global tag database
    #
    $ragent attach-tag-dbase $tag_dbase

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



proc create-query-agent { node id tag_dbase} {
    global ns_ qryagent_ tracefd opt

    set qryagent_($id) [new Agent/SensorQuery]
    set addr [$node node-addr]
    $qryagent_($id) addr $addr

    $node attach $qryagent_($id) 0
	
#    $ns_ at 2000.0 "$qryagent_($id) start-queries"

    #
    # Set-up link to global tag database
    #
    $qryagent_($id) attach-tag-dbase $tag_dbase

    #
    # Log Target
    #
    set T [new Trace/Generic]
    $T target [$ns_ set nullAgent_]
    $T attach $tracefd
    $T set src_ $id
    $qryagent_($id) tracetarget $T
}



proc landmark-create-mobile-node { id tag_dbase } {
	global ns ns_ chan prop topo tracefd opt node_
	global chan prop tracefd topo opt
	
	set ns_ [Simulator instance]
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
        # Add notion of energy to node
        #
        if [info exists opt(energymodel)] {
                $node addenergymodel [new $opt(energymodel) $opt(initialenergy)]
        }


	#
	# Create hierarchical landmark routing agent for the Node
	#
	create-landmark-agent $node $id $tag_dbase


	#
	# Create query agent for each sensor node
	#
	create-query-agent $node $id $tag_dbase

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










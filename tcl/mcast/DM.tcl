Class DM -superclass McastProtocol

DM set PruneTimeout 0.5

DM instproc init { sim node } {
	$self next
	$self instvar ns Node type
	set ns $sim
	set Node $node
	set type "DM"
	$self initialize
        set tracefile [$ns gettraceAllFile]
        if { $tracefile != 0 } {
	    $self trace $ns $tracefile $node
	}
}

DM instproc initialize { } {
	$self instvar Node prune
        # puts "initialize DM-like: creating prune msg agents"
	set prune [new Agent/Mcast/Prune $self]
        [$Node getArbiter] addproto $self
	$Node attach $prune
}

DM instproc join-group  { group } {
        $self instvar group_
        set group_ $group
        $self next
	$self instvar Node ns

	# puts "_node [$Node id], joining group $group"
	set listOfReps [$Node getRepByGroup $group]
	foreach r $listOfReps {
		if ![$r is-active] {
			# puts "SENDING GRAFTS... ####"
			$self send-ctrl graft [$r set srcID_] $group
		}
	}
}

DM instproc leave-group { group } {
        $self instvar group_
        set group_ $group
        $self next
}

DM instproc handle-cache-miss { argslist } {
        set srcID [lindex $argslist 0]
        set group [lindex $argslist 1]
        set iface [lindex $argslist 2]

        # puts "$self handel-cache-miss $srcID $group $iface"
        $self instvar Node 

	set neighbor [$Node set neighbor_]
        # init a list of lan indexes
        set indexList ""
	set oiflist ""
	set id [$Node id]

        # in the future this should be foreach iface $interfaces
        foreach node $neighbor {
           set nbr [$node id]
           set oifInfo [$Node RPF-interface $srcID $id $nbr]
           if { $oifInfo != "" } {
                set index [lindex $oifInfo 0]
                set oif [lindex $oifInfo 1]
                set k [lsearch -exact $indexList $index]
                if { $k < 0 } {
                        lappend indexList $index
                        lappend oiflist $oif
                }
           }
        }
	$Node add-mfc $srcID $group -1 $oiflist
}

DM instproc drop { replicator src dst } {
	$self instvar Node

	#
        # No downstream listeners
        # Send a prune back toward the source
        #
        if { $src == [$Node id] } {
                #
                # if we are trying to prune ourself (i.e., no
                # receivers anywhere in the network), set the
                # ignore bit in the object (we turn it back on
                # when we get a graft).  This prevents this
                # drop methood from being called on every packet.
                #
		$replicator set ignore 1
        } else {
	        $self send-ctrl prune $src $dst
        }
}

DM instproc recv-prune { from src group } {
        $self instvar Node PruneTimer_ ns

	# puts "_node [$Node id], recv prune from $from, src $src, group $group"
	set r [$Node getRep $src $group]
	if { $r == "" } { 
		return 0
	}

	set oifInfo [$Node RPF-interface $src [$Node id] $from]
	set tmpoif [$Node set outLink_([$Node get-oifIndex $from])]
	$r instvar active_
	if [$r exists $tmpoif] {
	    if !$active_($tmpoif) {
		#puts "recv prune when iface is already pruned"
		#$ns cancel $PruneTimer_($src:$group:$tmpoif)
		#set PruneTimer_($src:$group:$tmpoif) [$ns at [expr [$ns now] + [DM set PruneTimeout]] "$r enable $tmpoif"]
	    } else {
		# puts "prune oif $tmpoif [$ns now]"
		$r disable $tmpoif
		set PruneTimer_($src:$group:$tmpoif) [$ns at [expr [$ns now] + [DM set PruneTimeout]] "$r enable $tmpoif"]
	    }
	} else {
	    puts "warning: try to prune interface not existing?"
	}

        #
        # If there are no remaining active output links
        # then send a prune upstream.
        #
        $r instvar nactive_
        if {$nactive_ == 0} {
	    # set src [expr $src >> 8]
	    if { $src != [$Node id] } {
		$self send-ctrl prune $src $group
	    }
	}
}

DM instproc recv-graft { from src group } {
        $self instvar Node PruneTimer_ ns
        #puts "_node [Node id], RECV-GRAFT from $from src $src group $group"
	set id [$Node id]
        set r [$Node set replicator_($src:$group)]
        if { ![$r is-active] && $src != $id } {
                #
                # the replicator was shut-down and the
                # source is still upstream so propagate
                # the graft up the tree
                #
                $self send-ctrl graft $src $group
        }
        #
        # restore the flow
        #
	set tmpoif [$Node set outLink_([$Node get-oifIndex $from])] 
	$r instvar active_
	if {[$r exists $tmpoif] && !($active_($tmpoif))} {
	    $ns cancel $PruneTimer_($src:$group:$tmpoif)
	}
        $r enable $tmpoif
}

#
# send a graft/prune for src/group up the RPF tree
#
DM instproc send-ctrl { which src group } {
        $self instvar prune ns Node
	set id [$Node id]
	#puts "_node $id, send ctrl $which, src $src, group $group"
        set nbr [$ns upstream-node $id $src]
	$ns connect $prune [[[$nbr getArbiter] getType [$self info class]] set prune]
        if { $which == "prune" } {
                $prune set class_ 30
        } else {
                $prune set class_ 31
        }        
        $prune send $which $id $src $group
}

Agent/Mcast/Prune instproc init { protocol } {
	$self next
	$self instvar proto
	set proto $protocol
}
 
Agent/Mcast/Prune instproc handle {type from src group} {
	$self instvar proto 
	# puts "_node [[$proto set Node] id], prune agent handle"
        eval $proto recv-$type $from $src $group 
}

#####
Simulator instproc gettraceAllFile {} {
        $self instvar traceAllFile_
        if [info exists traceAllFile_] {
	    return $traceAllFile_
	} else {
	    return 0
	}
}
Class DM -superclass McastProtocol

DM instproc init { sim node } {
	$self next
	$self instvar ns Node type
	set ns $sim
	set Node $node
	set type "DM"
	$self initialize
}

DM instproc initialize { } {
	$self instvar Node prune
	set prune [new Agent/Message/Prune $self]
        [$Node getArbiter] addproto $self
	$Node attach $prune
}

DM instproc join-group  { group } {
	$self instvar Node
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
        $self instvar Node

	#puts "_node [$Node id], recv prune from $from, src $src, group $group"
	set r [$Node getRep $src $group]
	if { $r == "" } { 
		return 0
	}
        $r disable [$Node set outLink_([$Node get-oifIndex $from])]
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
        $self instvar Node
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
        $r enable [$Node set outLink_([$Node get-oifIndex $from])]
}

#
# send a graft/prune for src/group up the RPF tree
#
DM instproc send-ctrl { which src group } {
        $self instvar prune ns Node
	set id [$Node id]
	#puts "_node $id, send ctrl $which, src $src, group $group"
        set nbr [$ns upstream-node $id $src]
	$ns connect $prune [[[$nbr getArbiter] getType "DM"] set prune]
        if { $which == "prune" } {
                $prune set class_ 30
        } else {
                $prune set class_ 31
        }        
        $prune send "$which/$id/$src/$group"
}



Class Agent/Message/Prune -superclass Agent/Message

Agent/Message/Prune instproc init { protocol } {
	$self next
	$self instvar proto
	set proto $protocol
}
 
Agent/Message/Prune instproc handle msg {
	$self instvar proto 
	# puts "_node [[$proto set Node] id], prune agent handle"
        set L [split $msg /]
        set type [lindex $L 0]
        set from [lindex $L 1]
        set src [lindex $L 2]
        set group [lindex $L 3]
        $self instvar node
        if { $type == "prune" } {
                $proto recv-prune $from $src $group 
        } else {
                $proto recv-graft $from $src $group
        }
}

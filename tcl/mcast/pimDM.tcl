Class pimDM -superclass DM

pimDM instproc init { sim node } {
	$self instvar ns Node type
	set ns $sim
	set Node $node
	set type "pimDM"
	$self initialize
        set tracefile [$ns gettraceAllFile]
        if { $tracefile != 0 } {
	    $self trace $ns $tracefile $node
	}
}

pimDM instproc handle-cache-miss { argslist } {
        set srcID [lindex $argslist 0]
        set group [lindex $argslist 1]
        set iface [lindex $argslist 2]

        # puts "$self handel-cache-miss $srcID $group $iface"
        $self instvar Node ns iif_
	set neighbor [$Node set neighbor_]
        # init a list of lan indexes
        set indexList ""
	set oiflist ""
	set id [$Node id]

        # in the future this should be foreach iface $interfaces
        set upstream [$ns upstream-node $id $srcID]
        foreach node $neighbor {
	    set nbr [$node id]
	    if { [$upstream id] != $nbr } {
		set oifInfo [$Node get-oif [$ns set link_($id:$nbr)]]
		set index [lindex $oifInfo 0]
		set oif [lindex $oifInfo 1]
		set k [lsearch -exact $indexList $index]
		if { $k < 0 } {
		    lappend indexList $index
		    lappend oiflist $oif
		}
	    }
	}
	# got to find iface
	if {$srcID == $id} {
	    set iif -2
	} else {
	    set iilink [$ns RPF-link $srcID [$upstream id] $id]
	    set iif [[$iilink set dest_] id]
	}
	set iif_($srcID) $iif
	$Node add-mfc $srcID $group $iif $oiflist
}

pimDM instproc handle-wrong-iif { argslist } {
    $self instvar Node ns iif_
    set srcID [lindex $argslist 0]
    set group [lindex $argslist 1]
    set iface [lindex $argslist 2]
    # puts "warning: pimDM $self wrong incoming interface src:$srcID group:$group iface:$iface"

    set id [$Node id]
    set nbr [$Node ifaceGetNode $iface]
    if {[$ns upstream-node $id $srcID] == $nbr} {
	set r [$Node getRep $srcID $group] 
	set newoifnode [$Node ifaceGetNode $iif_($srcID)]
	if {$newoifnode > -1} {
	    $r insert [lindex [$Node get-oif [$ns set link_($id:[$newoifnode id])]] 1]
	}
	$r change-iface $srcID $group $iif_($srcID) $iface
	set iif_($srcID) $iface
    } else {
	$self send-ctrl-wrong-iface prune $srcID $group $iface
    }
    
}

pimDM instproc send-ctrl-wrong-iface { which src group iface } {
        $self instvar prune ns Node
	set id [$Node id]
	# puts "wrong-iface: _node $id, send ctrl $which, src $src, group $group"
        set nbr [$Node ifaceGetNode $iface]
	$ns connect $prune [[[$nbr getArbiter] getType [$self info class]] set prune]
        if { $which == "prune" } {
                $prune set class_ 30
        } else {
                $prune set class_ 31
        }        
        $prune send $which $id $src $group
}


Class dynamicDM -superclass DM

dynamicDM set ReportRouteTimeout 1
dynamicDM set periodic 0

dynamicDM instproc init { sim node } {
	$self instvar ns Node type
	set ns $sim
	set Node $node
	set type "dynamicDM"
	$self initialize
        set tracefile [$ns gettraceAllFile]
        if { $tracefile != 0 } {
	    $self trace $ns $tracefile $node
	}
}

dynamicDM instproc start { } {
        $self check-downstream-list
}

dynamicDM instproc check-downstream-list { } {
        $self instvar ns DownstreamList_ Node PruneTimer_ iif_

        # puts "[$ns now] check downstream list"
	set neighbor [$Node set neighbor_]
	set id [$Node set id_]


        set i 0
	set n [Node set nn_]
	while { $i < $n } {
	    if {$i != $id} {
		set oiflist ""
		# puts "$i"
		set n1 [[$ns set Node_($i)] set id_]
		foreach node $neighbor {
		    set nbr [$node id]
		    set oifInfo [$Node RPF-interface $n1 $id $nbr]
		    if { $oifInfo != "" } {
			set oif [lindex $oifInfo 1]
			set k [lsearch -exact $oiflist $oif]
			if { $k < 0 } {
			    lappend oiflist $oif
			}
		    }
		}

		#update iif
		if [info exists iif_($n1)] {
		    set oldiface $iif_($n1)
		}
		set upstream [$ns upstream-node $id $n1]	
		if { $upstream != ""} {
		    set iilink [$ns RPF-link $n1 [$upstream id] $id]
		    set iif_($n1) [[$iilink set dest_] id]
		}
		if [info exists oldiface] {
		    foreach pair [$Node getRepBySource $n1] {
			set pair [split $pair :]
			set group [lindex $pair 0]
			set r [lindex $pair 1]
			if {$oldiface != $iif_($n1)} {
			    $r change-iface $n1 $group $oldiface $iif_($n1)
			}
		    }
		}

		if [info exists DownstreamList_($n1)] {
		    set oldlist $DownstreamList_($n1)
		    # puts "$oldlist"
		}
		set DownstreamList_($n1) [lsort -increasing $oiflist]
		# puts "$DownstreamList_($n1)"

		if [info exists oldlist] {
		    set m [llength $oldlist]
		    set n [llength $DownstreamList_($n1)]
		    # puts "$m, $n"
		    set j 0
		    set k 0
		    while { $j < $m || $k < $n} {
			set old [lindex $oldlist $j]
			set new [lindex $DownstreamList_($n1) $k]
			if {($n == $k) || (($m != $j) && ($old < $new))} {
			    foreach pair [$Node getRepBySource $n1] {
				set pair [split $pair :]
				set group [lindex $pair 0]
				set r [lindex $pair 1]
				if [$r exists $old] {
				    if [$r set active_($old)] {
					$r disable $old
				    } else {
					$ns cancel $PruneTimer_($n1:$group:$old)
				    }
				} else {
				    puts "panic: why $old not in old downstream list"
				}
			    }
			    incr j
			} elseif {$m == $j || ($n != $k && $old > $new)} {
			    foreach pair [$Node getRepBySource $n1] {
				set pair [split $pair :]
				set r [lindex $pair 1]
				$r insert $new
			    }
			    incr k
			} else {
			    incr j
			    incr k
			}
		    }
		}
	    }
	    incr i  
	}
	if [dynamicDM set periodic] {
	    $ns at [expr [$ns now] + [dynamicDM set ReportRouteTimeout]] "$self check-downstream-list"
	}
}

dynamicDM instproc handle-cache-miss { argslist } {
        set srcID [lindex $argslist 0]
        set group [lindex $argslist 1]
        set iface [lindex $argslist 2]

        # puts "$self handel-cache-miss $srcID $group $iface"
        $self instvar Node ns
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
	# got to find iface
	if {$srcID == $id} {
	    set iif -2
	} else {
	    set upstream [$ns upstream-node $id $srcID]	
	    set iilink [$ns RPF-link $srcID [$upstream id] $id]
	    set iif [[$iilink set dest_] id]
	}
	$Node add-mfc $srcID $group $iif $oiflist
}

dynamicDM instproc handle-wrong-iif { argslist } {


}

dynamicDM instproc send-ctrl { which src group } {
        $self instvar prune ns Node
	set id [$Node id]
	# puts "_node $id, send ctrl $which, src $src, group $group"
        set nbr [$ns upstream-node $id $src]
	$ns connect $prune [[[$nbr getArbiter] getType "dynamicDM"] set prune]
        if { $which == "prune" } {
                $prune set class_ 30
        } else {
                $prune set class_ 31
        }        
        $prune send "$which/$id/$src/$group"
}

########## CtrMcastComp Class ##################################
########## init {}
########## compute-tree {src group Ttype}
########## compute-branch {src group member Ttype}
########## exists-Mlist {group}
########## exists-Slist {group}
########## exists-treetype {group}

Class CtrMcastComp

CtrMcastComp instproc init sim {
    $self instvar ns Glist Mlist Slist treetype 
    $self instvar RPT SPT

    set ns $sim
    set Glist ""
    set SPT 1
    set RPT 2
}
    
##### Main computation functions #####
CtrMcastComp instproc reset-mroutes {} {
    $self instvar ns Glist Slist

    set i 0
    set n [Node set nn_]
    while { $i < $n } {
        set n1 [$ns set Node_($i)]
	foreach group $Glist {
	    set tmp [$n1 getRepByGroup $group]
	    if {$tmp != ""} {
		foreach r $tmp {
		    $r reset
		}
		$n1 unset repByGroup_($group)
	    }
	    
	    if [info exists Slist($group)] {
		foreach tmp $Slist($group) {
		    if {[$n1 getRep $tmp $group] != ""} {
			$n1 unset replicator_($tmp:$group)
		    }
		}
	    }
	}
	incr i
    }
}

CtrMcastComp instproc compute-mroutes {} {
    $self instvar ns Glist Slist treetype

    $self reset-mroutes
    foreach group $Glist {
	if [info exists Slist($group)] {
	    foreach src $Slist($group) {
		$self compute-tree $src $group
	    }
	}
    }
}

CtrMcastComp instproc compute-tree { src group } {
    $self instvar ns Mlist

    if { [$self exists-Mlist $group] } {
	foreach m $Mlist($group) {
	    $self compute-branch $src $group $m
	}
    }
}


CtrMcastComp instproc compute-branch { src group member } {
    $self instvar ns treetype 
    $self instvar SPT RPT

    #puts "create $src $member mcast entry until merging with an existing one"

    ### set (S,G) join target
    if { $treetype($group) == $SPT } {
	#puts "compute SPT branch: install ($src, $group) cache from $member to $src"
	
	set target $src
    } elseif { $treetype($group) == $RPT } {
	#puts "compute RPT branch"

        set n [$ns set Node_($member)]
	set arbiter [$n getArbiter]
	set ctrmcast [$arbiter getType "CtrMcast"]
	set RP [$ctrmcast getrp $group]
	set target $RP
    }


    set tmp $member
    set downstreamtmp -1

    while { $downstreamtmp != $target } {

	### set iif : RPF link dest id: interface label

	if {$tmp == $target} {
	    if {$treetype($group) == $SPT} {
		set iif -2
	    } else {
		set upstreamtmp [$ns upstream-node $tmp $src]	
		set iilink [$ns RPF-link $src [$upstreamtmp id] $tmp]
		set iif [[$iilink set dest_] id]
	    }
	} else {
	    set upstreamtmp [$ns upstream-node $tmp $target]	
	    set iilink [$ns RPF-link $target [$upstreamtmp id] $tmp]
	    set iif [[$iilink set dest_] id]
	}

	### set oif : RPF link
	set oiflist "" 
	if { $downstreamtmp >= 0 } {
	    set rpflink [$ns RPF-link $target $tmp $downstreamtmp]
	    if { $rpflink != "" } {
		lappend oiflist [$rpflink head]
 	    }
	}

	set node [$ns set Node_($tmp)]
	if [$node exists-Rep $src $group] {
	    if { $oiflist != "" } {
		[$node set replicator_($src:$group)] insert [lindex $oiflist 0]
	    }
	    ### reach merging point
	    return 1
	}

	### hasn't reached merging point, so keep creating (S,G) like a graft
	$node add-mfc $src $group $iif $oiflist
	set downstreamtmp $tmp
	set tmp [[$ns upstream-node $tmp $target] id]
    }
}


CtrMcastComp instproc prune-branch { src group member } {
    $self instvar ns treetype 
    $self instvar SPT RPT

    ### set (S,G) prune target
    if { $treetype($group) == $SPT } {
	#puts "prune SPT branch: remove ($src, $group) cache from $member to $src"
	
	set target $src
    } elseif { $treetype($group) == $RPT } {
	#puts "prune RPT branch"

        set n [$ns set Node_($member)]
	set arbiter [$n getArbiter]
	set ctrmcast [$arbiter getType "CtrMcast"]
	set RP [$ctrmcast getrp $group]
	set target $RP
    }

    set tmp $member
    set downstreamtmp -1

    while { $downstreamtmp != $target } {
	set iif -1

	set oif -1
	if { $downstreamtmp >= 0 } {
	    set rpflink [$ns RPF-link $target $tmp $downstreamtmp]
	    if { $rpflink != "" } {
		set oif [$rpflink head]
	    }
	}

	set node [$ns set Node_($tmp)]
	if {![$node exists-Rep $src $group]} {
	    return 0
	}
	set r [$node set replicator_($src:$group)] 
	if { $oif != -1 } {
	    $r disable $oif
	}

	if [$r is-active] {
	    return 1
	}

	set downstreamtmp $tmp
	set tmp [[$ns upstream-node $tmp $target] id]
    }
    
}


##### utility functions to detect existence of array elements #####
##### for the use of distributed CtrMcast agents              #####
CtrMcastComp instproc exists-Mlist group {
    $self instvar Mlist

    if [info exists Mlist($group)] {
	return 1
    } else {
	return 0
    }
	
}

CtrMcastComp instproc exists-Slist group {
    $self instvar Slist

    if [info exists Slist($group)] {
	return 1
    } else {
	return 0
    }
	
}

CtrMcastComp instproc exists-treetype group {
    $self instvar treetype

    if [info exists treetype($group)] {
	return 1
    } else {
	return 0
    }
	
}

CtrMcastComp instproc switch-treetype group {
    $self instvar treetype SPT Glist

    set group [expr $group]
    set treetype($group) $SPT
    if { [lsearch $Glist $group] < 0 } {
	lappend Glist $group
    }

    $self compute-mroutes
}

###########Classifier/Replicator/Demuxer ###############
Classifier/Replicator/Demuxer instproc reset {} {
    $self instvar nslot_ nactive_ active_ index_
    
    set i 0
    while { $i < $nslot_ } {
	$self clear $i
	incr i
    }
    set nslot_ 0
    set nactive_ 0
    unset active_ index_
}

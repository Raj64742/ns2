 #
 # tcl/ctr-mcast/CtrMcastComp.tcl
 #
 # Copyright (C) 1997 by USC/ISI
 # All rights reserved.                                            
 #                                                                
 # Redistribution and use in source and binary forms are permitted
 # provided that the above copyright notice and this paragraph are
 # duplicated in all such forms and that any documentation, advertising
 # materials, and other materials related to such distribution and use
 # acknowledge that the software was developed by the University of
 # Southern California, Information Sciences Institute.  The name of the
 # University may not be used to endorse or promote products derived from
 # this software without specific prior written permission.
 # 
 # THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 # WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 # MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 # 
 # Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 # 
 #
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
    $self instvar ctrrpcomp dynT_

    set ns $sim
    set Glist ""
    set SPT 1
    set RPT 2
        $ns maybeEnableTraceAll $self {}
}

CtrMcastComp instproc id {} {
    return 0
}

CtrMcastComp instproc trace { f nop {op ""} } {
        $self instvar ns dynT_
        if {$op == "nam" && [info exists dynT_]} {
                foreach tr $dynT_ {
                        $tr namattach $f
                }
        } else {
                lappend dynT_ [$ns create-trace Generic $f $self $self $op]
        }
}

##### Main computation functions #####
CtrMcastComp instproc reset-mroutes {} {
    $self instvar ns Glist Slist

    set i 0
    set n [Node set nn_]
    while { $i < $n } {
        set n1 [$ns set Node_($i)]
	foreach group $Glist {
	    set tmp [$n1 getReps * $group]
	    if {$tmp != ""} {
		$n1 clearReps * $group
	    }
	    
	    if [info exists Slist($group)] {
		foreach tmp $Slist($group) {
		    if {[$n1 getReps $tmp $group] != ""} {
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

    # puts "create $src $member mcast entry until merging with an existing one"

    ### set (S,G) join target
    if { $treetype($group) == $SPT } {
	set target $src
	# puts "compute SPT branch: install ($src, $group) cache from $member to $src"
    } elseif { $treetype($group) == $RPT } {
        set n [$ns set Node_($member)]
	set RP [$self get_rp $n $group]
	set target $RP
	# puts "compute RPT branch, to $RP"
    }


    set tmp $member
    set downstreamtmp -1

    while { $downstreamtmp != $target } {

	### set iif : RPF link dest id: interface label

	if {$tmp == $target} {
	    if {$treetype($group) == $SPT || $tmp == $src} {
		#when the member is also the source
		set iif -1
	    } else {
		#when member is at RP, find iif from RP to source
		set upstreamtmp [[$ns get-node-by-id $tmp] rpf-nbr $src]	
		set iilink [$ns RPF-link $src [$upstreamtmp id] $tmp]
		set iif [[$iilink set iif_] label]
	    }
	} else {
	    set upstreamtmp [[$ns get-node-by-id $tmp] rpf-nbr $target]
	    set iilink [$ns RPF-link $target [$upstreamtmp id] $tmp]
	    set iif [[$iilink set iif_] label]
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
	set r [$node getReps $src $group]
	if { $r != "" } {
	    if [$r is-active] {
		# puts "reach merging point, r exist & active"
		if { $oiflist != "" } {
		    $r insert [lindex $oiflist 0]
		}
		return 1
	    } else {
		# puts "hasn't reached merging point, so continue to insert the oif"
		if { $oiflist != "" } {
		    $r insert [lindex $oiflist 0]
		}
	    }
	} else {
	    # puts "hasn't reached merging point, so keep creating (S,G) like a graft, $src, $group, $iif, $oiflist"
	    $node add-mfc $src $group $iif $oiflist
	}

	set downstreamtmp $tmp
	set tmp [[[$ns get-node-by-id $tmp] rpf-nbr $target] id]
    }
}


CtrMcastComp instproc prune-branch { src group member } {
    $self instvar ns treetype 
    $self instvar SPT RPT

    ### set (S,G) prune target
    if { $treetype($group) == $SPT } {
	# puts "prune SPT branch: remove ($src, $group) cache from $member to $src"
	
	set target $src
    } elseif { $treetype($group) == $RPT } {
	# puts "prune RPT branch"

        set n [$ns set Node_($member)]
	set RP [$self get_rp $n $group]
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
	set r [$node getReps $src $group]
	if { $r == "" } {
	    return 0
	}
	if { $oif != -1 } {
	    $r disable $oif
	}

	if [$r is-active] {
	    return 1
	}

	set downstreamtmp $tmp
	set tmp [[[$ns get-node-by-id $tmp] rpf-nbr $target] id]
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
    $self instvar treetype SPT Glist dynT_
    set group [expr $group]

    if [info exists dynT_] {
	foreach tr $dynT_ {
	    $tr annotate "$group switch tree type"
	}
    }
    set treetype($group) $SPT
    if { [lsearch $Glist $group] < 0 } {
	lappend Glist $group
    }

    $self compute-mroutes
}

CtrMcastComp instproc set_c_rp { nodeList } {
    $self instvar ns
    
    foreach n [$ns all-nodes-list] {
	set arbiter [$n getArbiter]	   
	set ctrmcast [$arbiter getType "CtrMcast"]
	$ctrmcast unset_c_rp
    }

    foreach node $nodeList {
	set arbiter [$node getArbiter]	   
	set ctrmcast [$arbiter getType "CtrMcast"]
	$ctrmcast set_c_rp
    }
}

CtrMcastComp instproc set_c_bsr { nodeList } {
    foreach node $nodeList {
	set tmp [split $node :]
	set node [lindex $tmp 0]
	set prior [lindex $tmp 1]
	set arbiter [$node getArbiter]
	set ctrmcast [$arbiter getType "CtrMcast"]
	$ctrmcast set_c_bsr $prior
    }
}

CtrMcastComp instproc get_rp { node group } {
    set arbiter [$node getArbiter]
    set ctrmcast [$arbiter getType "CtrMcast"]
    $ctrmcast get_rp $group
}

CtrMcastComp instproc get_bsr { node } {
    set arbiter [$node getArbiter]
    set ctrmcast [$arbiter getType "CtrMcast"]
    $ctrmcast get_bsr
}

############# notify(): adapt to rtglib dynamics ####################
CtrMcastComp instproc notify {} {
    $self instvar ctrrpcomp

    ### need to add a delay before recomputation
    $ctrrpcomp compute-rpset
    $self compute-mroutes
}






########## CtrMcast Class: Individual Node join-group, leave-group, etc #####
Class CtrMcast -superclass McastProtocol

CtrMcast instproc init { sim node agent confArgs} {
    $self next
    $self instvar ns Node type Agent
    $self instvar c_rp c_bsr rpset priority
    $self instvar SPT RPT default decapagent
    $self instvar MRT

    set ns $sim
    set Node $node
    set type "CtrMcast"
    set Agent $agent
    set SPT 1
    set RPT 2
    set default $RPT

    [$Node getArbiter] addproto $self
    set decapagent [new Agent/Message/DecapAgent]
    $ns attach-agent $Node $decapagent

    ### config PIM nodes
    if { ! [set len [llength $confArgs]] } { return 0 }

    set c_rp [lindex $confArgs 0]
    if { $len == 1 } { return 1 }
    set c_bsr [lindex $confArgs 1]
    if { $len == 2 } { 
	set priority 0
	return 1
    }
    set priority [lindex $confArgs 2]
}

CtrMcast instproc join-group  { group } {
    $self instvar Node ns Agent
    $self instvar SPT RPT default
    #puts "_node [$Node id], joining group $group"

    if {![$Agent exists-treetype $group] } {
	$Agent set treetype($group) $default
	set tmp [$Agent set Glist]
	if { [lsearch $tmp $group] < 0 } {
	    lappend tmp $group
	    $Agent set Glist $tmp
	}
    }

    ### add new member to a global group member list
    if [$Agent exists-Mlist $group] { 
	### add into Mlist
	set tmp [$Agent set Mlist($group)]
	lappend tmp [$Node id] 
	$Agent set Mlist($group) $tmp
    } else { 
	### create Mlist if not existing
	$Agent set Mlist($group) "[$Node id]" 
    }

    ### puts "JOIN-GROUP: compute branch acrding to tree type"
    if [$Agent exists-Slist $group] {
	foreach s [$Agent set Slist($group)] {
	    $Agent compute-branch $s $group [$Node id]
	}
    }
}

CtrMcast instproc leave-group  { group } {
    $self instvar Node ns Agent default
    #puts "_node [$Node id], leaving group $group"

    if [$Agent exists-Mlist $group] { 

	set k [lsearch [$Agent set Mlist($group)] [$Node id]]

	### find the member in Mlist
	if { $k >= 0 } {

	    ### remove from Mlist
	    $Agent set Mlist($group) [lreplace \
		    [$Agent set Mlist($group)] $k $k]

	    ### reset group tree type when no members
	    if { [$Agent set Mlist($group)] == "" } {
		#$Agent set treetype($group) $default
		set tmp [$Agent set Glist]
		set k [lsearch $tmp $group]
		$Agent set Glist [lreplace $tmp $k $k]
	    }

	    ### prune off branches
	    foreach s [$Agent set Slist($group)] {
		$Agent prune-branch $s $group [$Node id]
	    }

	} else {
	    puts "panic: leaving group that doesn't contain this node"
	}

    } else { 
	puts "panic: leaving group that doesn't have any member"
    }
}

CtrMcast instproc handle-cache-miss { argslist } {
    $self instvar ns Agent Node
    $self instvar RPT default

    set srcID [lindex $argslist 0]
    set group [lindex $argslist 1]
    set iface [lindex $argslist 2]
    set change 0
            
    # puts "CtrMcast $self handle-cache-miss $argslist"

    if { ![$Agent exists-treetype $group] } {
	$Agent set treetype($group) $default
	set tmp [$Agent set Glist]
	if { [lsearch $tmp $group] < 0 } {
	    lappend tmp $group
	    $Agent set Glist $tmp
	}
    }
    if { [$Node id] == $srcID } {
	if { [$Agent set treetype($group)] == $RPT && $srcID != [$self getrp $group]} {
	    ### create encapsulation agent
	    set encapagent [new Agent/Message/EncapAgent $srcID $group]
	    $ns attach-agent $Node $encapagent

	    ### find decapsulation agent and connect encap and decap agents
	    set RP [$self getrp $group]
	    set n [$ns set Node_($RP)]
	    set arbiter [$n getArbiter]
	    set ctrmcast [$arbiter getType "CtrMcast"]
	    set target [$ctrmcast set decapagent]
	    $ns connect $encapagent $target

	    ### create (S,G,iif=-2) entry
	    set oiflist "$encapagent"
	    $Node add-mfc-reg $srcID $group -2 $oiflist
	    #puts "creat (S,G) oif to register $srcID $group -1 $oiflist"
	}
    
	### add into global source list
	if [$Agent exists-Slist $group] {
	    set k [lsearch [$Agent set Slist($group)] [$Node id]]
	    if { $k < 0 } {
		set tmp [$Agent set Slist($group)]
		lappend tmp [$Node id] 
		$Agent set Slist($group) $tmp
		set change 1
	    }
	} else { 
	    $Agent set Slist($group) "[$Node id]" 
	    set change 1
	}

	### decide what tree to build acrding to tree type
	if { $change } {
	    ### new source, so compute tree
	    $Agent compute-tree [$Node id] $group
	    #puts "CACHE-MISS: compute-tree [$Node id] $group"
	}
    }
}

CtrMcast instproc drop  { replicator src group } {
    #puts "drop"
}

##### Two functions to help get RP for a group #####
##### getrp {group}                            #####
##### hash {rp group}                          #####
CtrMcast instproc getrp group {
    $self instvar rpset

    if { $rpset != ""} {
	set returnrp -1
	set hashval -1
	foreach rp $rpset {
	    if {[$self hash $rp $group] > $hashval} {
		set hashval [$self hash $rp $group]
		set returnrp $rp
	    }
	}
	return $returnrp
    } else {
	return -1
    }
}

CtrMcast instproc hash {rp group} {
    return $rp
}



################# Agent/Message messagers ###############
Class Agent/Message/EncapAgent -superclass Agent/Message

Agent/Message/EncapAgent instproc init { src grp } {
        $self next
        $self instvar source group

        set source $src
        set group $grp
        $self set class_ 1
}


Agent/Message/EncapAgent instproc handle msg {
    $self instvar source group
    $self send "$source/$group"
}


Class Agent/Message/DecapAgent -superclass Agent/Message

Agent/Message/DecapAgent instproc handle msg {
    $self instvar addr_ dst_

    set L [split $msg /]
    $self set addr_ [expr [lindex $L 0] << 8 ]
    $self set dst_ [lindex $L 1]
    set mesg [lindex $L 2]
    $self send "$mesg"
}


#################### MultiNode: add-mfc-reg ################


MultiNode instproc add-mfc-reg { src group iif oiflist } {
    $self instvar multiclassifier_ Regreplicator_

    #XXX node addr is in upper 24 bits

    if [info exists Regreplicator_($group)] {
	foreach oif $oiflist {
	    $Regreplicator_($group) insert $oif
	}
	return 1
    }
    set r [new Classifier/Replicator/Demuxer]
    $r set srcID_ $src
    set Regreplicator_($group) $r

    $r set node_ $self

    foreach oif $oiflist {
	$r insert $oif
    }

    # Install the replicator.  We do this only once and leave
    # it forever.  Since prunes are data driven, we want to
    # leave the replicator in place even when it's empty since
    # the replicator::drop callback triggers the prune.
    #
    $multiclassifier_ add-rep $r $src $group $iif
}

MultiNode instproc getRegreplicator group {
    $self instvar Regreplicator_
    if [info exists Regreplicator_($group)] {
	return $Regreplicator_($group)
    } else {
	return -1
    }
}



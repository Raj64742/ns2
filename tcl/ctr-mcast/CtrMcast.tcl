 #
 # tcl/ctr-mcast/CtrMcast.tcl
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
########## CtrMcast Class: Individual Node join-group, leave-group, etc #####
Class CtrMcast -superclass McastProtocol

CtrMcast instproc init { sim node agent confArgs } {
    $self next $sim $node
    $self instvar ns Node type Agent
    $self instvar c_rp c_bsr rpset priority
    $self instvar SPT RPT default decapagent

    set ns $sim
    set Node $node
    set type "CtrMcast"
    if {$agent != 0} {
	set Agent $agent
    } else {	
	set Agent [$ns set MrtHandle_]
    }	
    set SPT 1
    set RPT 2
    set default $RPT

    set c_rp      1
    set c_bsr     1
    set priority  0

    set decapagent [new Agent/CtrMcast/Decap]
    $ns attach-agent $Node $decapagent

    ### config PIM nodes
    if ![info exists confArgs] { return 0 }
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
    $self instvar group_
    set group_ $group
    $self next $group
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
    $self instvar group_
    set group_ $group
    $self next $group
    $self instvar Node ns Agent default
    # puts "_node [$Node id], leaving group $group"

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
	    if [$Agent exists-Slist $group] {
		foreach s [$Agent set Slist($group)] {
		    $Agent prune-branch $s $group [$Node id]
		}
	    }

	} else {
	    puts "panic: leaving group that doesn't contain this node"
	}

    } else { 
	puts "panic: leaving group that doesn't have any member"
    }
}

CtrMcast instproc handle-cache-miss { srcID group iface } {
    $self instvar ns Agent Node
    $self instvar RPT default
    set change 0
            
    if { ![$Agent exists-treetype $group] } {
	$Agent set treetype($group) $default
	set tmp [$Agent set Glist]
	if { [lsearch $tmp $group] < 0 } {
	    lappend tmp $group
	    $Agent set Glist $tmp
	}
    }
    if { [$Node id] == $srcID } {
	set RP [$self get_rp $group]
	if { [$Agent set treetype($group)] == $RPT && $srcID != $RP} {
	    ### create encapsulation agent
	    set encapagent [new Agent/CtrMcast/Encap]
	    $ns attach-agent $Node $encapagent

	    ### find decapsulation agent and connect encap and decap agents
	    set n [$ns set Node_($RP)]
	    set arbiter [$n getArbiter]
	    set ctrmcast [$arbiter getType "CtrMcast"]
	    $ns connect $encapagent [$ctrmcast set decapagent]

	    ### create (S,G,iif=-2) entry
	    set oiflist "$encapagent"
	    $Node add-mfc-reg $srcID $group -1 $oiflist
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

CtrMcast instproc drop  { replicator src group iface} {
    #packets got dropped only due to null oiflist
    #puts "drop"
}

CtrMcast instproc handle-wrong-iif { srcID group iface } {
    puts "warning: $self wrong incoming interface src:$srcID group:$group iface:$iface"

}
##### Two functions to help get RP for a group #####
##### get_rp {group}                            #####
##### hash {rp group}                          #####
CtrMcast instproc get_rp group {
    $self instvar rpset Agent

    if [info exists rpset] {
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
    } else {
	[$Agent set ctrrpcomp] compute-rpset
	set tmp [$self get_rp $group]
	return $tmp
    }
}

CtrMcast instproc hash {rp group} {
    return $rp
}

CtrMcast instproc get_bsr {} {
    puts "CtrMcast doesn't require a BSR"
}

CtrMcast instproc set_c_bsr { prior } {
    $self instvar c_bsr priority
    set c_bsr 1
    set priority $prior
}

CtrMcast instproc set_c_rp {} {
    $self instvar c_rp
    set c_rp 1
}

CtrMcast instproc unset_c_rp {} {
    $self instvar c_rp
    set c_rp 0
}

################# Agent/CtrMcast/Encap ###############

Agent/CtrMcast/Encap instproc init {} {
        $self next
        $self instvar fid_
        
        set fid_ 1
}


#################### MultiNode: add-mfc-reg ################

Node instproc add-mfc-reg { src group iif oiflist } {
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
    # puts "add-rep in add-mcf-reg: $src $group $iif"
    $multiclassifier_ add-rep $r $src $group $iif
}

Node instproc getRegreplicator group {
    $self instvar Regreplicator_
    if [info exists Regreplicator_($group)] {
	return $Regreplicator_($group)
    } else {
	return -1
    }
}




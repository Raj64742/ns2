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

CtrMcast instproc init { sim node agent args } {
	$self next $sim $node
	$self instvar node_ type_ agent_
	$self instvar c_rp_ c_bsr_ rpset_ priority_
	$self instvar defaultTree_ decapagent_

	if {$agent != 0} {
		set agent_ $agent
	} else {	
		set agent_ [$ns_ set MrtHandle_]
	}
	set defaultTree_ RPT

	# should switch to Yuri's agents
	set decapagent_ [new Agent/CtrMcast/Decap]
	$ns_ attach-agent $node_ $decapagent_

	### config PIM nodes
	set c_rp      1
	set c_bsr     1
	set priority  0
	set len [llength $args]
	if { $len >= 1 } {
		set c_rp [lindex $args 0]
	}
	if { $len >= 2 } {
		set c_bsr [lindex $args 1]
	}
	if { $len >= 3 } {
		set priority [lindex $args 2]
	}
}

CtrMcast instproc join-group  { group } {
	$self next $group
	$self instvar node_ ns_ agent_
	$self instvar defaultTree_

	if {![$agent_ exists-treetype $group] } {
		$agent_ set treetype($group) $defaultTree_
		set tmp [$agent_ set Glist]
		if { [lsearch $tmp $group] < 0 } {
			lappend tmp $group
			$agent_ set Glist $tmp
		}
	}

	### add new member to a global group member list
	if [$agent_ exists-Mlist $group] { 
		### add into Mlist
		set tmp [$agent_ set Mlist($group)]
		lappend tmp [$node_ id] 
		$agent_ set Mlist($group) $tmp
	} else { 
		### create Mlist if not existing
		$agent_ set Mlist($group) "[$node_ id]" 
	}

	### puts "JOIN-GROUP: compute branch acrding to tree type"
	if [$agent_ exists-Slist $group] {
		foreach s [$agent_ set Slist($group)] {
			$agent_ compute-branch $s $group [$node_ id]
		}
	}
}

CtrMcast instproc leave-group  { group } {
	$self next $group
	$self instvar node_ ns_ agent_ defaultTree_
	# puts "_node [$Node id], leaving group $group"

	if ![$agent_ exists-Mlist $group] {
		warn "$elf: leaving group that doesn't have any member"
		return
	}
	set k [lsearch [$agent_ set Mlist($group)] [$node_ id]]
	if { $k <= 0 } {
		warn "$self: leaving group that doesn't contain this node"
		return
	}
	### remove from Mlist
	$agent_ set Mlist($group) [lreplace [$agent_ set Mlist($group)] $k $k]
	### reset group tree type when no members
	if { [$agent_ set Mlist($group)] == "" } {
		#$agent_ set treetype($group) $default
		set tmp [$agent_ set Glist]
		set k [lsearch $tmp $group]
		$agent_ set Glist [lreplace $tmp $k $k]
	}
	### prune off branches
	if [$agent_ exists-Slist $group] {
		foreach s [$agent_ set Slist($group)] {
			$agent_ prune-branch $s $group [$node_ id]
		}
	}
}

CtrMcast instproc handle-cache-miss { srcID group iface } {
	$self instvar ns_ agent_ node_
	$self instvar defaultTree_

	set change 0
	if { ![$agent_ exists-treetype $group] } {
		$agent_ set treetype($group) $defaultTree
		set tmp [$agent_ set Glist]
		if { [lsearch $tmp $group] < 0 } {
			lappend tmp $group
			$agent_ set Glist $tmp
		}
	}
	if { [$node_ id] == $srcID } {
		set RP [$self get_rp $group]
		if { [$agent_ set treetype($group)] == RPT && $srcID != $RP} {
			### create encapsulation agent
			set encapagent [new Agent/CtrMcast/Encap]
			$ns_ attach-agent $node_ $encapagent

			### find decapsulation agent and connect encap and decap agents
			set n [$ns_ set node_($RP)]
			set arbiter [$n getArbiter]
			set ctrmcast [$arbiter getType "CtrMcast"]
			$ns_ connect $encapagent [$ctrmcast set decapagent]

			### create (S,G,iif=-2) entry
			$node_ add-mfc-reg $srcID $group -2 $encapagent
			# Why isn't it simply add-mfc?
		}
		
		### add into global source list
		if [$agent_ exists-Slist $group] {
			set k [lsearch [$agent_ set Slist($group)] [$node_ id]]
			if { $k < 0 } {
				set tmp [$agent_ set Slist($group)]
				lappend tmp [$node_ id] 
				$agent_ set Slist($group) $tmp
				set change 1
			}
		} else { 
			$agent_ set Slist($group) "[$node_ id]" 
			set change 1
		}

		### decide what tree to build acrding to tree type
		if { $change } {
			### new source, so compute tree
			$agent_ compute-tree [$node_ id] $group
		}
	}
}

CtrMcast instproc drop  { replicator src group } {
	#packets got dropped only due to null oiflist
}

CtrMcast instproc handle-wrong-iif { srcID group iface } {
	warn "$self: wrong incoming interface <S: $srcID, G: $group, if: $iface>"
}

##### Two functions to help get RP for a group #####
##### get_rp {group}                            #####
##### hash {rp group}                          #####
CtrMcast instproc get_rp group {
	$self instvar rpset_ agent_

	if [info exists rpset_] {
		set returnrp -1
		set hashval -1
		foreach rp $rpset_ {
			if {[$self hash $rp $group] > $hashval} {
				set hashval [$self hash $rp $group]
				set returnrp $rp
			}
		}
		set returnrp		;# return
	} else {
		[$agent_ set ctrrpcomp] compute-rpset
		$self get_rp $group	;# return
	}
}

CtrMcast instproc hash {rp group} {
	return $rp
}

CtrMcast instproc get_bsr {} {
	warn "$self: CtrMcast doesn't require a BSR"
}

CtrMcast instproc set_c_bsr { prior } {
	$self instvar c_bsr_ priority_
	set c_bsr_ 1
	set priority_ $prior
}

CtrMcast instproc set_c_rp_ {} {
	$self instvar c_rp_
	set c_rp_ 1
}

CtrMcast instproc unset_c_rp {} {
	$self instvar c_rp_
	set c_rp_ 0
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




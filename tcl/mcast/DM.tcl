#
# tcl/mcast/DM.tcl
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
# Ported/Modified by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
# 

# DVMRP - like dense mode

Class DM -superclass McastProtocol

DM set PruneTimeout 0.5

DM instproc init { sim node } {
	$self instvar mctrl_
	set mctrl_ [new Agent/Mcast/Control $self]
	$node attach $mctrl_
	Timer/Iface/Prune set timeout [[$self info class] set PruneTimeout]
	$self next $sim $node
}

DM instproc join-group  { group } {
        $self next $group
	$self instvar node_

	set listOfReps [$node_ getReps * $group]
	foreach r $listOfReps {
		if ![$r is-active] {
			$self send-ctrl graft [$r set srcID_] $group
		}
	}
}

DM instproc leave-group { group } {
        $self next $group
}

DM instproc handle-cache-miss { srcID group iface } {
        $self instvar node_ ns_

	set oiflist ""
        foreach nbr [$node_ neighbors] {
		# peek into other nodes' routing tables to simulate 
		# child-parent relationship maintained by dvmrp
		set rpfnbr [$nbr rpf-nbr $srcID]
		if { $rpfnbr == $node_ } {
			set link [$ns_ link $node_ $nbr]
			lappend oiflist [$node_ link2oif $link]
		}
		
	}
	$node_ add-mfc $srcID $group $iface $oiflist
}

DM instproc drop { replicator src dst iface} {
	$self instvar node_ ns_

        if { $iface < 0 } {
                # optimization for sender: if no listeners, set the ignore bit, 
		# so this function isn't called for every packet.
		$replicator set ignore_ 1
        } else {
		set from [[[$node_ iif2link $iface] src] id]
		$self send-ctrl "prune" $src $dst $from
        }
}

DM instproc recv-prune { from src group iface} {
        $self instvar node_ PruneTimer_ ns_

	set r [$node_ getReps $src $group]
	if { $r == "" } { 
		return 0
	}
	set id [$node_ id]
	set tmpoif [$node_ iif2oif $iface]

	if { [$r is-active-target $tmpoif] } {
		$r disable $tmpoif
		if ![$r is-active] {
			if { $src != $id } {
				# propagate prune only if the disabled oif
				# was the last one
				$self send-ctrl prune $src $group
			}
		}
	}
	if ![info exists PruneTimer_($src:$group:$tmpoif)] {
		set PruneTimer_($src:$group:$tmpoif) \
				[new Timer/Iface/Prune $self $src $group $tmpoif $ns_]
	}
	$PruneTimer_($src:$group:$tmpoif) schedule

}

DM instproc recv-graft { from src group iface} {
        $self instvar node_ PruneTimer_ ns_

	set id [$node_ id]
        set r [$node_ set replicator_($src:$group)]
        if { ![$r is-active] && $src != $id } {
                # propagate the graft
                $self send-ctrl graft $src $group
        }
	set tmpoif [$node_ iif2oif $iface]
        $r enable $tmpoif
	if [info exists PruneTimer_($src:$group:$tmpoif)] {
		delete $PruneTimer_($src:$group:$tmpoif)
		unset  PruneTimer_($src:$group:$tmpoif)
	}
}

# send a graft/prune for src/group up to the source or towards $to
DM instproc send-ctrl { which src group { to "" } } {
        $self instvar mctrl_ ns_ node_
	set id [$node_ id]
	set toid $src 
	if { $to != "" } {
		set toid $to
	}
	set nbr [$node_ rpf-nbr $toid]
	$ns_ connect $mctrl_ [[[$nbr getArbiter] getType [$self info class]] set mctrl_]
        if { $which == "prune" } {
                $mctrl_ set class_ 30
        } else {
                $mctrl_ set class_ 31
        }        
        $mctrl_ send $which $id $src $group
}

DM instproc timeoutPrune { oif src grp } {
	$self instvar node_ PruneTimer_ ns_
	set r [$node_ getReps $src $grp]

	$r insert $oif
	if [info exists PruneTimer_($src:$grp:$oif)] {
		delete $PruneTimer_($src:$grp:$oif)
		unset PruneTimer_($src:$grp:$oif)
	}
	return
}


Class Timer/Iface/Prune -superclass Timer/Iface
Timer/Iface/Prune set timeout 0.5

Timer/Iface/Prune instproc timeout {} {
	$self instvar proto src grp oif
	$proto timeoutPrune $oif $src $grp
}




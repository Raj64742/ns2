#
# tcl/mcast/detailedDM.tcl
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
# Contributed by Ahmed Helmy (USC/ISI), http://www-scf.usc.edu/~ahelmy
# 
#
Class detailedDM -superclass DM

detailedDM instproc init { sim node } {
	$self next $sim $node
}

detailedDM instproc start {} {
	$self next
	$self instvar node_ prune_
	$node_ join-group $prune_ [mrtObject getWellKnownGroup ALL_ROUTERS]
}

detailedDM instproc stop {} {
	foreach var {rpfNode_ iifLink_} {
		$self instvar $var
		unset $var
	}
	foreach var {PruneTimer_ DelTimer_ RtxTimer_} {
		$self instvar $var
		if { [info exists $var] } {
			foreach {key val} { [array get $var] } {
				$val cancel
			}
			unset $var
		}
	}
}

#
#
# PACKET FORWARDING TRAP HANDLERS
#
### handle-cache-miss
# The base class, DM, handles cache misses; it invokes:
#	add-mfc [$sel find-iif] [$self find-oifs]
#
detailedDM instproc find-iif-link src {
	$self instvar ns_ node_ iifLink_
	if ![info exists iifLink_($src)] {
		if { $src == [$node_ id] } {
			set iifLink_($src) "SRC"
		} else {
			set iifLink_($src) [$ns_ link [$self rpf-nbr $src] $node_]
		}
	}
	set iifLink_($src)
}

detailedDM instproc find-iif src {
	set iifl [$self find-iif-link $src]
	if { $iifl == "SRC" } {
		return -2
	} else {
		return [$iifl if-label?]
	}
}

detailedDM instproc find-oifs {src grp iif} {
	# may check if iif == iface before creating the entry.. !
        $self instvar ns_ node_

	set nbr [$self rpf-nbr $src]
	if { $node_ == $nbr } {
		set rpfLink ""
		set rpfHead ""
	} else {
		set rpfLink [$ns_ link $node_ $nbr]
		set rpfHead [$rpfLink head]
	}

        set oiflist ""
        foreach oif [$node_ get-all-oifs] {
		if { $oif != $rpfHead } {
			lappend oiflist $oif
                }
        }
	
	# if null oif and no local members then this is a neg. cahce
	# and we should send a prune. Necessary for LANs
	if { $iif >= 0 && $oiflist == "" &&				\
			$rpfLink != "" && [$rpfLink isLan?] &&		\
			![$node_ check-local $group] } {
		$self send-prune $srcID $group
	}
	set oiflist
}

detailedDM instproc handle-wrong-iif { srcID group iface } {
	$self instvar ns_ node_ iifLink_

	# if the iif is in the oiflist, then send Assert
	set badifl [$node_ if2link $iface]
	if { [$badifl isLan?] } {
		set badoif [[$ns reverse $badifl] link2oif]
		set r [$node_ getReps $srcID $group]
		if [$r is-active-target $badoif] {
			# on LANs we send asserts
			$self send-assert $srcID $group $iface
		}
	} else {
		# on p2p links we may send prunes
		# need to send prunes to nbr other than rpf..
		$self send-mcast prune $srcID $group [$badifl src] $badifl ""
	}
	return 1
}

#
#
# ASSERT PROCESSING FOR LANS
#
detailedDM instproc send-assert { src grp iface } {
	# asserts don't have a next hop field
	# what about the metric.. ?! use mesg
	set rpf 0
	#use 0 metric for now
	set mesg 0

	$self send-mcast assert $src $grp $rpf $iface $mesg
}

detailedDM instproc recv-assert { from src grp to msg } {
	$self instvar ns_ node_ iifLink_ rpfDone_
	set r [$node_ getReps $src $grp]
	if { $r == "" } {
		return 0
	}
	set iifl [$ns_ link $from $node_]
	if { $iifLink_($src) == $iifl } {
		# I am a downstream router, set the rpf
		set-rpfNode-info $src $from
		return 1
	}
	# I am an upstream router, so check if the cache is active
	set oifObj [[$ns_ reverse $iifl] link2oif]
	if ![$r is-active-target $oifObj] {
		return 0
	}
	# I have active cache, so compare metric, and address
	# a- compare metric... later, TBD
	# b- compare add
	if { $from > $node_ } {
		# I lost the assert, delete oif
		$self delete_oif $src $grp $iif
	} else {
		# I won the assert, send a winning assert
		$self send-assert $src $grp $iif
	}
}

#
# JOIN/LEAVE PROCESSING
#
detailedDM instproc send-join { src grp } {
        set iif [$self find-iif-link $src]
        set rpf [$self rpf-nbr $src]
        $self send-mcast join $src $grp $rpf $iif ""
}

detailedDM instproc recv-join { from src grp to msg } {
        $self instvar ns_ node_ DelTimer_
	if { $to == $node_ } {
		# if the deletion timer is running, clear it
		$self cancel DelTimer_($src:$grp)
		set iif [$self find-iif-link $src]
		set oif [[$ns_ link $from $node_] head]
		$node_ add-mfc $src $grp $iif $oif
	}
}

detailedDM instproc leave-group { group } {
	$self instvar node_ RtxTimer_
	# if I have no local members, then prune all negative caches
	# with oiflist null
	if ![$node_ check-local $group] {
		foreach rep [$node_ getReps * $group] {
			if ![$rep is-active] {
				set src [$rep set srcID_] 
				$self send-prune $src $group
				$self cancel RtxTimer_($src:$group)
			}
		}
	}
	$self next $group
}

detailedDM instproc drop { replicator src grp iface } {
        $self instvar node_ ns_ iifLink_
	set id [$node_ id]
        #
        # No downstream listeners 
        # Send a prune back toward the source
        #
        if { $src == $id } {
                #
                # if we are trying to prune ourself (i.e., no
                # receivers anywhere in the network), set the
                # ignore bit in the object (we turn it back on
                # when we get a graft).  This prevents this
                # drop methood from being called on every packet.
                #
                $replicator set ignore_ 1
        } else {
		# only on p2p links do we send pkt triggered prunes
		if { ![info exists iifLink_($src)] } {
			return 0
		}
		set rpfLink [$ns_ link $node_ [$self rpf-nbr $src]]
		if { [$rpfLink isLan?] } {
			# on a LAN .. ignore the dropped pkts
			# be sure to set ignore to 0 when joining or grafting
			$replicator set ignore_ 1
		} else {
	                $self send-prune $src $grp
		}
        }
}

# Pruning. 
#
detailedDM instproc send-prune { src grp } {
	set iif [$self find-iif-link $src]
	set rpf [$self rpf-nbr $src]
	$self send-mcast prune $src $grp $rpf $iif ""
}

detailedDM instproc recv-prune { from src grp pktiif to msg } {
	$self instvar ns_ node_
	set iif [$self find-iif-link $src]
	set piif [$self iface2link $pktiif]
	if { [$piif isLan?] } {
		if { $iif == $piif } {
			set r [$node_ getReps $src $grp]
			if { $r != "" && [$r is-active] } {
				# I have active entries...override the prune
				$self send-join $src $grp
			}
		} else {
			if { $to == $node_ } {
				# if on a lan set the deletion timer
				$self instvar DelTimer_
				if ![info exists DelTimer_($src:$grp)] {
					set DelTimer_($src:$grp)  \
						[new Timer/Iface/Deletion \
						 $ns_ $self $src $grp $piif]
				}
				$DelTimer_($src:$grp) schedule
			}
		}
	} else {
		if { $iif != $piif } {
			# if on p2p just call delete_oif
			$self delete_oif $src $grp $piif
		}
	}
}

detailedDM instproc delete_oif { src grp oiflink } {
	$self instvar ns_ node_ DelTimer_ PruneTimer_
	set r [$node_ getReps $src $grp]
	if { $r != "" } {
		set outLink [$ns_ reverse $oiflink]
		if [$r is-active] {
			$self send-prune $src $grp
		}
		# destroy deletion timer
		$self cancel DelTimer_($src:$grp:$outLink)
		$self schedule-prune $r $src $grp $outLink
	}
}

detailedDM instproc schedule-prune {rep src grp oifl} {
	$self instvar ns_ PruneTimer_

	$rep disable [$self link2oif $oifl]
	if ![info exists PruneTimer_($src:$grp:$oifl)] {
		set PruneTimer_($src:$grp:$oifl) [new Timer/Iface/Prune \
				$ns_ $self $src $grp $oifl]
	}
	$PruneTimer_($src:$grp:$oifl) schedule
}

detailedDM instproc timeoutPrune { src grp outLink } {
	$self instvar ns_ node_ PruneTimer_
	set r [$node_ getReps $src $grp]
	if { $r != "" } {
		# check if the oif is up
		if { [$outLink up?] == "up" } {
			if { ![$r is-active] } {
				$self send-graft $src $grp
			}
			$r insert [$self link2oif $outLink]
			$self cancel PruneTimer_($src:$grp:$outLink)
		} else {
			$PruneTimer_($src:$grp:$outLink) schedule
		}
	}
}

#
# GRAFT HANDLING
#
detailedDM instproc send-graft { src grp } {
	$self instvar ns_ node_ RtxTimer_
	# set the retx timer
	if { [$node_ id] != $src } {
		if ![info exists RtxTimer_($src:$grp)] {
			set RtxTimer_($src:$grp) [new Timer/GraftRtx $ns_ $self $src $grp]
		}
		$RtxTimer_($src:$grp) schedule
		$self send-unicast graft $src $grp [$self rpf-nbr $src]
	}
}

detailedDM instproc recv-graft { from src group msg } {
	$self instvar ns_ node_ PruneTimer_
	# send a graft ack
	$self send-unicast graftAck $src $group $from

	if { $node_ != $from } { 
		if { $src != [$node_ id] } {
			set r [$node_ getReps $src $group]
			if { $r == "" || ![$r is-active] } {
				# send a graft upstream
				$self send-graft $src $group
			}
		}
		set oif [$self link2oif [$ns_ link $node_ $from]]
		$self cancel DelTimer_($src:$group:$oif)

		# add the oif to the oiflist
		$node_ add-mfc $src $group [$self find-iif $src] $oif
	}
}

detailedDM instproc recv-graftAck { from src grp to args } {
	# if the retransmission timer is running, clear it
	$self cancel RtxTimer_($src:$grp)
}

#
# Failure modes
#
detailedDM instproc notify changes {
	$self instvar node_ iifLink_ ns_ rpfNode_

	# check which links/oifs are down (if any)
	set _down ""
	foreach oif [$node_ get-all-oifs] {
		# XXX Assumes link head is special no-op connector
		if { [[$oif set link_] up?] != "up" } {
			lappend _down $oif
		}
	}
	# traverse the mcast states and check to which routes have changed
	foreach {index rep} [$node_ getReps-raw * *] {
		set src [lindex [split $index :] 0]
		set grp [lindex [split $index :] 1]

		$self instvar PruneTimer_
		# remove the down oifs from the oiflist
		# take care with crashes, we may never get a graft, and so
		# may want to set the prune timer,... but if expired check
		# if the links is still down.. so on.. deal with this later XXX
		foreach oif $_down {
			if { [$rep exists $oif] && [$rep set active_($oif)] } {
				$self schedule-prune $rep $src $grp $oif
			}
		}

		if ![info exists rpfNode_($src)] {
			continue
		}
		set oldrpf [$self reset-rpfNode-info $src]
		if ![info exists srcDone($src)] {
			set srcDone($src) [$self rpf-nbr $src]
		}
		if { $oldrpf == $srcDone($src) } {
			continue
		}
		set newiifl [$ns_ link $srcDone($src) $node_]
		if { [info exists iifLink_($src)] && $iifLink_($src) != $newiifl } {
			$self change-iif $rep $src $grp $newiifl $srcDone($src)
		} else {
			$self change-rpf $rep $src $grp $oldrpf $srcDone($src)
		}
	}
}

detailedDM instproc change-iif { rep src grp newiif newrpf } {
	$self instvar ns_ node_ iifLink_ PruneTimer_
	# remove the new iif from the oiflist
	$rep disable [$self link2oif [$ns_ reverse $newiif]]
	# schedule to add the old iif to the oiflist
	# check if the link is down first
	if { [$iifLink_($src) up?] == "up" } {
		set oldiif $iifLink_($src)
		$rep disable [$oldiif head]
		$self schedule-prune $rep $src $grp $oldiif
		# prune off the old nbr
		if [$rep is-active] {
			$self send-prune $src $grp
		}
	} 

	$rep change-iface $src $grp $iifLink_($src) $newiif
	set iifLink_($src) $newiif
	# if we have active cache then graft the new nbr
	if [$rep is-active] {
		$self send-graft $src $grp
	}
	$self cancel PruneTime_($src:$grp:$newiif)
}


detailedDM instproc change-rpf { rep src grp oldrpf newrpf } {
        # invoking this proc means we are on a lan, changed the rpf,
        # but not the iif
        $self instvar rpfNode_
        if [$rep is-active] {
                # prune old rpf, and graft new rpf
                set rpfNode_($src:$grp) $oldrpf
                $self send-prune $src $grp
                set rpfNode_($src:$grp) $newrpf
                $self send-graft $src $grp
        } else {
		# if we are a negative cache on a lan, prune off new rpf
		set rpfNode_($src:$grp) $newrpf
		$self send-prune $src $grp
	}
}                

###############################################
### assorted helpers

detailedDM instproc send-ctrl { type src grp } {
	$self send-$type $src $grp
}

detailedDM instproc send-unicast { which src group to } {
	$self instvar ns_ node_ prune_
	set prune2 [[[$to getArbiter] getType [$self info class]] set prune_]

	$prune_ target [$node_ entry]
	$ns_ connect $prune_ $prune2
	$prune_ send $which $node_ $src $group
}

detailedDM instproc send-mcast { type src grp rpfNode iifLink mesg } {
	# check that oif and rpf are not bogus.. 
	# 0 for rpf is needed for asserts

	if { $iifLink != "SRC" } {
		$self instvar ns_ prune_ node_
		$prune_ target [[$ns_ reverse $iifLink] head]
		$prune_ set dst_ [mrtObject getWellKnownGroup ALL_ROUTERS]
		$prune_ send $type $node_ $src $grp $rpfNode $mesg	;# XXX
	}
}

detailedDM instproc cancel timer {
	$self instvar $timer
	if [info exists $timer] {
		set handle $timer
		$handle cancel
		delete $handle
		unset $timer
	}
}

detailedDM instproc rpf-nbr src {
	$self instvar rpfNode_
	if ![info exists rpfNode_($src)] {
		$self instvar node_
		$self set-rpfNode-info $src [$node_ rpf-nbr $src]
	}
	set rpfNode_($src)
}

detailedDM instproc reset-rpfNode-info src {
	$self instvar rpfNode_
	set oldval ""
	if [info exists rpfNode_($src)] {
		set oldval $rpfNode_($src)
		unset rpfNode_($src)
	}
	set oldval
}

detailedDM instproc set-rpfNode-info {src val} {
	$self set rpfNode_($src) $val
}

### Assorted timers
Class Timer/GraftRtx -superclass Timer
Timer/GraftRtx set timeout_ 0.05
Timer/GraftRtx instproc init {ns proto src grp} {
	$self set proto_ $proto
	$self set src_   $src
	$self set grp_   $grp

	$self set timeout_ [[$self info class] set timeout_]
	$self next $ns
}

Timer/GraftRtx instproc schedule {} {
	$self instvar timeout_
	$self sched $timeout_
}

Timer/GraftRtx instproc timeout {} {
	$self instvar proto_ src_ grp_
	$proto_ send-graft $src_ $grp_
}

Class Timer/Iface -superclass Timer
#Timer/Iface set timeout_ 99999			;# defaults to infinity
Timer/Iface instproc init {ns proto src grp oif} {
	$self set proto_ $proto
	$self set src_   $src
	$self set grp_   $grp
	$self set oif_   $oif

	$self set timeout_ [[$self info class] set timeout_]
	$self next $ns
}

Timer/Iface instproc schedule {} {
	$self instvar timeout_
	$self sched $timeout_
}

Class Timer/Iface/Prune -superclass Timer/Iface
Timer/Iface/Prune set timeout_ 0.5
Timer/Iface/Prune instproc timeout {} {
	$self instvar proto_ src_ grp_ oif_
	$proto_ timeoutPrune $src_ $grp_ $oif_
}

Class Timer/Iface/Deletion -superclass Timer/Iface
Timer/Iface/Deletion set timeout_ 0.1
Timer/Iface/Deletion instproc timeout {} {
	$self instvar proto_ src_ grp_ oif_
	$proto_ delete_oif $src_ $grp_ $oif_
}

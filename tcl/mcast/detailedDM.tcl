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
#	$prune_ set dst_ [mrtObject getWellKnownGroup ALL_ROUTERS]
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
# IIF handling code
#
detailedDM instproc find-iif-link {src grp {ifc -1}} {
	$self instvar node_ iifLink_
	if ![info exists iifLink_($src)] {
		if { $src == [$node_ id] } {
			set iifLink_($src) -2
		} else {
			set iifLink_($src) [$self get-iif-link	\
					[$self find-rpf-node $src $grp]]
		}
	}
	set iifLink_($src)
}

detailedDM instproc get-iif-link nextHop {
	$self instvar ns_ node_
	if { $nextHop != $node_ } {
		return [$ns_ link $nextHop $node_]
	} else {
		return ""
	}
}

#
# RPF handling
#
detailedDM instproc find-rpf-node {s g} {
	$self instvar ns_ node_ rpfNode_
	if { ![info exists rpfNode_($s:$g)] } {
		set rpfNode_($s:$g) [$ns_ upstream-node [$node_ id] $s]
	}
	set rpfNode_($s:$g)
}

#
# OIF handling code
#
detailedDM instproc find-oifs {src grp iif} {
	# may check if iif == iface before creating the entry.. !
        $self instvar ns_ node_

	set nbr [$self find-rpf-node $src $grp]
	if { $node_ == $nbr } {
		set rpfHead ""
	} else {
		set rpfHead [[$ns_ link $node_ $nbr] head]
	}

        set oiflist ""
        foreach oif [$node_ get-oifs] {
		if { $oif != $rpfHead } {
			lappend oiflist $oif
                }
        }
	
	# if null oif and no local members then this is a neg. cahce
	# and we should send a prune. Necessary for LANs
	if { $iif > -1 && [$rpfLink isLan?] &&			\
			$oiflist == "" && ![$node_ check-local $group] } {
		$self send-prune $srcID $group
	}
	set oiflist
}


detailedDM instproc drop { replicator src grp } {
        $self instvar node_ ns iifLink_
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
		set rpfLink [$ns_ link $node_ [$self find-rpf-node $src $grp]]
		if { [$rpfLink isLan?] } {
			# on a LAN .. ignore the dropped pkts
			# be sure to set ignore to 0 when joining or grafting
			$replicator set ignore_ 1
		} else {
	                $self send-prune $src $grp
		}
        }
}

#
# Message sends
#
detailedDM instproc send-ctrl { type src grp } {
	error "how reached?"
	$self send-$type $src $grp
}

detailedDM instproc send-unicast { which src group to } {
	$self instvar ns_ node_ prune_
	set prune2 [[[$to getArbiter] getType [$self info class]] set prune_]

	$prune_ target [$node_ entry]
	$ns connect $prune_ $prune2
	$prune_ send $which $node_ $src $group
}

detailedDM instproc send-mcast { type src grp rpfNode iifLink mesg } {
	# check that oif and rpf are not bogus.. 
	# 0 for rpf is needed for asserts

	# iif may be -2 in the source router
	if { $oifLabel < 0 } {
		return 0
	}
	$self instvar ns_ prune_
	$prune_ target [[$ns_ reverse $iifLink] head]
	$prune_ set dst_ [mrtObject getWellKnownGroup ALL_ROUTERS]	
	$prune_ send $type $node_ $src $grp $rpfNode $mesg	;# XXX
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

#
# Protocol Timers
#
detailedDM instproc schedule-prune {rep src grp oif} {
	$self instvar ns_ PruneTimer_

	$rep disable $oif
	set revl [[$ns_ reverse $oif] if-label?]
	if ![info exists PruneTimer_($src:$grp:$revl)] {
		set PruneTimer_($src:$grp:$revl) [new Timer/Iface/Prune \
				$src $grp $revl $ns_]
	}
	$PruneTimer_($src:$grp:$revl) schedule
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

#
# Various handlers
#
detailedDM instproc notify changes {
	$self instvar node_ iifLink_ ns_ rpfDone_
	set id [$node_ id]

	# check which links/oifs are down (if any)
	set _down ""
	foreach link [$node_ get-oifs] {
		if { [$link up?] != "up" } {
			lappend _down [$node_ get-oif $link]
		}
	}

	# traverse the mcast states and check to which routes have changed
	foreach {index rep} {$node_ getReps-raw * *} {
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

		if ![info exists rpfNode_($index)] {
			continue
		}
		if ![info exists srcDone($src)] {
			set srcDone($src) [$ns upstream-node $id $src]
		}
		if { $rpfNode_($index) == $srcDone($src) } {
			continue
		}
		set newiif [$self get-iif-link $srcDone($src)]
		set oldrpf $rpfNode_($index)
		if { [info exists iifLink_($src)] && $iifLink_($src) != $newiif } {
			$self change-iif $rep $src $grp $newiif $srcDone($src)
		} else {
			$self change-rpf $rep $src $grp $oldrpf $srcDone($src)
		}
	}
}

detailedDM instproc change-iif { rep src grp newiif newrpf } {
	$self instvar ns_ node_ rpfNode_ iifLink_ PruneTimer_
	# remove the new iif from the oiflist
	$rep disable [$newiif head]
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
	set rpfDone_($src:$grp) $newrpf
	# if we have active cache then graft the new nbr
	if [$rep is-active] {
		$self send-graft $src $grp
	}
	$self cancel PruneTime_($src:$grp:$newiif)
}

detailedDM instproc handle-wrong-iif { srcID group iface } {
	$self instvar ns_ node_ iifLink_

	# if the iif is in the oiflist, then send Assert
	set r [$node_ getReps $srcID $group]
	$r instvar active_
	set badifl [$node_ if2link $iface]
	if { [[$ns_ link $id $nbr] isLan?] } {
		set badoif [[$ns reverse $badifl] head]
		if [info exists active_($badoif)] {
			if $active_($badoif) {
				# on LANs we send asserts
				$self send-assert $srcID $group $iface
			}
		}
	} else {
		# on p2p links we may send prunes
		# need to send prunes to nbr other than rpf..
		$self send-mcast prune $srcID $group [$badifl src] $badifl ""
	}
	return 1
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

# send-prune called by change-iif, change-rpf, handle-wrong-iif (p2p, 
# directed prune), handle-cache-miss, drop (p2p), leave-group
detailedDM instproc send-prune { src grp } {
	set iif [$self find-iif-link $src $grp]
	set rpf [$self find-rpf-node $src $grp]
	$self send-mcast prune $src $grp $rpf $iif ""
}

detailedDM instproc recv-prune { from src grp to msg } {
	$self instvar ns_ node_
	# see if the message is destined to me
	set iif [$self find-iif-link $src $grp]
	set ifl [$ns_ link $from $to]
	if { $to != $node_ } {
		# if I have a forwarding entry override by sending a join
		set r [$node_ getReps $src $grp]
		if { $r == "" } {
			return 0
		}
		# check if the interface is my iif
		if { $iif != $ifl } {
			return 0
		}
		if { [$r is-active] } {
			$self send-join $src $grp
		}
		return 1		
	}
	# drop prunes to you on iif
	if { $iif == $ifl } {
		return 0
	}
	if { [$ifl isLan?] } { 
		# if on a lan set the deletion timer
		$self instvar DelTimer_
		if ![info exists DelTimer_($src:$grp)] {
			set DelTimer_($src:$grp)  [new Timer/Iface/Deletion \
					$self $src $grp $ifl $ns_]
		}
		$DelTimer_($src:$grp) schedule
	} else {
		# if on p2p just call delete_oif
		$self delete_oif $src $grp $ifl
	}
}

# called by recv-graft, change-iif, change-rpf, and send-ctrl from 
# join-group (DM.tcl) 
detailedDM instproc send-graft { src grp } {
	$self instvar ns_ node_ RtxTimer_
	# set the retx timer
	if { $node_ == $src } {
		return 0
	}
	if ![info exists RtxTimer_($src:$grp)] {
		set RtxTimer_($src:$grp) [new Timer/GraftRtx $self $src $grp $ns]
	}
	$RtxTimer_($src:$grp) schedule
	$self send-unicast graft $src $grp [$self find-rpf-node $src $grp]
}

detailedDM instproc recv-graft { from src group msg } {
	$self instvar ns_ node_ PruneTimer_
	# send a graft ack
	$self send-unicast graftAck $src $group $from

	if { $node_ == $from } { 
		return 0
	}
	set r [$node_ getReps $src $group]
#	puts "active [$r is-active]"
	if { $r == "" || ![$r is-active] && $src != $id } {
		# send a graft upstream
		$self send-graft $src $group
	}
	set oif [[$ns_ link $node_ $from] head]
	$self cancel DelTimer_($src:$group:$oif)

	# add the oif to the oiflist
	$node_ add-mfc $src $group [$self find-iif-link $src $group] $oif
}

detailedDM instproc recv-graftAck { from src grp to msg } {
	# if the retransmission timer is running, clear it
	$self cancel RtxTimer_($src:$grp)
}

detailedDM instproc send-join { src grp } {
        set iif [$self find-iif $src $grp]   
        set rpf [$self find-rpf $src $grp]
        $self send-mcast join $src $grp $rpf $iif ""
}

detailedDM instproc recv-join { from src grp to msg } {
        $self instvar ns_ node_ DelTimer_
        # see if the message is destined to me
	if { $to != $node_ } {
		return 0
	}
	# if the deletion timer is running, clear it
	$self cancel DelTimer_($src:$grp)
	set iif [$self find-iif-link $src $grp]
	set oif [[$ns_ link $from $node_] head]
	$node_ add-mfc $src $grp $iif $oif
}

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
	set iif [$ns_ link $from $node_]
	if { $iifLink_($src) == $iif } {
		# I am a downstream router, set the rpf
		set rpfDone_($src:$grp) $from
		return 1
	}
	# I am an upstream router, so check if the cache is active
	set oifObj [[$ns_ reverse $iif] head]
	if ![$r exists $oifObj] {
	 	return 0
	}
	$r instvar active_
	if !$active_($oifObj) {
		return 0
	}
	# I have active cache, so compare metric, and address
	# a- compare metric... later, TBD
	# b- compare add
	if { $from > $node_ } {
		# I lost the assert, delete oif
		$self delete_oif $src $grp $iif
		return 0
	}
	# I won the assert, send a winning assert
	$self send-assert $src $grp $iif
}


detailedDM instproc delete_oif { src grp oiflink } {
	$self instvar ns_ node_ DelTimer_ PruneTimer_
	set r [$node_ getReps $src $grp]
	if { $r == "" } {
		return -1
	}
	$r disable [$oiflink head]
	if { ![$r set nactive_] } {
		$self send-prune $src $grp
	}
	# destroy deletion timer
	$self cancel DelTimer_($src:$grp:$oiflink)
	$self schedule-prune $r $src $grp $oiflink
}

detailedDM instproc timeoutPrune { oif src grp } {
	$self instvar ns_ node_ PruneTimer_
	set r [$node_ getReps $src $grp]
	if { $r == "" } {
		return -1
	}

	# check if the oif is up
	if { [$oiflink up?] == "up" } {
		$PruneTimer_($src:$grp:$oiflink) schedule
		return 0
	}
	if { ![$r is-active] } {
		$self send-graft $src $grp
	}
	$r insert  [$oiflink head]
	$self cancel PruneTime_($src:$grp:$oiflink)
}


###############################################

Class Timer/GraftRtx -superclass Timer

Timer/GraftRtx set timeout 0.05

Timer/GraftRtx instproc init { protocol source group sim} {
	$self instvar src grp proto ns
	set src $source 
	set grp $group
	set proto $protocol
	set ns $sim
}

Timer/GraftRtx instproc schedule {} {
	$self sched [Timer/GraftRtx set timeout]
}

Timer/GraftRtx instproc timeout {} {
	$self instvar proto src grp
	$proto send-graft $src $grp
}

Class Timer/Iface -superclass Timer

Timer/Iface instproc init { protocol source group oiface sim} {
	$self instvar proto src grp oif ns
	set proto $protocol
	set src $source
	set grp $group
	set oif $oiface
	set ns $sim
}

Timer/Iface instproc schedule {} {
	$self sched [[$self info class] set timeout]
}

Class Timer/Iface/Prune -superclass Timer/Iface

Timer/Iface/Prune set timeout 0.5

Timer/Iface/Prune instproc timeout {} {
	$self instvar proto src grp oif
	$proto timeoutPrune $oif $src $grp
}

Class Timer/Iface/Deletion -superclass Timer/Iface

Timer/Iface/Deletion set timeout 0.1

Timer/Iface/Deletion instproc timeout {} {
	$self instvar proto src grp oif
	$proto delete_oif $src $grp $oif
}

###################################

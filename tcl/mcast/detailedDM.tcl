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
	$node_ join-group $prune_ [mrtObject getWellKnownGroup ALL_PIM_ROUTERS]
}

detailedDM instproc get-iif-label { nxthop } {
        $self instvar node_ ns_
        if { $nxthop == [$node_ id] } {
                return -1
        }
	$ns_ instvar link_
        set oifInfo [$node_ get-oif $link_([$node_ id]:$nxthop)]
        set oif [lindex $oifInfo 0]
        return $oif
}

# helper function
detailedDM proc getLinkType { link } {
	if { [$link info class ] == "DummyLink" } {
		return lan
	}
	return p2p
}

detailedDM instproc handle-cache-miss { srcID group iface } {

        # get RPF iif
	set iif [$self get_iif $srcID $group]

	# may check if iif == iface before creating the entry.. !

        $self instvar node_ ns_
	set id [$node_ id]
        set oiflist ""
    
        # include all oifs in the oiflist except the rpf iif
        set alloifs [$node_ get-oifs]
        foreach oif $alloifs {
		$node_ instvar outLink_
                if { $iif != $oif } {
                        lappend oiflist $outLink_($oif)
                }
        }
	
	# if null oif and no local members then this is a neg. cahce
	# and we should send a prune. Necessary for LANs
	
	if { $iif > -1 } {
	 set nbr [[$node_ ifaceGetNode $iif] id]
	 $ns_ instvar link_ 
	 if { [detailedDM getLinkType $link_($id:$nbr)] == "lan" } {
	  if { $oiflist == "" && ![$node_ check-local $group] } {
		$self send-prune $srcID $group
	  }
	 }
	}
        $node_ add-mfc $srcID $group $iif $oiflist
}

detailedDM instproc drop { replicator src grp iface} {
        $self instvar node_ ns_ iif_
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
		if { ![info exists iif_($src)] } {
			return 0
		}
		set nbr [[$node_ ifaceGetNode $iif_($src)] id]
		$ns_ instvar link_
		if { [detailedDM getLinkType $link_($id:$nbr)] == "p2p" } {
	                $self send-prune $src $grp
		} else {
			# on a LAN .. ignore the dropped pkts
			# be sure to set ignore to 0 when joining or grafting
			$replicator set ignore_ 1
		}
        }
}

detailedDM instproc leave-group { group } {
	$self instvar node_ RtxTimer_
	# if I have no local members, then prune all negative caches
	# with oiflist null
	if ![$node_ check-local $group] {
	  foreach rep [$node_ getRepByGroup $group] {
	  	if ![$rep is-active] {
			set src [$rep set srcID_] 
			$self send-prune $src $group
			# clear any graft retransmission timer
			if [info exists RtxTimer_($src:$group)] {
				$RtxTimer_($src:$group) cancel
				delete $RtxTimer_($src:$group)
				unset RtxTimer_($src:$group)
			}
		}
	  }
	}
	$self next $group
}

detailedDM instproc send-ctrl { type src grp } {
	$self send-$type $src $grp
}

detailedDM instproc notify changes {
	$self instvar node_ iif_ ns_ RPF_
	$node_ instvar replicator_
	set id [$node_ id]

	# check which links/oifs are down (if any) 
	foreach node [$node_ set neighbor_] {
		set link [$ns_ set link_($id:[$node id])]
		if { [$link up?] != "up" } {
			set oifDown [$node_ get-oif $link]
			set down_($oifDown) 1
		}
	}

	# traverse the mcast states and check to which routes have changed
	foreach index [array names replicator_] {
	  set rep $replicator_($index)
	
	  set src [lindex [split $index :] 0]
	  set grp [lindex [split $index :] 1]

	  $self instvar PruneTimer_
	  # remove the down oifs from the oiflist
	  # take care with crashes, we may never get a graft, and so
	  # may want to set the prune timer,... but if expired check
	  # if the links is still down.. so on.. deal with this later XXX
	  foreach oif [array names down_] {
		  if [$rep is-active-target $oif] {
			  $replicator_($index) disable $oif
			  if ![info exists PruneTimer_($index:[$oif id])] {
				  set $PruneTimer_($index:[$oif id]) \
						  [new Prune/Iface/Timer $src $grp [$oif id] $ns_]
			  }
			  $PruneTimer_($index:[$oif id]) schedule
		  }
	  }

	  if ![info exists RPF_($index)] {
		continue
	  }
	  if ![info exists srcDone($src)] {
		  set nbr [$ns_ upstream-node $id $src]
		  set rpf [$nbr id]
		  set srcDone($src) $rpf
	  } else {
		  set rpf $srcDone($src)
	  }
	  if { $RPF_($index) == $rpf } {
		continue
	  }
	  set newiif [$self get-iif-label $rpf]
	  set oldrpf $RPF_($index)
	  set iifChange 0
	  if [info exists iif_($src)] {
	    if { $iif_($src) != $newiif } {
		$self change-iif $rep $src $grp $newiif $rpf
		set iifChange 1
	    } else {
		set iifChange 0
	    }
	  }
	  if !$iifChange {
	    $self change-rpf $rep $src $grp $oldrpf $rpf
	  }
	}
}

detailedDM instproc change-rpf { rep src grp oldrpf newrpf } {
        # invoking this proc means we are on a lan, changed the rpf,
        # but not the iif
        $self instvar node_
#	puts "Change RPF node [$node_ id] oldrpf $oldrpf, newrpf $newrpf"
        if [$rep is-active] {
                # prune old rpf, and graft new rpf
                $self instvar RPF_
                set RPF_($src:$grp) $oldrpf
                $self send-prune $src $grp
                set RPF_($src:$grp) $newrpf
                $self send-graft $src $grp
                return 1
        }
        # if we are a negative cache on a lan, prune off new rpf
        $self instvar RPF_
        set RPF_($src:$grp) $newrpf
        $self send-prune $src $grp
}                

detailedDM instproc change-iif { rep src grp newiif newrpf } {
	$self instvar iif_ RPF_ node_ PruneTimer_ ns_
	# remove the new iif from the oiflist
	$rep disable [$node_ label2iface $newiif]
	# schedule to add the old iif to the oiflist
	# check if the link is down first
	$ns_ instvar link_
	set id [$node_ id]
	set nbr [[$node_ ifaceGetNode $iif_($src)] id]
	set link $link_($id:$nbr)
	if { [$link up?] == "up" } {
	  set oldiif $iif_($src)
	  $rep disable [$node_ label2iface $oldiif]
	  if ![info exists PruneTimer_($src:$grp:$oldiif)] {
	    set PruneTimer_($src:$grp:$oldiif) \
		[new Prune/Iface/Timer $self $src $grp $oldiif $ns_]
	  }
	  $PruneTimer_($src:$grp:$oldiif) schedule
	  # prune off the old nbr
	  if [$rep is-active] {
		  $self send-prune $src $grp
	  }
	} 

	$rep change-iface $src $grp $iif_($src) $newiif
	set iif_($src) $newiif
	set RPF_($src:$grp) $newrpf
	# if we have active cache then graft the new nbr
	if [$rep is-active] {
		$self send-graft $src $grp
	}
	# destroy the prune timer for the new iif
	if [info exists PruneTimer_($src:$grp:$newiif)] {
		$PruneTimer_($src:$grp:$newiif) cancel
		delete $PruneTimer_($src:$grp:$newiif)
		unset PruneTimer_($src:$grp:$newiif)
	}
}

detailedDM instproc handle-wrong-iif { srcID group iface } {
	$self instvar node_ ns_ iif_

	# if the iif is in the oiflist, then send Assert
	set r [$node_ getRep $srcID $group]

	set tempif [$node_ label2iface $iface]
	set nbr [[$node_ ifaceGetNode $iface] id]
	$ns_ instvar link_
	set id [$node_ id]
	if { [detailedDM getLinkType $link_($id:$nbr)] == "lan" } {
		if [$r is-active-target $tempif] {
			# on LANs we send asserts
			$self send-assert $srcID $group $iface
		}
	} else {
		# on p2p links we may send prunes
		# need to send prunes to nbr other than rpf..
		$self send-mcast prune $srcID $group $nbr $iface ""
	}
}

# send-prune called by change-iif, change-rpf, handle-wrong-iif (p2p, 
# directed prune), handle-cache-miss, drop (p2p), leave-group
detailedDM instproc send-prune { src grp } {
	set iif [$self get_iif $src $grp]
	set rpf [$self get_rpf $src $grp]
	#puts "send-mcast prune $src $grp $rpf $iif "
	$self send-mcast prune $src $grp $rpf $iif ""
}

# called by recv-graft, change-iif, change-rpf, and send-ctrl from 
# join-group (DM.tcl) 
detailedDM instproc send-graft { src grp } {
	$self instvar ns_
	# set the retx timer 
	$self instvar node_ RtxTimer_
	if { [$node_ id] == $src } {
		return 0
	}
	if ![info exists RtxTimer_($src:$grp)] {
		set RtxTimer_($src:$grp) [new GraftRtx/Timer $self $src $grp $ns_]
	}
	$RtxTimer_($src:$grp) schedule
        set rpf [$self get_rpf $src $grp]
	$self send-unicast graft $src $grp $rpf
}

detailedDM instproc send-assert { src grp iface } {
	# asserts don't have a next hop field
	# what about the metric.. ?! use mesg
	set rpf 0
	#use 0 metric for now
	set mesg 0
	$self instvar node_
	$self send-mcast assert $src $grp $rpf $iface $mesg
}

detailedDM instproc recv-prune { from src grp iface msg} {
	$self instvar node_ ns_
	set to [lindex $msg 0]
	# see if the message is destined to me
	set iif [$self get_iif $src $grp]
	set id [$node_ id]
#	puts "$id received a prune $src, $grp, $from, MSG:$msg"
	set ifaceLabel $iface ;#[$node_ get-oifIndex $from]
	if { $to != $id } {
		# if I have a forwarding entry override by sending a join
		set r [$node_ getRep $src $grp]
		if { $r == "" } {
			return 0
		}
		# check if the interface is my iif
		if { $iif != $ifaceLabel } {
			return 0
		}
		if { [$r is-active] } {
			$self send-join $src $grp
		}
		return 1		
	}
	# drop prunes to you on iif
	if { $iif == $ifaceLabel } {
		return 0
	}
	$ns_ instvar link_
	if { [detailedDM getLinkType $link_($id:$from)] == "lan" } {
		# if on a lan set the deletion timer
		$self instvar DelTimer_
		if ![info exists DelTimer_($src:$grp)] {
		  set DelTimer_($src:$grp) \
			[new Deletion/Iface/Timer $self $src $grp $ifaceLabel $ns_]
		}
		$DelTimer_($src:$grp) schedule
		return 1
	}
	# if on p2p just call delete_oif
	$self delete_oif $src $grp $ifaceLabel
}

detailedDM instproc recv-graft { from src group iface msg } {
	$self instvar node_ PruneTimer_ ns_
	# send a graft ack
	$self send-unicast graftAck $src $group $from

	set id [$node_ id]
#	puts "at [$ns_ now] node $id, recv-graft, src $src, grp $group from $from"

	if { $from == $id } {
		return 0
	}
	set r [$node_ getRep $src $group]
#	puts "active [$r is-active]"
	if { $r == "" || ![$r is-active] && $src != $id } {
		# send a graft upstream
		$self send-graft $src $group
	}
	$node_ instvar outLink_
	set oif $outLink_([$node_ get-oifIndex $from])
	if [info exists DelTimer_($src:$group:$oif)] {
		$DelTimer_($src:$group:$oif) cancel
		delete $DelTimer_($src:$group:$oif)
		unset DelTimer_($src:$group:$oif)
	}

	# add the oif to the oiflist
	set iif [$self get_iif $src $group]
	$node_ add-mfc $src $group $iif $oif
}

detailedDM instproc send-join { src grp } {
        set iif [$self get_iif $src $grp]   
        set rpf [$self get_rpf $src $grp]
        $self send-mcast join $src $grp $rpf $iif ""
}

detailedDM instproc recv-join { from src grp iface msg } {
        $self instvar node_ DelTimer_
	set to [lindex $msg 0]
        # see if the message is destined to me
        set id [$node_ id]
       if { $to != $id } {
		return 0
	}
	# if the deletion timer is running, clear it
	if [info exists DelTimer_($src:$grp)] {
		$DelTimer_($src:$grp) cancel
		delete $DelTimer_($src:$grp)
		unset DelTimer_($src:$grp)
	}
	set iif [$self get_iif $src $grp]
	set oif [$node_ label2iface [$node_ get-oifIndex $from]]	
	$node_ add-mfc $src $grp $iif $oif
}

detailedDM instproc recv-graftAck { from src grp iface msg } {
	$self instvar RtxTimer_
	# if the retransmission timer is running, clear it
	if [info exists RtxTimer_($src:$grp)] {
		$RtxTimer_($src:$grp) cancel
		delete $RtxTimer_($src:$grp)
		unset RtxTimer_($src:$grp)
	}
}

detailedDM instproc recv-assert { from src grp iface msg } {
	$self instvar node_ iif_ RPF_
	set r [$node_ getRep $src $grp]
	if { $r == "" } {
		return 0
	}
	set iif [$node_ get-oifIndex $from]
	if { $iif_($src) == $iif } {
	  # I am a downstream router, set the rpf
	  set RPF_($src:$grp) $from
	  return 1
	}
	# I am an upstream router, so check if the cache is active
	set oifObj [$node_ label2iface $iif]
	if [$r is-active-target $oifObj] {
		return 0
	}
	# I have active cache, so compare metric, and address
	# a- compare metric... later, TBD
	# b- compare add
	set id [$node_ id]
	if { $from > $id } {
		# I lost the assert, delete oif
		$self delete_oif $src $grp $iif
		return 0
	}
	# I won the assert, send a winning assert
	$self send-assert $src $grp $iif
}

detailedDM instproc get_iif { src grp } {
	$self instvar iif_ node_
	if [info exists iif_($src)] {
		return $iif_($src)
	}
	if { $src == [$node_ id] } {
		set iif -2
	} else {
		set iif [$self get-iif-label [$self get_rpf $src $grp]]
	}
	set iif_($src) $iif
	return $iif		
}

detailedDM instproc get_rpf { src grp } {
	$self instvar RPF_ ns_ node_
	if ![info exists RPF_($src:$grp)] {
		set nbr [$ns_ upstream-node [$node_ id] $src]
		set RPF_($src:$grp) [$nbr id]
	}
	return $RPF_($src:$grp)
}	

detailedDM instproc send-unicast { which src group to } {
	$self instvar prune_ ns_ node_
	$prune_ target [$node_ entry]
	$ns_ instvar Node_
	set id [$node_ id]
	set nbr $Node_($to)
	set prune_2 [[[$nbr getArbiter] getType [$self info class]] set prune_]
	$ns_ connect $prune_ $prune_2
	if { $which == "graft" } {
		$prune_ set class_ 31
	} else {
		# graft-ack
		$prune_ set class_ 32
	}
	$prune_ transmit $which $id $src $group
}

detailedDM instproc send-mcast { type src grp rpf oifLabel mesg } {
	# check that oif and rpf are not bogus.. 
	# 0 for rpf is needed for asserts

	# iif may be -2 in the source router
	if { $oifLabel < 0 } {
		return 0
	}
	$self instvar prune_ node_
	set oif [$node_ label2iface $oifLabel]
	$prune_ target $oif

	$prune_ set dst_ [mrtObject getWellKnownGroup ALL_PIM_ROUTERS]
	
	switch $type {
		prune { set cls 30 }
		join { set cls 33 }
		assert { set cls 34 }
		default { puts "unknown type"; return 0 }
	}
	$prune_ set class_ $cls

	set id [$node_ id]
	set msg "$type/$rpf/$mesg"
#	puts "msg= $msg"
	$prune_ transmit $msg $id $src $grp
#	$prune_ transmit $type $id $src $grp

}

detailedDM instproc delete_oif { src grp oif } {
	$self instvar ns_ node_ DelTimer_ PruneTimer_
	set r [$node_ getRep $src $grp]
	if { $r == "" } {
		return -1
	}
	set oifObj [$node_ label2iface $oif]
	$r disable $oifObj
	if ![$r is-active] {
		$self send-prune $src $grp
	}
	# destroy deletion timer
	if [info exists DelTimer_($src:$grp:$oif)] {
		delete $DelTimer_($src:$grp:$oif)
		unset DelTimer_($src:$grp:$oif)
	}
	if ![info exists PruneTimer_($src:$grp:$oif)] {
	  set PruneTimer_($src:$grp:$oif) \
		[new Prune/Iface/Timer $self $src $grp $oif $ns_]
	}
	$PruneTimer_($src:$grp:$oif) schedule
}

detailedDM instproc timeoutPrune { oif src grp } {
	$self instvar node_ PruneTimer_ ns_
	set r [$node_ getRep $src $grp]
	if { $r == "" } {
		return -1
	}

	# check if the oif is up
	set nbr [[$node_ ifaceGetNode $oif] id]
	set link [$ns_ set link_([$node_ id]:$nbr)]
	if { [$link up?] != "up" } {
		$PruneTimer_($src:$grp:$oif) schedule
		return 0
	}

	set oifObj [$node_ label2iface $oif]
	if ![$r is-active] {
		$self send-graft $src $grp
	}
	$r insert $oifObj
	if [info exists PruneTimer_($src:$grp:$oif)] {
		$PruneTimer_($src:$grp:$oif) cancel
		delete $PruneTimer_($src:$grp:$oif)
		unset PruneTimer_($src:$grp:$oif)
	}
	return 1
}

detailedDM instproc stop {} {
	$self instvar RPF_ iif_ PruneTimer_ DelTimer_ RtxTimer_
	if [info exists RPF_] {
		unset RPF_
	}
        if [info exists iif_] {
                unset iif_
        }
        if [info exists DelTimer_] {
		foreach index [array names DelTimer_] {
			$DelTimer_($index) cancel
		}
                unset DelTimer_
        }
        if [info exists PruneTimer_] {
                foreach index [array names PruneTimer_] {
                	$PruneTimer_($index) cancel
                }
		unset PruneTimer_
        }
        if [info exists RtxTimer_] {
                foreach index [array names RtxTimer_] {
                	$RtxTimer_($index) cancel
                }
		unset RtxTimer_
        }
}

###############################################

Class GraftRtx/Timer -superclass Timer

GraftRtx/Timer set timeout 0.05

GraftRtx/Timer instproc init { protocol source group sim} {
	$self instvar src grp proto ns
	set src $source 
	set grp $group
	set proto $protocol
	set ns $sim
}

GraftRtx/Timer instproc schedule {} {
	$self sched [GraftRtx/Timer set timeout]
}

GraftRtx/Timer instproc timeout {} {
	$self instvar proto src grp
	$proto send-graft $src $grp
}

Class Iface/Timer -superclass Timer

Iface/Timer instproc init { protocol source group oiface sim} {
	$self instvar proto src grp oif ns
	set proto $protocol
	set src $source
	set grp $group
	set oif $oiface
	set ns $sim
}

Iface/Timer instproc schedule {} {
	$self sched [[$self info class] set timeout]
}

Class Prune/Iface/Timer -superclass Iface/Timer

Prune/Iface/Timer set timeout 0.5

Prune/Iface/Timer instproc timeout {} {
	$self instvar proto src grp oif
	$proto timeoutPrune $oif $src $grp
}

Class Deletion/Iface/Timer -superclass Iface/Timer

Deletion/Iface/Timer set timeout 0.1

Deletion/Iface/Timer instproc timeout {} {
	$self instvar proto src grp oif
	$proto delete_oif $src $grp $oif
}

###################################

Agent/Mcast/Control instproc transmit { msg id src grp } {
	set L [split $msg /]
	set type [lindex $L 0]
	set L [lreplace $L 0 0]
	$self send $type $id $src $grp $L
}















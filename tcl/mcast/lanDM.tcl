#
# tcl/mcast/lanDM.tcl
#
# Copyright (C) 1998 by USC/ISI
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
# 

# A dense mode capable of dealing with LANs.  A lot of cheating:
# "centralized" asserts plus everybody on the LAN keeps track of how
# many subscribers there are for a particular group.

Class lanDM -superclass DM

lanDM set PruneTimeout 0.5

lanDM instproc join-group  { group } {
	$self instvar node_
	$self next $group

	# LAN specific part: for each LAN we maintain an array of
	# counters of how many local receivers we have on the lan for a
	# given group
	set listOfReps [$node_ getReps * $group]
	set lanlist ""
	foreach r $listOfReps {
		if ![$r is-active] {
			set srcid [$r set srcID_]
			set nhop [$node_ rpf-nbr $srcid]
			if [$nhop is-lan?] {
				if { [lsearch $lanlist $nhop] < 0 } {
					lappend lanlist $nhop
					
					$nhop instvar receivers_
					if ![info exists receivers_($group)] {
						set receivers_($group) 0
					}
					incr receivers_($group)
				}
			}
		}
	}
}
lanDM instproc leave-group { group } {
        $self next $group
	# LAN specifics: decrement counters
	$self instvar node_
	set listOfReps [$node_ getReps * $group]
	set lanlist ""
	foreach r $listOfReps {
		if [$r is-active] {
			set srcid [$r set srcID_]
			set nhop [$node_ rpf-nbr $srcid]
			if [$nhop is-lan?] {
				if { [lsearch $lanlist $nhop] < 0 } {
					lappend lanlist $nhop
					# we want to decrement a counter per lan and only once, 
					# even if there are several sources are sending via it.
					$nhop instvar receivers_
					if [info exists receivers_($group)] {
						if { $receivers_($group) > 0 } {
							incr receivers_($group) -1
						}
					}
				}
			}
		}
	}
}

lanDM instproc drop { replicator src dst iface} {
	$self instvar node_ ns_

        if { $iface < 0  } {
		$replicator set ignore_ 1
        } else {
		set from [[$node_ iif2link $iface] src]
		if [$from is-lan?] {
			$self send-ctrl "prune" $src $dst
			return
		}
		$self send-ctrl "prune" $src $dst [$from id]
	}
}

# send a graft/prune for src/group up to the source or towards $to
lanDM instproc send-ctrl { which src group { to "" } } {
        $self instvar mctrl_ ns_ node_
	set id [$node_ id]
	set toid $src 
	if { $to != "" } {
		if [[$ns_ get-node-by-id $to] is-lan?] return
		set toid $to
	}
	set nbr [$node_ rpf-nbr $toid]
	if ![$nbr is-lan?] {
		$self next $which $src $group $to
		return
	}
	# we're requested to send a $which via a lan
	set lan_node $nbr
	set nbr [$lan_node rpf-nbr $toid]
	$lan_node instvar receivers_
	# send a graft/prune only if there're no other receivers on the lan
	if { ![info exists receivers_($group)] || \
			$receivers_($group) == 0 } {
		$self next $which $src $group [$nbr id]
		return
	}
}








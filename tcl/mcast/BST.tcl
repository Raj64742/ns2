#
# tcl/mcast/BST.tcl
#
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

###############
# Implementation of a simple shared bi-directional tree protocol.  No
# timers.  Nodes send grafts/prunes toward the RP to join/leave the
# group.  The user needs to set two protocol variables: 
#
# "BST set RP_($group) $node" - indicates that $node 
#                               acts as an RP for the $group

Class BST -superclass McastProtocol

BST instproc init { sim node } {
	$self instvar mctrl_ oiflist_
	BST instvar RP_

	$self next $sim $node

	set mctrl_ [new Agent/Mcast/Control $self]
	$node attach $mctrl_
}

BST instproc start {} {
	$self instvar node_ oiflist_
	BST instvar RP_

	# need to do it in start when unicast routing is computed
	foreach grpx [array names RP_] {
                set grp [expr $grpx]
               	$self dbg "BST: grp $grp, node [$node_ id]"
               	if { [string compare $grp $grpx] } {
    	           	set RP_($grp) $RP_(grpx)
        	      	unset RP_($grpx)
               	}
		set rpfiif [$node_ from-node-iface $RP_($grp)]
		if { $rpfiif != "?" } {
			set rpfoif [$node_ iif2oif $rpfiif]
		} else {
			set rpfoif ""
		}
		# initialize with the value of rpfoif
               	set oiflist_($grp) $rpfoif
	}

}

BST instproc join-group  { group {src "x"} } {
	$self instvar node_ ns_
	BST instvar RP_
	
	set r [$node_ getReps "x" $group]

	if {$r == ""} {
		set iif [$node_ from-node-iface $RP_($group)]
		$self dbg "********* join: adding <x, $group, $iif>"
		$node_ add-mfc "x" $group $iif ""
		set r [$node_ getReps "x" $group]
	}
	if { ![$r is-active] } {
		$self send-ctrl "graft" $RP_($group) $group
	}
	$self next $group ; #annotate
}

BST instproc leave-group { group {src "x"} } {
	BST instvar RP_
	$self next $group

	$self instvar node_
	set r [$node_ getReps "x" $group]
	if ![$r is-active] {
		$self send-ctrl "prune" $RP_($group) $group
	}
}

BST instproc handle-wrong-iif { srcID group iface } {
	$self instvar node_ oiflist_
	BST instvar RP_
	
	$self dbg "BST: wrong iif $iface, src $srcID, grp $group"
	$self dbg "\t oiflist: $oiflist_($group)"

	set rep [$node_ getReps "x" $group]
	
	$node_ add-mfc "x" $group $iface $oiflist_($group)
	set iif [$node_ lookup-iface "x" $group]
	if { $iface >= 0 } {
		set oif [$node_ iif2oif $iface]
		set rpfiif [$node_ from-node-iface $RP_($group)]
		if { $iface == $rpfiif } {
			# forward direction: disable oif to RP
			$rep disable [$node_ iif2oif $rpfiif]
		} else {
			# reverse direction: disable where it came from
			$rep disable $oif
			if { $node_ != $RP_($group) } {
				$rep insert [$node_ iif2oif $rpfiif]
			}
		}
	}
	$node_ change-iface "x" $group $iif $iface
	return 1 ;#classify packet again
}

BST instproc handle-cache-miss { srcID group iface } {
	$self instvar node_  ns_ oiflist_
	BST instvar RP_
	
	if { [$node_ getReps "x" $group] != "" } {
		debug 1
	}
	$self dbg "handle-cache-miss, src: $srcID, group: $group, iface: $iface"
	set rpfiif [$node_ from-node-iface $RP_($group)]
	if { $rpfiif != "?" } {
		set rpfoif [$node_ iif2oif $rpfiif]
	} else {
		set rpfoif ""
	}

	$self dbg "********* miss: adding <x, $group, $iface, $oiflist_($group)>"
	$node_ add-mfc "x" $group $iface $oiflist_($group)

	if { $iface > 0 } {
		#disable reverse iface
		set rep [$node_ getReps "x" $group]
		$rep disable [$node_ iif2oif $iface]
	}
	return 1 ;# classify the packet again.
}

BST instproc drop { replicator src dst iface} {
	$self instvar node_ ns_
	BST instvar RP_
	
	# No downstream listeners
	# Send a prune back toward the RP
	$self dbg "drops src: $src, dst: $dst, replicator: [$replicator set srcID_]"
	
	if {$iface >= 0} {
		# so, this packet came from outside of the node
		$self send-ctrl "prune" $RP_($dst) $dst
	}
}

BST instproc recv-prune { from src group iface} {
	$self instvar node_ ns_ oiflist_ 
	BST instvar RP_ 
	$self dbg "received a prune from: $from, src: $src, grp: $group, if: $iface"

	set rep [$node_ getReps "x" $group]
	if {$rep != ""} {
		set oif [$node_ iif2oif $iface]
		set idx [lsearch $oiflist_($group) $oif]
		if { $idx >= 0 } {
			set oiflist_($group) [lreplace $oiflist_($group) $idx $idx]
			$rep disable $oif
			set rpfiif [$node_ from-node-iface $RP_($group)]
			if { $rpfiif != "?" } {
				set rpfoif [$node_ iif2oif $rpfiif]
			} else {
				set rpfoif ""
	}
			if { $oiflist_($group) == $rpfoif && ![$node_ check-local $group] } {
				# propagate
				$self send-ctrl "prune" $RP_($group) $group
			}
		}
	}
}

BST instproc recv-graft { from to group iface } {
	$self instvar node_ ns_ oiflist_
	BST instvar RP_
	
	$self dbg "received a graft from: $from, to: $to, if: $iface"
	set oif [$node_ iif2oif $iface]
	set rpfiif [$node_ from-node-iface $RP_($group)]
	if { $rpfiif != "?" } {
		set rpfoif [$node_ iif2oif $rpfiif]
	} else {
		set rpfoif ""
	}

	if { $oiflist_($group) == $rpfoif && ![$node_ check-local $group] } {
		# propagate
		$self send-ctrl "graft" $RP_($group) $group
	}
	if { [lsearch $oiflist_($group) $oif] < 0 } {
		lappend oiflist_($group) $oif
		if { [$node_ lookup-iface "x" $group] != $iface } {
			set rep [$node_ getReps "x" $group]
			if { $rep != "" } {
				$rep insert $oif
			}
		}
	}
	$self dbg "oiflist: $oiflist_($group)"
}

#
# send a graft/prune for src/group up the RPF tree towards dst
#
BST instproc send-ctrl { which dst group } {
        $self instvar mctrl_ ns_ node_
	
	if {$node_ != $dst} {
		set nbr [$node_ rpf-nbr $dst]
		$ns_ simplex-connect $mctrl_ \
				[[[$nbr getArbiter] getType [$self info class]] set mctrl_]
		if { $which == "prune" } {
			$mctrl_ set class_ 30
		} else {
			$mctrl_ set class_ 31
		}        
		$mctrl_ send $which [$node_ id] -1 $group
	}
}

################ Helpers

BST instproc dbg arg {
	$self instvar ns_ node_
	puts [format "At %.4f : node [$node_ id] $arg" [$ns_ now]]
}





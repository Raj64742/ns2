#
# tcl/mcast/dynamicDM.tcl
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
Class dynamicDM -superclass DM

dynamicDM set ReportRouteTimeout 1

dynamicDM instproc notify { dummy } {
        $self instvar ns_ node_ PruneTimer_

	#build list of current sources
        foreach r [$node_ getReps "*" "*"] {
		set src_id [$r set srcID_]
		set sources($src_id) 1
	}
	set sourceIDs [array names sources]
	foreach src_id $sourceIDs {
		set src [$ns_ get-node-by-id $src_id]
		if {$src != $node_} {
			set upstream [$node_ rpf-nbr $src]
			if { $upstream != ""} {
				set inlink [$ns_ link $upstream $node_]
				set newiif [$node_ link2iif $inlink]
				set reps [$node_ getReps $src_id "*"]
				foreach r $reps {
					set oldiif [$node_ lookup-iface $src_id [$r set grp_]]
					if { $oldiif != $newiif } {
						$node_ change-iface $src_id [$r set grp_] $oldiif $newiif
					}
				}
			}
		}
		#next update outgoing interfaces
		set oiflist ""
		foreach nbr [$node_ neighbors] {
			set nbr_id [$nbr id]
			set nh [$nbr rpf-nbr $src] 
			if { $nh != $node_ } {
				# are we ($node_) the next hop from ($nbr) to 
				# the source ($src)
				continue
			}
			set oif [$node_ link2oif [$ns_ link $node_ $nbr]]
			# oif to such neighbor
			set oifs($oif) 1
		}
		set oiflist [array names oifs]

		set reps [$node_ getReps $src_id "*"]
		foreach r $reps {
			set grp [$r set grp_]
			set oldoifs [$r dump-oifs]
			set newoifs $oiflist
			foreach old $oldoifs {
				if [catch "$node_ oif2link $old" ] {
					# this must be a local agent, not an oif
					continue
				}
				set idx [lsearch $newoifs $old]
				if { $idx < 0} {
					$r disable $old
					if [info exists PruneTimer_($src_id:$grp:$old)] {
						delete $PruneTimer_($src_id:$grp:$old)
						unset PruneTimer_($src_id:$grp:$old)
					}
				} else {
					set newoifs [lreplace $newoifs $idx $idx]
				}
			}
			foreach new $newoifs {
				foreach r $reps {
					$r insert $new
				}
			}
		}
	}
}






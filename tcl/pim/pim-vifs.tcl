 #
 # tcl/pim/pim-vifs.tcl
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
 # Contributed by Ahmed Helmy, Ported by Polly Huang (USC/ISI), 
 # http://www-scf.usc.edu/~bhuang
 # 
 #
PIM instproc leave-group { group } {
        # essential while no timers
        $self instvar MRTArray Node
        if [info exists MRTArray($group)] {
                $self del-oif $MRTArray($group) [$Node id] "WC"
                return 1
        }
        #puts "node [$Node id] leave group and no WC Ent.. !!! ERROR"
        return 0
}

PIM instproc leave-group-source { group source } {
        # XXX TBDone
}

PIM instproc join-group { group } {
	# like accept_group_report
	# puts "$self join-group $group"
	$self add_leaf $group
}

PIM instproc join-group-source { group source } {
        # like accept_group_report
        # puts "$self join-group $group"
        $self add_source_leaf $group $source
}

PIM instproc add_source_leaf { group srcID } {
        $self instvar MRTArray
        set notfound 1
        if [info exists MRTArray($srcID:$group)] {
                set notfound 0
        } elseif { ! [$self findSG $srcID $group 1 [PIM set SG]] } {
                return 0
        }
        if $notfound {
           $self send-join "SG" $srcID $group
           set iif [$self get-iif-label [$MRTArray($srcID:$group) getNextHop]]
           $MRTArray($srcID:$group) setiif $iif
        }
}

# typically add_leaf is called with the vif number too... if we have igmp
PIM instproc add_leaf { group } {
	$self instvar Node MRTArray
	# puts "node [$Node id] $self addleaf $group"
  # XXX move the WKG proc into arbiter.. XXX
	if { $group == [PIM set ALL_PIM_ROUTERS] } {
		# puts "group is [PIM set ALL_PIM_ROUTERS]"
		return 0
	}
	set notfound 1
	if [info exists MRTArray($group)] {
		set notfound 0
	} elseif { ! [$self findWC $group 1] } { return 0 }
	if $notfound {
		$self send-join "WC" 0 $group
                set iif [$self get-iif-label [$MRTArray($group) getNextHop]]
                $MRTArray($group) setiif $iif
        } elseif { [$self compute-oifs $MRTArray($group)] == "" &&
                ! [$Node check-local $group] } {
                $self send-join "WC" 0 $group
	}
	$self check-sources $group $notfound
}

PIM instproc check-sources { group notfound } {
	$self instvar MRTArray groupArray Node

	# if there aren't any srcs... then return
	if { ![info exists groupArray($group)] } {
		return 0
	}

	# if I am the RP then don't chk, cuz won't trigger joins anyway
	if { [$Node id] == [$MRTArray($group) getRP] } {
		return 0
	}

	# if this is the first leaf
	if { [$Node check-local $group] > 0 } {
		return 0
	}

	set sourcesToPrune ""
	set join 0
	# if found entry, then I haven't triggered a join yet
	foreach src $groupArray($group) {
		set ent $MRTArray($src:$group)
		# prune sources for which you are registering
		if { [$ent getflags] & [PIM set REG] } {
			lappend sourcesToPrune $src
		} elseif { !$notfound && !$join && 
				[$ent get-prunelist] != "" } {
		   if { [$self compute-oifs $ent] == "" } {
			set join 1
		   }
		}
	}

	# join if necessary
	if $join { $self send-join "WC" 0 $group }

	# prune sources
	if { $join || $notfound } {
	   foreach src $sourcesToPrune {
		$self send-prune "SG_RP" $src $group
	   }
	}
}

PIM instproc except-prunes { oiflist prune } {
        foreach pif $prune {
		if { [set k [lsearch -exact $oiflist $pif]] != -1 } {
                        set oiflist [lreplace $oiflist $k $k]
		}
	}
	return $oiflist
}

PIM instproc calculate-oifs { ent oifs prunes } {
	 set oiflist [$self mergeoifs $oifs [$ent get-oiflist]]
	 set oiflist [$self except-prunes $oiflist $prunes]
	 return $oiflist
}

PIM instproc add-oif { ent origin code } {
	$self instvar Node ns MRTArray groupArray 
	# puts "node [$Node id] adding oif $origin"
        set link [$ns set link_([$Node id]:$origin)]
        set oifInfo [$Node get-oif $link]

	set oldoifs [$ent get-oiflist]
	set index [lindex $oifInfo 0]
	set newOif [lindex $oifInfo 1]
	$ent add-oif $index $newOif

	# XXX chk all S,Gs under *,G and mod as necessary.. 
	# especially check the S,G caches... 
	if { [$ent info class] == "WCEnt" } {
	  set group [$ent get-group]
	  if [info exists groupArray($group)] {
	    set locals [$Node check-local $group]
	    foreach source $groupArray($group) {
                if { $source == $origin } { continue }
                if { [$link info class] == "DummyLink" } {
                   set lofnodes [[$link getContainingObject] set nodes_]
                   set directlyConnected 0
                   foreach n $lofnodes {
                        if { $source == [$n id] } {
                                set directlyConnected 1
                                break
                        }
                   }
                   if $directlyConnected { continue }
                }
		set entry $MRTArray($source:$group)
		set modify 0
		set oldprunes ""
		if { [$entry is-pruned $index] } {
			set oldprunes [$entry get-prunelist]
			$entry remove-prune $index
			set modify 1
		} elseif { [$self calculate-oifs $entry $oldoifs $oldprunes]
                                != [$self compute-oifs $entry] } {
			set modify 1
		}
		if $modify {
			if { !$locals && 
				[$self calculate-oifs $entry $oldoifs $oldprunes] == "" } {
				$self send-join "WC" 0 $group
			}
		}
		if { $modify && [$entry getflags] & [PIM set CACHE] } {
			$Node add-mfc $source $group -1 $newOif
		}
	    }
	  }
	} elseif { [$ent info class == "SGEnt"] } {
	     if { [$ent getflags] & [PIM set CACHE] } {
                # XXX fix cache Ent.. !! not defined.. ! later
		if { [$cacheEnt is-pruned $index] } {
                        $cacheEnt remove-prune $index
		}

		$ent add-oif $index $newOif
		$Node add-mfc $source $group -1 $newOif
	     }
	}

}	

PIM instproc del-oif { ent origin code } {
        $self instvar Node ns groupArray MRTArray 
        # puts "node [$Node id] pruning oif $origin"
        if { $origin != [$Node id] } {
          set oifInfo [$Node get-oif [$ns set link_([$Node id]:$origin)]]
          set index [lindex $oifInfo 0]
          set oif [lindex $oifInfo 1]
        } else {
          set index -1
          set oif "local"
        }
       set group [$ent get-group]

        # XXX chk all S,Gs under *,G and mod as necessary..
        # especially check the S,G caches...
        if { [$ent info class] == "WCEnt" } {
          if [info exists groupArray($group)] {
            # XXX do we need to check group,src pairs
            set locals [$Node check-local $group]   
            foreach source $groupArray($group) {
                set entry $MRTArray($source:$group)
                if { ! [expr [$entry getflags] & [PIM set CACHE]] } {
                        continue
                }
                set oiflist [$self compute-oifs $entry]
                if { [set idx [$self index $oiflist $oif]] != -1
                        || $oif == "local" } {
                        if { $oif != "local" } {
                          $Node del-mfc $source $group $oif
                        }
                        if { [llength $oiflist] <= 1 && ! $locals } {
                                $self send-prune "SG_RP" $source $group
                        }
                }
            }
          }
          $ent rem-oif $index $oif
          return 1
          # XXX may need more clean up for cache lists... etc XXX
        } elseif { [$ent info class] == "SGEnt" } {
                $ent del-oif $index $oif
                if { [expr [$ent getflags] & [PIM set CACHE]] } {
                        set source [$ent set source]
                        $Node del-mfc $source $group $oif
                }
        }
}

PIM instproc index { oifs iface } {
        set k [lsearch -exact $oifs $iface]
        return $k
}

PIM instproc compute-oif-ids { ent } {
        set oiflist [$self compute-oifs $ent]
        return [$self oifs2labels $oiflist]
}

PIM instproc oifs2labels { oiflist } {  
        set labelList ""  
        foreach oif $oiflist {
                lappend labelList [$oif set intf_label_]
        }
        return $labelList
}
 
PIM instproc get-iif-label { nxthop } {
        $self instvar Node ns
        if { $nxthop == [$Node id] } {
                return -1
        }
        set oifInfo [$Node get-oif [$ns set link_([$Node id]:$nxthop)]]
        set oif [lindex $oifInfo 1]   
        #puts "oif is $oif [$oif info class]" 
        return [$oif set intf_label_]
}

PIM instproc mergeoifs { list1 list2 } {
	set reslist $list1
	foreach el $list2 {
	  if { [lsearch $list1 $el] == -1 } {
		lappend reslist $el
	  }
	}
	return $reslist
}

PIM instproc compute-oifs { ent } {
	$self instvar MRTArray
	set oiflist ""
	if { [$ent info class] == "SGEnt" } {
		if [info exists MRTArray([$ent get-group])] {
		   set oiflist [$MRTArray([$ent get-group]) get-oiflist]
  		   set oiflist [$self mergeoifs $oiflist [$ent get-oiflist]]
		   # XXX remove iif(S,G) from the oiflist
		} else {
			set oiflist [$ent get-oiflist]
		}
		set oiflist [$self exclude-prunes $ent $oiflist]
	}
	return $oiflist
}

PIM instproc exclude-prunes { ent oiflist } {
	foreach pif [$ent get-prunelist] {
		if { [set k [lsearch -exact $oiflist $pif]] != -1 } {
			set oiflist [lreplace $oiflist $k $k]
		}
	}
	return $oiflist
}

PIM instproc nexthop { dst } {
	$self instvar ns Node
	return [[$ns upstream-node [$Node id] $dst] id]
} 

PIM instproc nexthopWC { group } {
	$self instvar MRTArray
	set ent $MRTArray($group)
	if { [set nxt [$ent getNextHop]] != -1 } {
		return $nxt
	}
	set nxt [$self nexthop [$ent getRP]]
	$ent setNextHop $nxt
	return $nxt
}

PIM instproc nexthopSG { source group } {
        $self instvar MRTArray
        set ent $MRTArray($source:$group)
        if { [set nxt [$ent getNextHop]] != -1 } {
                return $nxt  
        }
        set nxt [$self nexthop $source]
        $ent setNextHop $nxt
        return $nxt
}


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
#
Class dynamicDM -superclass DM

dynamicDM set ReportRouteTimeout 1

dynamicDM instproc init { sim node } {
	$self next $sim $node
}

dynamicDM instproc start { } {
        $self periodic-check
}

dynamicDM instproc periodic-check {} {
        $self instvar ns_
        $self check-downstream-list
	$ns schedule-after [$class set ReportRouteTimeout] $self $proc
}

dynamicDM instproc notify changes {
	$self check-downstream-list
}

dynamicDM instproc check-downstream-list {} {
	$self instvar ns_ downstreamList_ node_ pruneTimer_ iif_

	set neighbor [$Node set neighbor_]
	set id [$Node set id_]

        foreach Anode [$ns all-nodes-list] {

	    set oiflist ""
	    set n1 [$Anode set id_]
	    foreach node $neighbor {
		set nbr [$node id]
		set oifInfo [$Node RPF-interface $n1 $id $nbr]
		if { $oifInfo != "" } {
		    set oif [lindex $oifInfo 1]
		    set k [lsearch -exact $oiflist $oif]
		    if { $k < 0 } {
			lappend oiflist $oif
		    }
		}
	    }

	    #update iif
	    if {$Anode != $Node} {
		if [info exists iif_($n1)] {
		    set oldiface $iif_($n1)
		}
		set upstream [$ns upstream-node $id $n1]	
		if { $upstream != ""} {
		    set iilink [$ns RPF-link $n1 [$upstream id] $id]
		    set iif_($n1) [[$iilink set dest_] id]
		}
		if [info exists oldiface] {
		    foreach pair [$Node getRepBySource $n1] {
			set pair [split $pair :]
			set group [lindex $pair 0]
			set r [lindex $pair 1]
			if {$oldiface != $iif_($n1)} {
			    $r change-iface $n1 $group $oldiface $iif_($n1)
			}
		    }
		}
	    }
	    
	    if [info exists DownstreamList_($n1)] {
		set oldlist $DownstreamList_($n1)
		# puts "$oldlist"
	    }
	    set DownstreamList_($n1) [lsort -increasing $oiflist]
	    # puts "$DownstreamList_($n1)"

	    if [info exists oldlist] {
		set m [llength $oldlist]
		set n [llength $DownstreamList_($n1)]
		# puts "$m, $n"
		set j 0
		set k 0
		while { $j < $m || $k < $n} {
		    set old [lindex $oldlist $j]
		    set new [lindex $DownstreamList_($n1) $k]
		    if {($n == $k) || (($m != $j) && ($old < $new))} {
			foreach pair [$Node getRepBySource $n1] {
			    set pair [split $pair :]
			    set group [lindex $pair 0]
			    set r [lindex $pair 1]
			    if [$r exists $old] {
				if [$r set active_($old)] {
				    $r disable $old
				} else {
				    $ns cancel $PruneTimer_($n1:$group:$old)
				}
			    } else {
				puts "panic: why $old not in old downstream list"
			    }
			}
			incr j
		    } elseif {$m == $j || ($n != $k && $old > $new)} {
			foreach pair [$Node getRepBySource $n1] {
			    set pair [split $pair :]
			    set r [lindex $pair 1]
			    $r insert $new
			}
			incr k
		    } else {
			incr j
			incr k
		    }
		}
	    }
	}
}

dynamicDM instproc handle-cache-miss { srcID group iface } {
        $self instvar node_ ns_
        # init a list of lan indexes
        set indexList ""
	set oiflist ""
	set id [$node_ id]

        # in the future this should be foreach iface $interfaces
        foreach node [$node_ neighbor] {
           set nbr [$node id]
           set oifinfo [$node_ RPF-interface $srcID $id $nbr]
           if { $oifInfo != "" } {
                set index [lindex $oifInfo 0]
                set oif [lindex $oifInfo 1]
                set k [lsearch -exact $indexList $index]
                if { $k < 0 } {
                        lappend indexList $index
                        lappend oiflist $oif
                }
           }
        }
	# got to find iface
	if {$srcID == $id} {
	    set iif -2
	} else {
	    set upstream [$ns_ upstream-node $id $srcID]	
	    set iilink [$ns_ RPF-link $srcID [$upstream id] $id]
	    set iif [[$iilink set dest_] id]
	}
	$node_ add-mfc $srcID $group $iif $oiflist
}


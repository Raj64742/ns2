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

Class DM -superclass McastProtocol

DM set PruneTimeout 0.5

DM instproc init { sim node } {
	$self instvar prune_
	set prune_ [new Agent/Mcast/Control $self]
	$node attach $prune_
	$self next $sim $node
}

DM instproc join-group  { group } {
        $self next $group

	$self instvar node_
	foreach r [$node_ getReps * $group] {	;# XXX
		if ![$r is-active] {
			$self send-ctrl graft [$r set srcID_] $group
		}
	}
}

DM instproc leave-group { group } {
        $self next $group
}

DM instproc handle-cache-miss { srcID group iface } {
        $self instvar node_
	set iif [$self find-oifs $srcID $group $iface]
	$node_ add-mfc $srcID $group $iif [$self find-oifs $srcID $group $iif]
	return 1
}

DM instproc find-oifs {s g ifc} {
	return -1
}

DM instproc find-oifs {src grp iif} {
	$self instvar node_

	set id [$node_ id]
	set oiflist ""
        # in the future this should be foreach iface $interfaces
	# yea, right...sure....whatever
	foreach nbr [$node_ neighbors] {
        	set oifInfo [$node_ RPF-interface $src $id [$nbr id]]
        	if { $oifInfo != "" && ![info exists oifSeen($oifInfo)] } {
			lappend oiflist $oifInfo
			set oifSeen($oifInfo) 1
        	}
        }
	set oiflist
}

DM instproc drop { replicator src dst } {
	$self instvar node_

	#
        # No downstream listeners
        # Send a prune back toward the source
        #
        if { $src == [$node_ id] } {
                #
                # if we are trying to prune ourself (i.e., no
                # receivers anywhere in the network), set the
                # ignore bit in the object (we turn it back on
                # when we get a graft).  This prevents this
                # drop method from being called on every packet.
                #
		$replicator set ignore_ 1
        } else {
	        $self send-ctrl prune $src $dst
        }
}

DM instproc recv-prune { from src group iface} {
        $self instvar node_ PruneTimer_ ns_

	set r [$node_ getReps $src $group]
	if { $r == "" } { 
		return 0
	}

	set id [$node_ id]
	set oifInfo [$node_ RPF-interface $src $id $from]
	set tmpoif  [[$ns_ link $id $from] head]
	if ![$r exists $tmpoif] {
		warn {trying to prune a non-existing interface?}
	} else {
		$r instvar active_
		if $active_($tmpoif) {
			$r disable $tmpoif
			set PruneTimer_($src:$group:$tmpoif) [$ns_	\
					after [DM set PruneTimeout]	\
					"$r enable $tmpoif"]
		}
		# ELSE
		#puts "recv prune when iface is already pruned"
		#$ns_ cancel $PruneTimer_($src:$group:$tmpoif)
		#set PruneTimer_($src:$group:$tmpoif) [$ns_ after [DM set PruneTimeout] "$r enable $tmpoif"]
	}

        #
        # If there are no remaining active output links
        # then send a prune upstream.
        #
        $r instvar nactive_
        if {$nactive_ == 0 && $src != $id} {
		$self send-ctrl prune $src $group
	}
}

DM instproc recv-graft { from src group iface } {
        $self instvar node_ PruneTimer_ ns_
	set id [$node_ id]
	set r [$node_ getReps $src $group]
        if { ![$r is-active] && $src != $id } {
                #
                # the replicator was shut-down and the
                # source is still upstream so propagate
                # the graft up the tree
                #
                $self send-ctrl graft $src $group
        }
        #
        # restore the flow
        #
	set tmpoif [[$ns_ link $id $from] head]
	$r instvar active_
	if {[$r exists $tmpoif] && !($active_($tmpoif))} {
		$ns_ cancel $PruneTimer_($src:$group:$tmpoif)
	}
        $r enable $tmpoif
}

#
# send a graft/prune for src/group up the RPF tree
#
DM instproc send-ctrl { which src group } {
        $self instvar prune_ ns_ node_
	set id [$node_ id]
        set na   [[$ns_ upstream-node $id $src] getArbiter]  ;# nbr Arbiter
	set ndma [$na getType [$self info class]]            ;# nbr DM agent
	$ns_ simplex-connect $prune_ [$ndma set prune_]
        $prune_ send $which $id $src $group
}

DM instproc dump-routes {chan args} {
	$self instvar ns_ node_
	if { [llength $args] == 0 } {
		# dump all replicator entries
		set reps [$node_ getReps-raw * *]   ;# now isn't that cute? :-)
	} elsif { [llength $args] == 1 } {
		# dump entries for group
		set g [lindex $args 0]
		set reps [$node_ getReps-raw * $g]  ;# actually, more than *,g
	} elsif { [llength $args] == 2 } {
		# dump entries for src, group.
		set s [lindex $args 0]
		set g [lindex $args 1]
		set reps [$node_ getReps-raw $s $g]
	} else {
		carp-real-bad "$proc: invalid number of arguments"
		return
	}
	if { $reps == "" } {
		# really, we'd like a hypothetical answer based on RPF chks.
		puts $chan "no multicast entries for $args (yet)"
		return
	}
	puts $chan [concat "Node:\t${node_}([$node_ id])\tat t ="	\
			[format "%4.2f" [$ns_ now]]]
	puts $chan "repTag\tState\t\tsrc  group  iif\t\toiflist"
	array set areps $reps
	foreach ent [lsort -command sort-s-g [array names areps]] {
		set sg [split $ent ":"]
		if { [$areps($ent) is-active] } {
			set state Y
		} else {
			set state N
		}
		# really, I want to translate each adjacent to a link,
		# and then the neighbour node.
		set fwds ""
		foreach {key val} [$areps($ent) adjacents] {
			# throw away the key
			lappend fwds $val
		}
		# also, what about pruned sources?
		puts $chan [format "%5s\t  %s\t%3d  %5d   -1\t\t%s"	\
				$areps($ent) $state			\
				[lindex $sg 0] [lindex $sg 1] $fwds]
	}
}
#####

#
# tcl/mcast/STsrc.tcl
#
# Simple Shared Tree (as ST.tcl), plus source-specific (SRC.tcl)
#

Class STsrc -superclass {ST SRC}

STsrc instproc init { sim node } {
	ST instvar Groups_ RP_
	set Groups_ [STsrc set Groups_]
	array set RP_ [STsrc array get RP_]
	STsrc superclass ST
	$self next $sim $node
}

STsrc instproc join-group { group {src "*"} } { 
	if {$src == "*"} {
		# shared tree join
		STsrc superclass ST
	} else {
		# source-specific join
		STsrc superclass SRC
	}
	$self next $group $src
}

STsrc instproc leave-group { group {src "*"}} { 
	if {$src == "*"} {
		# shared tree leave
		STsrc superclass ST
	} else {
		# source-specific leave
		STsrc superclass SRC
	}
	$self next $group $src
}

STsrc instproc recv-graft { from to group } {
	STsrc instvar RP_

	if {$to == [$RP_($group) id]} {
		# graft on to the shared tree
		STsrc superclass ST
	} else {
		# souce specific
		STsrc superclass SRC
	}
	$self next $from $to $group
}

STsrc instproc recv-prune { from to group } {
	STsrc instvar RP_

	if {$to == [$RP_($group) id]} {
		# prune off the shared tree
		STsrc superclass ST
	} else {
		# souce specific
		STsrc superclass SRC
	}
	$self next $from $to $group
}

STsrc instproc drop { replicator src dst iface} {
	STsrc instvar RP_
	if {$iface != "?"} {
		# send a prune back on the iface??? note: back iface is different
	} else {
		$replicator set ignore_ 1
	}
}

STsrc instproc handle-wrong-iif { srcID group iface } {
	$self dbg "wrong iif $iface, src $srcID, grp $group"
}

STsrc instproc handle-cache-miss { srcID group iface } {
	$self instvar node_
	$self dbg "cache miss, src: $srcID, group: $group, iface: $iface"
	STsrc superclass ST
	$self next $srcID $group $iface
#	$self dbg "   adding <$srcID, $group, $iface>"
#	$node_ add-mfc $srcID $group $iface ""
}

STsrc instproc dbg arg {
	$self instvar ns_ node_ id_
	puts [format "At %.4f : STsrc: node $id_ $arg" [$ns_ now]]
}

#####



Class SessionSim -superclass Simulator

SessionSim instproc bw_parse { bspec } {
        if { [scan $bspec "%f%s" b unit] == 1 } {
                set unit b
        }
	# xxx: all units should support X"ps" --johnh
        switch $unit {
        b  { return $b }
        bps  { return $b }
        kb { return [expr $b*1000] }
        Mb { return [expr $b*1000000] }
        Gb { return [expr $b*1000000000] }
        default { 
                  puts "error: bw_parse: unknown unit `$unit'" 
                  exit 1
                }
        }
}

SessionSim instproc delay_parse { dspec } {
        if { [scan $dspec "%f%s" b unit] == 1 } {
                set unit s
        }
        switch $unit {
        s  { return $b }
        ms { return [expr $b*0.001] }
        ns { return [expr $b*0.000001] }
        default { 
                  puts "error: bw_parse: unknown unit `$unit'" 
                  exit 1
                }
        }
}

SessionSim instproc node args {
    $self instvar Node_
    set node [new SessionNode $args]
    set Node_([$node id]) $node
    return $node
}

SessionSim instproc create-session { node agent } {
    $self instvar session_

    set nid [$node id]
    set dst [$agent set dst_]
    set session_($nid:$dst) [new SessionHelper]
    $session_($nid:$dst) set-node $nid
	
	# If exists nam-traceall, we'll insert an intermediate trace object
	set trace [$self get-nam-traceall]
	if {$trace != ""} {
		# This will write every packet sent and received to 
		# the nam trace file
		set p [$self create-trace SessEnque $trace $nid $dst "nam"]
		$agent target $p
		$p target $session_($nid:$dst)
	} else {
		$agent target $session_($nid:$dst)
	}

    if [SessionSim set rc_] {
	$session_($nid:$dst) set rc_ 1
    }
	return $session_($nid:$dst)
}

SessionSim instproc update-loss-dependency { src dst agent group } {
    $self instvar session_ routingTable_ loss_

    set loss_rcv 1
    set tmp $dst
    while {$tmp != $src} {
	set next [$routingTable_ lookup $tmp $src]
	if {[info exists loss_($next:$tmp)] && $loss_($next:$tmp) != 0} {
	    if {$loss_rcv} {
		#puts "update-loss-rcv $loss_($next:$tmp) $next $tmp $agent"
		set dep_loss [$session_($src:$group) update-loss-rcv $loss_($next:$tmp) $agent]
	    } else {
		#puts "update-loss-rcv $loss_($next:$tmp) $next $tmp $dep_loss"
		set dep_loss [$session_($src:$group) update-loss-loss $loss_($next:$tmp) $dep_loss]
	    }

	    if {$dep_loss == 0} { 
		return 
	    }
	    set loss_rcv 0
	}
	set tmp $next
    }

    if [info exists dep_loss] {
	$session_($src:$group) update-loss-top $dep_loss
    }
}

SessionSim instproc join-group { agent group } {
    $self instvar session_ routingTable_ delay_ link_

    foreach index [array names session_] {
	set pair [split $index :]
	if {[lindex $pair 1] == $group} {
	    set src [lindex $pair 0]
	    set dst [[$agent set node_] id]
	    set delay 0
	    set accu_bw 0
	    set ttl 0
	    set tmp $dst
	    while {$tmp != $src} {
		set next [$routingTable_ lookup $tmp $src]
		set delay [expr $delay + $delay_($tmp:$next)]
		if {$accu_bw} {
		    set accu_bw [expr 1 / (1 / $accu_bw + 1 / $link_($tmp:$next))]
		} else {
		    set accu_bw $link_($tmp:$next)
		}
		incr ttl
		set tmp $next
	    }

	    # Create nam queues for all receivers if traceall is turned on
	    # XXX 
	    # nam will deal with the issue whether all groups share a 
	    # single queue per receiver. The simulator simply writes 
	    # this information there
	    $self puts-nam-config "G -t [$self now] -i $group -a $dst"

	    # And we should add a trace object before each receiver,
	    # because only this will capture the packet before it 
	    # reaches the receiver and after it left the sender
	    set f [$self get-nam-traceall]
	    if {$f != ""} { 
		    set p [$self create-trace SessDeque $f $src $dst "nam"]
		    $p target $agent
		    $session_($index) add-dst $accu_bw $delay $ttl $dst $p
		    $self update-loss-dependency $src $dst $p $group
	    } else {
		    #puts "add-dst $accu_bw $delay $ttl $src $dst"
		    $session_($index) add-dst $accu_bw $delay $ttl $dst $agent
		    $self update-loss-dependency $src $dst $agent $group
	    }

	}
    }
}

SessionSim instproc insert-loss { lossmodule from to } {
    $self instvar loss_ link_

    if [info exists link_($from:$to)] {
	set loss_($from:$to) $lossmodule
    }
}

SessionSim instproc get-delay { src dst } {
    $self instvar routingTable_ delay_
    set delay 0
    set tmp $src
    while {$tmp != $dst} {
	set next [$routingTable_ lookup $tmp $dst]
	set delay [expr $delay + $delay_($tmp:$next)]
	set tmp $next
    }
    return $delay
}

SessionSim instproc get-bw { src dst } {
    $self instvar routingTable_ link_
    set accu_bw 0
    set tmp $src
    while {$tmp != $dst} {
	set next [$routingTable_ lookup $tmp $dst]
	if {$accu_bw} {
	    set accu_bw [expr 1 / (1 / $accu_bw + 1 / $link_($tmp:$next))]
	} else {
	    set accu_bw $link_($tmp:$next)
	}
	set tmp $next
    }
    return $accu_bw
}

SessionSim instproc leave-group { agent group } {
    $self instvar session_

    foreach index [array names session_] {
	set pair [split $index :]
	if {[lindex $pair 1] == $group} {
	    #$session_($index) delete-dst [[$agent set node_] id] $agent
		set dst [[$agent set node_] id]
		# remove the receiver from packet distribution list
		$self puts-nam-traceall \
			"G -t [$self now] -i $group -x $dst"
	}
    }
}

SessionSim instproc simplex-link { n1 n2 bw delay type } {
    $self instvar link_ delay_ linkAttr_
    set sid [$n1 id]
    set did [$n2 id]

    set link_($sid:$did) [$self bw_parse $bw]
    set delay_($sid:$did) [$self delay_parse $delay]

	set linkAttr_($sid:$did:ORIENT) ""
	set linkAttr_($sid:$did:COLOR) "black"
}

SessionSim instproc duplex-link { n1 n2 bw delay type } {
    $self simplex-link $n1 $n2 $bw $delay $type
    $self simplex-link $n2 $n1 $bw $delay $type

	$self register-nam-linkconfig [$n1 id]:[$n2 id]
}

SessionSim instproc simplex-link-of-interfaces { n1 n2 bw delay type } {
    $self simplex-link $n1 $n2 $bw $delay $type
}

SessionSim instproc duplex-link-of-interfaces { n1 n2 bw delay type } {
    $self simplex-link $n1 $n2 $bw $delay $type
    $self simplex-link $n2 $n1 $bw $delay $type

	$self register-nam-linkconfig [$n1 id]:[$n2 id]
}

# Assume ops to be performed is 'orient' only
# XXX Poor hack. What should we do without a link object??
SessionSim instproc duplex-link-op { n1 n2 op args } {
	$self instvar linkAttr_ link_

	set sid [$n1 id]
	set did [$n2 id]

	if ![info exists link_($sid:$did)] {
		error "Non-existent link [$n1 id]:[$n2 id]"
	}

	switch $op {
		"orient" {
			set linkAttr_($sid:$did:ORIENT) $args
			set linkAttr_($did:$sid:ORIENT) $args
		}
		"color" {
			set ns [Simulator instance]
			$ns puts-nam-traceall \
				[eval list "l -t [$self now] -s $sid -d $did \
-S COLOR -c $args -o $linkAttr_($sid:$did:COLOR)"]
			$ns puts-nam-traceall \
				[eval list "l -t [$self now] -s $did -d $sid \
-S COLOR -c $args -o $linkAttr_($sid:$did:COLOR)"]
			eval set attr_($sid:$did:COLOR) $args
			eval set attr_($did:$sid:COLOR) $args
		}
		default {
			eval puts "Duplex link option $args not implemented \
in SessionSim"
		}
	} 
}

SessionSim instproc compute-routes {} {
	$self instvar link_
	#
	# Compute all the routes using the route-logic helper object.
	#
        set r [$self get-routelogic]
	foreach ln [array names link_] {
		set L [split $ln :]
		set srcID [lindex $L 0]
		set dstID [lindex $L 1]
	        if {$link_($ln) != 0} {
			$r insert $srcID $dstID
		} else {
			$r reset $srcID $dstID
		}
	}
	$r compute
}

SessionSim instproc compute-hier-routes {} {
        $self instvar link_
        set r [$self get-routelogic]
        #
        # send hierarchical data :
        # array of cluster size, #clusters, #domains
        # assuming 3 levels of hierarchy --> this should be extended to
	# support
        # n-levels of hierarchy
        #
        puts "Computing Hierarchical routes\n"
        set level [AddrParams set hlevel_]
        $r hlevel-is $level
        $self hier-topo $r

        foreach ln [array names link_] {
                set L [split $ln :]
                set srcID [[$self get-node-by-id [lindex $L 0]] node-addr]
                set dstID [[$self get-node-by-id [lindex $L 1]] node-addr]
                if { $link_($ln) != 0 } {
#                        $r hier-insert $srcID $dstID $link_($ln)
                        $r hier-insert $srcID $dstID
                } else {
                        $r hier-reset $srcID $dstID
                }
        }       
        $r hier-compute
}



# Because here we don't have a link object, we need to have a new 
# link register method
SessionSim instproc register-nam-linkconfig link {
	$self instvar linkConfigList_ link_ linkAttr_
	if [info exists linkConfigList_] {
		# Check whether the reverse simplex link is registered,
		# if so, don't register this link again.
		# We should have a separate object for duplex link.
		set tmp [split $link :]
		set i1 [lindex $tmp 0]
		set i2 [lindex $tmp 1]
		if [info exists link_($i2:$i1)] {
			set pos [lsearch $linkConfigList_ $i2:$i1]
			if {$pos >= 0} {
				set a1 $linkAttr_($i2:$i1:ORIENT)
				set a2 $linkAttr_($link:ORIENT)
				if {$a1 == "" && $a2 != ""} {
					# If this duplex link has not been 
					# assigned an orientation, do it.
					set linkConfigList_ \
					[lreplace $linkConfigList_ $pos $pos]
				} else {
					return
				}
			}
		}

		# Remove $link from list if it's already there
		set pos [lsearch $linkConfigList_ $link]
		if {$pos >= 0} {
			set linkConfigList_ \
				[lreplace $linkConfigList_ $pos $pos]
		}
	}
	lappend linkConfigList_ $link
}

# write link configurations
SessionSim instproc dump-namlinks {} {
	$self instvar link_ delay_ linkConfigList_ linkAttr_

	set ns [Simulator instance]
	foreach lnk $linkConfigList_ {
		set tmp [split $lnk :]
		set i1 [lindex $tmp 0]
		set i2 [lindex $tmp 1]
		$ns puts-nam-traceall \
			"l -t * -s $i1 -d $i2 -S UP -r $link_($lnk) -D \
$delay_($lnk) -o $linkAttr_($lnk:ORIENT)"
	}
	
}

SessionSim instproc run args {
        $self rtmodel-configure                 ;# in case there are any
        [$self get-routelogic] configure
	$self instvar scheduler_ Node_ started_

	set started_ 1

	#
	# Reset every node, which resets every agent
	#
	foreach nn [array names Node_] {
		$Node_($nn) reset
	}

	# We don't have queues in SessionSim
	$self dump-namcolors
	$self dump-namnodes
	$self dump-namlinks
	$self dump-namagents

        return [$scheduler_ run]
}

# Get multicast tree in session simulator: By assembling individual 
# (receiver, sender) paths into a SPT.
# src is a Node.
SessionSim instproc get-mcast-tree { src grp } {
	$self instvar treeLinks_ session_

	if [info exists treeLinks_] {
		unset treeLinks_
	}

	set sid [$src id] 

	# get member list
	foreach idx [array names session_] {
		set pair [split $idx :]
		if {[lindex $pair 0] == $sid && [lindex $pair 1] == $grp} {
			set mbrs [$session_($idx) list-mbr]
			break
		}
	}		

	foreach mbr $mbrs {
		# Find path from $mbr to $src
		while {![string match "Agent*" [$mbr info class]]} {
			# In case agent is at the end of the chain... 
			set mbr [$mbr target]
		}
		set mid [[$mbr set node_] id]
		if {$sid == $mid} {
			continue
		}
		# get paths for each individual member
		$self merge-path $sid $mid
	}

	# generating tree link list
	foreach lnk [array names treeLinks_] {
		lappend res $lnk $treeLinks_($lnk)
	}
	return $res
}

# Merge the path from mbr to src
# src is node id.
SessionSim instproc merge-path { src mbr } {
	$self instvar routingTable_ treeLinks_ link_

	# get paths from mbr to src and merge into treeLinks_
	set tmp $mbr
	while {$tmp != $src} {
		set nxt [$routingTable_ lookup $tmp $src]
		# XXX 
		# Assume routingTable lookup is always successful, so 
		#   don't validate existence of link_($tid:$sid)
		# Always arrange tree links in (parent, child).
		if ![info exists treeLinks_($nxt:$tmp)] {
			set treeLinks_($nxt:$tmp) $link_($nxt:$tmp)
		}
		if [info exists treeLinks_($tmp:$nxt)] {
			error "Reverse links in a SPT!"
		}
		set tmp $nxt
	}
}


############## SessionNode ##############
Class SessionNode -superclass Node
SessionNode instproc init args {
    $self instvar id_ np_ address_
    set id_ [Node getid]
    set np_ 0
    if {[llength $args] > 0} {
	set address_ $args
    } else {
        set address_ $id_
    }
}

SessionNode instproc id {} {
    $self instvar id_
    return $id_
}

SessionNode instproc reset {} {
}

SessionNode instproc alloc-port {} {
    $self instvar np_
    set p $np_
    incr np_
    return $p
}

SessionNode instproc attach agent {
    $self instvar id_ address_

    $agent set node_ $self
    set port [$self alloc-port]

    if [Simulator set EnableHierRt_] {
	set nodeaddr [AddrParams set-hieraddr $address_]
    } else {
	set nodeaddr [expr [expr $address_  & [AddrParams set NodeMask_(1)]] \
			  << [AddrParams set NodeShift_(1)]]
    }
    set mask [AddrParams set PortMask_]
    set shift [AddrParams set PortShift_]
    $agent set addr_ [expr [expr [expr $port & $mask] << $shift] | \
		      [expr [expr ~[expr $mask << $shift]] & $nodeaddr]]
    # $agent set addr_ [expr $id_ << 8 | $port]
}

SessionNode instproc join-group { agent group } {
    set group [expr $group]
    [Simulator instance] join-group $agent $group
}

SessionNode instproc leave-group { agent group } {
    set group [expr $group]
    [Simulator instance] leave-group $agent $group
}


Agent/LossMonitor instproc show-delay { seqno delay } {
    $self instvar node_

    puts "[$node_ id] $seqno $delay"
}


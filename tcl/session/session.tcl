Class SessionSim -superclass Simulator
SessionSim set MixMode_ 0

### Create a session helper that associates with the src agent ###
SessionSim instproc create-session { srcNode srcAgent } {
    $self instvar session_

    set nid [$srcNode id]
    set dst [$srcAgent set dst_]
    set session_($nid:$dst:$nid) [new SessionHelper]
    $session_($nid:$dst:$nid) set-node $nid
    if [SessionSim set rc_] {
	$session_($nid:$dst:$nid) set rc_ 1
    }
	
    # If exists nam-traceall, we'll insert an intermediate trace object
    set trace [$self get-nam-traceall]
    if {$trace != ""} {
	# This will write every packet sent and received to 
	# the nam trace file
	set p [$self create-trace SessEnque $trace $nid $dst "nam"]
	$srcAgent target $p
	$p target $session_($nid:$dst:$nid)
    } else {
	$srcAgent target $session_($nid:$dst:$nid)
    }

    return $session_($nid:$dst:$nid)
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
		set dep_loss [$session_($src:$group:$src) update-loss-rcv $loss_($next:$tmp) $agent]
	    } else {
		#puts "update-loss-rcv $loss_($next:$tmp) $next $tmp $dep_loss"
		set dep_loss [$session_($src:$group:$src) update-loss-loss $loss_($next:$tmp) $dep_loss]
	    }

	    if {$dep_loss == 0} { 
		return 
	    }
	    set loss_rcv 0
	}
	set tmp $next
    }

    if [info exists dep_loss] {
	$session_($src:$group:$src) update-loss-top $dep_loss
    }
}

SessionSim instproc join-group { rcvAgent group } {
    $self instvar session_ routingTable_ delay_ bw_

    foreach index [array names session_] {
	set pair [split $index :]
	if {[lindex $pair 1] == $group} {
	    set src [lindex $pair 0]
	    set dst [[$rcvAgent set node_] id]
	    set delay 0
	    set accu_bw 0
	    set ttl 0
	    set tmp $dst
	    while {$tmp != $src} {
		set next [$routingTable_ lookup $tmp $src]
		set delay [expr $delay + $delay_($tmp:$next)]
		if {$accu_bw} {
		    set accu_bw [expr 1 / (1 / $accu_bw + 1 / $bw_($tmp:$next))]
		} else {
		    set accu_bw $bw_($tmp:$next)
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
		$p target $rcvAgent
		$session_($index) add-dst $accu_bw $delay $ttl $dst $p
		$self update-loss-dependency $src $dst $p $group
	    } else {
		#puts "add-dst $accu_bw $delay $ttl $src $dst"
		$session_($index) add-dst $accu_bw $delay $ttl $dst $rcvAgent
		$self update-loss-dependency $src $dst $rcvAgent $group
	    }
	}
    }
}

SessionSim instproc leave-group { rcvAgent group } {
    $self instvar session_

    foreach index [array names session_] {
	set pair [split $index :]
	if {[lindex $pair 1] == $group} {
	    #$session_($index) delete-dst [[$rcvAgent set node_] id] $rcvAgent
		set dst [[$rcvAgent set node_] id]
		# remove the receiver from packet distribution list
		$self puts-nam-traceall \
			"G -t [$self now] -i $group -x $dst"
	}
    }
}

SessionSim instproc insert-loss { lossmodule from to } {
    $self instvar loss_ bw_

    if [info exists bw_($from:$to)] {
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
    $self instvar routingTable_ bw_
    set accu_bw 0
    set tmp $src
    while {$tmp != $dst} {
	set next [$routingTable_ lookup $tmp $dst]
	if {$accu_bw} {
	    set accu_bw [expr 1 / (1 / $accu_bw + 1 / $bw_($tmp:$next))]
	} else {
	    set accu_bw $bw_($tmp:$next)
	}
	set tmp $next
    }
    return $accu_bw
}

### Create sessoin links and nodes
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
    $self instvar sessionNode_
    set node [new SessionNode $args]
    set sessionNode_([$node id]) $node
    return $node
}

SessionSim instproc simplex-link { n1 n2 bw delay type } {
    $self instvar bw_ delay_ linkAttr_
    set sid [$n1 id]
    set did [$n2 id]

    set bw_($sid:$did) [$self bw_parse $bw]
    set delay_($sid:$did) [$self delay_parse $delay]

	set linkAttr_($sid:$did:ORIENT) ""
	set linkAttr_($sid:$did:COLOR) "black"
}

SessionSim instproc duplex-link { n1 n2 bw delay type } {
    $self simplex-link $n1 $n2 $bw $delay $type
    $self simplex-link $n2 $n1 $bw $delay $type

    $self session-register-nam-linkconfig [$n1 id]:[$n2 id]
}

SessionSim instproc simplex-link-of-interfaces { n1 n2 bw delay type } {
    $self simplex-link $n1 $n2 $bw $delay $type
}

SessionSim instproc duplex-link-of-interfaces { n1 n2 bw delay type } {
    $self simplex-link $n1 $n2 $bw $delay $type
    $self simplex-link $n2 $n1 $bw $delay $type

    $self session-register-nam-linkconfig [$n1 id]:[$n2 id]
}

### mix mode detailed link
SessionSim instproc detailed-node { id address } {
    $self instvar Node_

    if { [Simulator info vars EnableMcast_] != "" } {
	warn "Flag variable Simulator::EnableMcast_ discontinued.\n\t\
		Use multicast methods as:\n\t\t\
		% set ns \[new Simulator -multicast on]\n\t\t\
		% \$ns multicast"
	$self multicast
	Simulator unset EnableMcast_
    }
    set node [new [Simulator set node_factory_] $address]
    Node set nn_ [expr [Node set nn_] - 1]
    $node set id_ $id
    set Node_($id) $node

    if [$self multicast?] {
	$node enable-mcast $self
    }

    return $node
}

SessionSim instproc detailed-duplex-link { from to } {
    $self instvar bw_ delay_

    SessionSim set MixMode_ 1
    set fromNode [$self detailed-node [$from id] [$from set address_]]
    set toNode [$self detailed-node [$to id] [$from set address_]]

    $self simulator-duplex-link $fromNode $toNode $bw_([$from id]:[$to id]) $delay_([$from id]:[$to id]) DropTail
}

SessionSim instproc simulator-duplex-link { n1 n2 bw delay type args } {
	$self instvar link_
	set i1 [$n1 id]
	set i2 [$n2 id]
	if [info exists link_($i1:$i2)] {
		$self remove-nam-linkconfig $i1 $i2
	}

	eval $self simulator-simplex-link $n1 $n2 $bw $delay $type $args
	eval $self simulator-simplex-link $n2 $n1 $bw $delay $type $args
}

SessionSim instproc simulator-simplex-link { n1 n2 bw delay qtype args } {
	$self instvar link_ queueMap_ nullAgent_
	set sid [$n1 id]
	set did [$n2 id]
	
	if [info exists queueMap_($qtype)] {
		set qtype $queueMap_($qtype)
	}
	# construct the queue
	set qtypeOrig $qtype
	switch -exact $qtype {
		ErrorModule {
			if { [llength $args] > 0 } {
				set q [eval new $qtype $args]
			} else {
				set q [new $qtype Fid]
			}
		}
		intserv {
			set qtype [lindex $args 0]
			set q [new Queue/$qtype]
		}
		default {
			set q [new Queue/$qtype]
		}
	}

	# Now create the link
	switch -exact $qtypeOrig {
		RTM {
                        set c [lindex $args 1]
                        set link_($sid:$did) [new CBQLink       \
                                        $n1 $n2 $bw $delay $q $c]
                }
                CBQ -
                CBQ/WRR {
                        # assume we have a string of form "linktype linkarg"
                        if {[llength $args] == 0} {
                                # default classifier for cbq is just Fid type
                                set c [new Classifier/Hash/Fid 33]
                        } else {
                                set c [lindex $args 1]
                        }
                        set link_($sid:$did) [new CBQLink       \
                                        $n1 $n2 $bw $delay $q $c]
                }
                intserv {
                        #XX need to clean this up
                        set link_($sid:$did) [new IntServLink   \
                                        $n1 $n2 $bw $delay $q	\
						[concat $qtypeOrig $args]]
                }
                default {
                        set link_($sid:$did) [new SimpleLink    \
                                        $n1 $n2 $bw $delay $q]
                }
        }
	$n1 add-neighbor $n2
	
	#XXX yuck
	if {[string first "RED" $qtype] != -1} {
		$q link [$link_($sid:$did) set link_]
	}
	
	set trace [$self get-ns-traceall]
	if {$trace != ""} {
		$self trace-queue $n1 $n2 $trace
	}
	set trace [$self get-nam-traceall]
	if {$trace != ""} {
		$self namtrace-queue $n1 $n2 $trace
	}
	
	# Register this simplex link in nam link list. Treat it as 
	# a duplex link in nam
	$self register-nam-linkconfig $link_($sid:$did)
}

# Assume ops to be performed is 'orient' only
# XXX Poor hack. What should we do without a link object??
SessionSim instproc duplex-link-op { n1 n2 op args } {
	$self instvar linkAttr_ bw_

	set sid [$n1 id]
	set did [$n2 id]

	if ![info exists bw_($sid:$did)] {
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

# nam support for session sim, Contributed by Haobo Yu
# Because here we don't have a link object, we need to have a new 
# link register method
SessionSim instproc session-register-nam-linkconfig link {
	$self instvar sessionLinkConfigList_ bw_ linkAttr_
	if [info exists sessionLinkConfigList_] {
		# Check whether the reverse simplex link is registered,
		# if so, don't register this link again.
		# We should have a separate object for duplex link.
		set tmp [split $link :]
		set i1 [lindex $tmp 0]
		set i2 [lindex $tmp 1]
		if [info exists bw_($i2:$i1)] {
			set pos [lsearch $sessionLinkConfigList_ $i2:$i1]
			if {$pos >= 0} {
				set a1 $linkAttr_($i2:$i1:ORIENT)
				set a2 $linkAttr_($link:ORIENT)
				if {$a1 == "" && $a2 != ""} {
					# If this duplex link has not been 
					# assigned an orientation, do it.
					set sessionLinkConfigList_ [lreplace $sessionLinkConfigList_ $pos $pos]
				} else {
					return
				}
			}
		}

		# Remove $link from list if it's already there
		set pos [lsearch $sessionLinkConfigList_ $link]
		if {$pos >= 0} {
			set sessionLinkConfigList_ \
				[lreplace $sessionLinkConfigList_ $pos $pos]
		}
	}
	lappend sessionLinkConfigList_ $link
}

# write link configurations
SessionSim instproc dump-namlinks {} {
    $self instvar bw_ delay_ sessionLinkConfigList_ linkAttr_

    set ns [Simulator instance]
    foreach lnk $sessionLinkConfigList_ {
	set tmp [split $lnk :]
	set i1 [lindex $tmp 0]
	set i2 [lindex $tmp 1]
	$ns puts-nam-traceall \
		"l -t * -s $i1 -d $i2 -S UP -r $bw_($lnk) -D \
		$delay_($lnk) -o $linkAttr_($lnk:ORIENT)"
    }
}

### Routing support
SessionSim instproc compute-routes {} {
    #
    # call hierarchical routing, if applicable
    #
    if [Simulator set EnableHierRt_] {
	$self compute-hier-routes 
    } else {
	$self compute-flat-routes
    }
}

SessionSim instproc compute-flat-routes {} {
    $self instvar bw_
	#
	# Compute all the routes using the route-logic helper object.
	#
        set r [$self get-routelogic]
	foreach ln [array names bw_] {
		set L [split $ln :]
		set srcID [lindex $L 0]
		set dstID [lindex $L 1]
	        if {$bw_($ln) != 0} {
			$r insert $srcID $dstID
		} else {
			$r reset $srcID $dstID
		}
	}
	$r compute
}

SessionSim instproc compute-hier-routes {} {
        $self instvar bw_
        set r [$self get-routelogic]
        #
        # send hierarchical data :
        # array of cluster size, #clusters, #domains
        # assuming 3 levels of hierarchy --> this should be extended to
	# support
        # n-levels of hierarchy
        #
        # puts "Computing Hierarchical routes\n"
        set level [AddrParams set hlevel_]
        $r hlevel-is $level
        $self hier-topo $r

        foreach ln [array names bw_] {
                set L [split $ln :]
                set srcID [[$self get-node-by-id [lindex $L 0]] node-addr]
                set dstID [[$self get-node-by-id [lindex $L 1]] node-addr]
                if { $bw_($ln) != 0 } {
#                        $r hier-insert $srcID $dstID $bw_($ln)
                        $r hier-insert $srcID $dstID
                } else {
                        $r hier-reset $srcID $dstID
                }
        }       
        $r hier-compute
}

SessionSim instproc compute-algo-routes {} {
    set r [$self get-routelogic]
    
    # puts "Computing algorithmic routes"

    $r BFS
    $r compute
}

### Route length analysis helper function
SessionSim instproc dump-routelogic-distance {} {
	$self instvar routingTable_ sessionNode_ bw_
	if ![info exists routingTable_] {
	    puts "error: routing table is not computed yet!"
	    return 0
	}

	# puts "Dumping Routing Table: Distance Information"
	set n [Node set nn_]
	set i 0
	puts -nonewline "\t"
	while { $i < $n } {
	    if ![info exists sessionNode_($i)] {
		incr i
		continue
	    }
	    puts -nonewline "$i\t"
	    incr i
	}

	set i 0
	while { $i < $n } {
		if ![info exists sessionNode_($i)] {
		    incr i
		    continue
		}
		puts -nonewline "\n$i\t"
		set n1 $sessionNode_($i)
		set j 0
		while { $j < $n } {
			if { $i != $j } {
				set nh [$routingTable_ lookup $i $j]
				if { $nh >= 0 } {
				    set distance 0
				    set tmpfrom $i
				    set tmpto $j
				    while {$tmpfrom != $tmpto} {
					set tmpnext [$routingTable_ lookup $tmpfrom $tmpto]
					set distance [expr $distance + 1]
					set tmpfrom $tmpnext
				    }
				    puts -nonewline "$distance\t"
				} else {
				    puts -nonewline "0\t"
				}
			} else {
			    puts -nonewline "0\t"
			}
			incr j
		}
		incr i
	}
	puts ""
}

### SessionSim instproc run
SessionSim instproc run args {
        $self rtmodel-configure                 ;# in case there are any
        [$self get-routelogic] configure
	$self instvar scheduler_ sessionNode_ started_

	set started_ 1

	#
	# Reset every node, which resets every agent
	#
	foreach nn [array names sessionNode_] {
		$sessionNode_($nn) reset
	}

	if [SessionSim set MixMode_] {
	    foreach nn [array names Node_] {
		$Node_($nn) reset
	    }
	}

	# We don't have queues in SessionSim
	$self dump-namcolors
	$self dump-namnodes
	$self dump-namlinks
	$self dump-namagents

        return [$scheduler_ run]
}

# Debugging mcast tree function; Contributed by Haobo Yu
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
	$self instvar routingTable_ treeLinks_ bw_

	# get paths from mbr to src and merge into treeLinks_
	set tmp $mbr
	while {$tmp != $src} {
		set nxt [$routingTable_ lookup $tmp $src]
		# XXX 
		# Assume routingTable lookup is always successful, so 
		#   don't validate existence of bw_($tid:$sid)
		# Always arrange tree links in (parent, child).
		if ![info exists treeLinks_($nxt:$tmp)] {
			set treeLinks_($nxt:$tmp) $bw_($nxt:$tmp)
		}
		if [info exists treeLinks_($tmp:$nxt)] {
			error "Reverse links in a SPT!"
		}
		set tmp $nxt
	}
}

SessionSim instproc get-node-by-id id {
	$self instvar sessionNode_
	set sessionNode_($id)
}

############## SessionNode ##############
Class SessionNode -superclass Node
SessionNode instproc init args {
    $self instvar id_ np_ address_
    set args [lreplace $args 0 1]
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

    set mask [AddrParams set PortMask_]
    set shift [AddrParams set PortShift_]
    if [Simulator set EnableHierRt_] {
	set nodeaddr [AddrParams set-hieraddr $address_]
    } else {
	set nodeaddr [expr [expr $address_ & [AddrParams set NodeMask_(1)]] \
			  << [AddrParams set NodeShift_(1)]]
    }

    $agent set addr_ [expr [expr [expr $port & $mask] << $shift] | \
		      [expr [expr ~[expr $mask << $shift]] & $nodeaddr]]
    # $agent set addr_ [expr $id_ << 8 | $port]
}

SessionNode instproc join-group { rcvAgent group } {
    set group [expr $group]
    if [SessionSim set MixMode_] {
	[Simulator instance] join-intermediate-session $rcvAgent $group
    } else {
	[Simulator instance] join-group $rcvAgent $group
    }
}

SessionNode instproc leave-group { rcvAgent group } {
    set group [expr $group]
    [Simulator instance] leave-group $rcvAgent $group
}


Agent/LossMonitor instproc show-delay { seqno delay } {
    $self instvar node_

    puts "[$node_ id] $seqno $delay"
}

####################### Mix Mode Stuff ##################################
### Create a session helper that does not associates with a src agent ###
### I.e., Create an intermediate session for mix mode operation       ###
### Return the obj to perform detailed join                           ###

SessionSim instproc create-intermediate-session { src group thisNode } {
    $self instvar session_

    set nid [$thisNode id]
    set session_($src:$group:$nid) [new SessionHelper]
    $session_($src:$group:$nid) set-node $nid
	
    if [SessionSim set rc_] {
	$session_($src:$group:$nid) set rc_ 1
    }

    # If exists nam-traceall, we'll insert an intermediate trace object
    set trace [$self get-nam-traceall]
    if {$trace != ""} {
	# This will write every packet sent and received to 
	# the nam trace file
	set p [$self create-trace SessEnque $trace $nid $dst "nam"]
	$p target $session_($src:$group:$nid)
	return $p
    } else {
	return $session_($src:$group:$nid)
    }

}

SessionSim instproc join-intermediate-session { rcvAgent group } {
    $self instvar session_ routingTable_ delay_ bw_

    foreach index [array names session_] {
	set pair [split $index :]
	set src [lindex $pair 0]
	set grp [lindex $pair 1]
	set owner [lindex $pair 2]
	if {$grp == $group && $src == $owner} {
	    set dst [[$rcvAgent set node_] id]
	    set delay 0
	    set accu_bw 0
	    set ttl 0
	    set tmp $dst
	    while {$tmp != $src} {
		set next [$routingTable_ lookup $tmp $src]
		set delay [expr $delay + $delay_($tmp:$next)]
		if {$accu_bw} {
		    set accu_bw [expr 1 / (1 / $accu_bw + 1 / $bw_($tmp:$next))]
		} else {
		    set accu_bw $bw_($tmp:$next)
		}
		incr ttl

		# Conditions to perform session/detailed join
		
		
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
		$p target $rcvAgent
		$session_($index) add-dst $accu_bw $delay $ttl $dst $p
		$self update-loss-dependency $src $dst $p $group
	    } else {
		#puts "add-dst $accu_bw $delay $ttl $src $dst"
		$session_($index) add-dst $accu_bw $delay $ttl $dst $rcvAgent
		$self update-loss-dependency $src $dst $rcvAgent $group
	    }
	}
    }
}

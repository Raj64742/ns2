proc abort msg {
    puts stderr "ns: $msg"
    exit 1
}

Simulator instproc rtproto {proto args} {
    eval [$self get-routelogic] register $proto $args
}

Simulator instproc get-routelogic {} {
    $self instvar routingTable_
    if ![info exists routingTable_] {
	set routingTable_ [new RouteLogic]
    }
    return $routingTable_
}


RouteLogic instproc register {proto args} {
    $self instvar rtprotos_
    eval lappend rtprotos_($proto) $args
}

RouteLogic proc configure args {
    set ns [Simulator instance]
    eval [$ns get-routelogic] config-protos $args
}

RouteLogic instproc config-protos args {
    $self instvar rtprotos_
    if [info exists rtprotos_] {
	if { [llength $args] != 0 } {
	    puts -nonewline stderr {Routing protocol overspecification.  }
	    puts stderr {Arguments to `$Simulator run' ignored.}
	}
	foreach proto [array names rtprotos_] {
	    eval rtProto/$proto init-all $rtprotos_($proto)
	}
    } else {
	if {[llength $args] == 0} {
	    rtProto/Static init-all
	} else {
	    switch [lindex $args 0] {
		Static {
		    rtProto/Static init-all
		    set rtprotos_(Static) 1
		}
		Dynamic {
		    if { [llength $args] == 1 } {
			rtProto/DV init-all
			set rtprotos_(DV) 1
		    } else {
			set proto [lindex $args 1]
			set pargs [lrange $args 2 end]
			rtProto/$proto $pargs
			set rtprotos_($proto) 1
		    }
		}
		Session {
		    rtProto/Session init-all
		    set rtprotos_(Session) 1
		}
		default {
		    abort "unknown rtProto argument: [lindex $args 0]"
		}
	    }
	}
    }
}

RouteLogic instproc lookup { nodeid destid } {
    set ns [Simulator instance]
    set node [$ns get-node-by-id $nodeid]
    if { [$node info vars rtObject_] != "" } {
	set dest [$ns get-node-by-id $destid]
	[$node set rtObject_] lookup $dest
    } else {
	$self cmd lookup $nodeid $destid
    }
}

RouteLogic instproc notify {} {
    $self instvar rtprotos_
    foreach i [array names rtprotos_] {
	rtProto/$i compute-all
    }
}


Node instproc rtObject obj {
    $self instvar rtObject_
    $self set rtObject_ $obj
}

Node instproc rtObject? {} {
    $self instvar rtObject_
    $self set rtObject_
}


#
Class rtObject

rtObject instproc init node {
    $self next

    $node rtObject $self
    $self add-proto Direct $node
    $self intf-changed
}

rtObject proc init-all args {
    foreach node $args {
	if { [$node info vars rtObject_] == "" } {
	    $node rtObject [new rtObject $node]
	}
    }
}

rtObject instproc add-proto proto {
    $self instvar rtProtos_
    set rtProtos_($proto) [new rtProto/$proto $node]
    
    set ns [Simulator instance]
    $ns attach-agent $node $rtProtos_($proto)
}

rtObject instproc rtProto? proto {
    $self instvar rtProtos_
    if [info exists rtProtos_($proto)] {
	return $rtProtos_($proto)
    } else {
	return ""
    }
}

rtObject instproc compute-routes {} {
    $self instvar rtProtos_
    foreach p $rtProtos_ {
	$p compute-routes
    }
}

rtObject instproc new-routes-computed? {} {
    $self instvar rtProtos_ nextHop_ rtpref_ metric_

    set changes 0
    foreach p $rtProtos_ {
	if [$p set rtsChanged_] {
	    set changes [expr changes + [assess-changes $p]]
	    $p set rtsChanged_ 0
	}
    }
    return changes
}

rtObject instproc assess-changes p {
    set changes 0
    foreach dest [Simulator all-nodes-list] {
	set pnh [$p set nextHop_($dest)]
	set ppf [$p set rtpref_($dest)]
	set pmt [$p set metric_($dest)]
	if { $pnh != "" && $pnh != $nextHop_($dest) &&
	     { $ppf < $rtpref_($dest) ||
	       { $ppf == $rtpref_($dest) && $pmt < $metric_($dest) }}} {
	    set nextHop_($dest) $pnh
	    set rtpref_($dest)  $ppf
	    set metric_($dest)  $pmt
	    incr changes
	}
    }
    return changes
}

rtObject instproc install-routes {} {
    $self instvars node_ nextHop_
    foreach dest [Simulator all-nodes-list] {
	$node_ add-route $dest [$nextHop_($dest) head]
    }
}

rtObject instproc lookup dest {
    return $nextHop_($dest)
}

rtObject instproc intf-changed {} {
    $self compute-routes
    $self install-if-new-routes
}

rtObject instproc install-if-new-routes {} {
    $self instvar rtProtos_

    if [$self new-routes-computed?] {
	install-routes
        foreach proto $rtProtos_ {
	    $proto send-updates
	}
    }
}

#
Class rtProto -superclass Agent
rtProto set preference_ 100		;# global default preference

rtProto proc init-all args {
    abort "No initialisation defined"
}

rtProto instproc init node {
    $self next

    $self instvar preference_ node_ ifs_ ifstat_
    catch "set preference_ [[$self info class] set preference_]" ret
    if { $ret == "" } {
	set preference_ [$class set preference_]
    }
    foreach nbr [$node set neighbor_] {
	set link [Simulator link $node $nbr]
	set ifs_($nbr) $link
	set ifstat_($nbr) [$link status?]
    }
}

rtProto instproc compute-routes {} {
    abort "No route computation defined"
}

rtProto instproc send-updates {} {
    #NOTHING
}

rtProto proc compute-all {} {
    #NOTHING
}

Class rtProto/Direct -superclass rtProto

rtProto/Direct set preference_ 100

rtProto/Direct instproc init node {
    $self next $node
    $self instvar preference_ rtpref_ nextHop_ metric_

    foreach node [Simulator all-nodes-list] {
	set rtpref_($node) 255
	set nextHop_($node) ""
	set metric_($node) -1
    }
    foreach node [array names ifs_] {
	set rtpref_($node) $preference_
    }
}

rtProto/Direct instproc compute-routes {} {
    $self instvar ifs_ ifstat_ nextHop_ rtsChanged_
    set rtsChanged_ 0
    foreach nbr [array names ifs_] {
	if {$nextHop_($nbr) == "" && [$ifs_($nbr) status?] == "up"} {
	    set ifstat_($nbr) 1
	    set nextHop_($nbr) $ifs_($nbr)
	    set metric_($nbr) 1
	    incr rtsChanged_
	} elsif {$nextHop_($nbr) != "" && [$ifs_($nbr) status?] != "up"} {
	    set ifstat_($nbr) 0
	    set nextHop_($nbr) ""
	    set metric_($nbr) -1
	    incr rtsChanged_
	}
    }
}

#
# Static routing, the default
#
Class rtProto/Static -superclass rtProto

rtProto/Static proc init-all args {
    # The Simulator knows the entire topology.
    # Hence, the current compute-routes method in the Simulator class is
    # well suited.  We use it as is.
    [Simulator instance] compute-routes
}

#
# Session based unicast routing
#
Class rtProto/Session -superclass rtProto

rtProto/Session proc init-all args {
    [Simulator instance] compute-routes
}

rtProto/Session proc compute-all {} {
    [Simulator instance] compute-routes
}

#########################################################################
#
# Code below this line is experimental, and should be considered work
# in progress.  None of this code is used in production test-suites, or
# in the release yet, and hence should not be a problem to anyone.
#

Class rtPeer

rtPeer instproc init {addr class} {
    $self next
    $self instproc addr_ metric_ rtpref_
    set addr_ $addr
    foreach dest [Simulator all-nodes-list] {
	set metric_($dest) [$class set $INFINITY]
	set rtpref_($dest) [$class set $preference_]
    }
}
rtPeer instproc metric {dest val} {
    $self instproc metric_
    set metric_($dest) $val
}

rtPeer instproc metric? dest {
    $self instproc metric_
    return $metric_($dest)
}

rtPeer instproc preference {dest val} {
    $self instproc rtpref_
    set rtpref_($dest) $val
}

rtPeer instproc preference? dest {
    $self instproc rtpref_
    return $rtpref_($dest)
}

#
# Distance Vector Route Computation
#

Class rtProto/DV -superclass rtProto

rtProto/DV set preference_ 120
rtProto/DV set INFINITY	    32

rtProto/DV proc init-all args {
    if { [lindex args] == 0 } {
	set nodeslist [Simulator all-nodes-list]
    } else {
	eval "set nodeslist $args"
    }
    eval rtObject init-all $nodeslist
    foreach node $nodeslist {
	[$node rtObject?] add-proto DV
    }
    foreach $node $nodeslist {
	set srtproto [[$node rtObject?] rtProto? DV]
	set saddr [$srtproto set addr_]
	foreach nbr [$node neighbors] {
	    set nrtObject [$nbr rtObject?]
	    if { $nrtObject != "" } {
		set nrtproto [$nrtObject rtProto? DV]
		if { $nrtproto != "" } {
		    $srtproto add-neighbor $nbr [$nrtproto set addr_]
		}
	    }
	}
    }
    $self send-updates
}

rtProto/DV instproc init node {
    $self next $node
    $self instvar preference_ rtpref_ nextHop_ metric_ node_

    foreach dest [Simulator all-nodes-list] {
	set rtpref_($dest) $preference_
	set nextHop_($dest) ""
	set metric_($dest) -1
    }
}

rtProto/DV instproc add-neighbor {nbr agent-addr} {
    $self instvar neighbors_
    $self set neighbors_($nbr) [new rtPeer $agent-addr]
}

rtProto/DV instproc compute-routes {} {
    $self instvar neighbors_ rtsChanged_

    set rtsChanged_ 0
    foreach nbr [array names neighbors_] {
	foreach dest [Simulator all-nodes-list] {
	    set ppf [$neighbors_($nbr) preference?]
	    set pmt [$neighbors_($nbr) metric?]
	    if { $pmt < [$class set INFINITY] &&
	        { $ppf < $rtpref_($dest) ||
	    { $ppf == $rtpref_($dest) && $pmt < $metric_($dest) }}} {
		set rtpref_($dest) $ppf
		set metric_($dest) $pmt
		set nextHop_($dest) $nbr
		incr rtsChanged_
	    }
	}
    }
    return $rtsChanged_
}

rtProto/DV instproc recv-updates {peer args} {
    set metricsChanged 0
    set nn [Node set nn_]
    for {set i 0} {i < $nn} {incr i} {
	set metric [expr [lindex $args $i] + [$ifs_($peer) cost?]]
	if { $metric > [$class set INFINITY] } {
	    set metric [$class set INFINITY]
	}	    
	if {[$rtPeer($peer) metric?] != $metric} {
	    $rtPeer($peer) metric $metric
	    incr metricsChanged
	}
    }
    if $metricsChanged {
	$self compute-routes
	$self instvar rtsChanged_ node_
	if $rtsChanged_ {
	    [$node_ rtObject?] install-if-new-routes
	}
    }
}

rtProto/DV instproc send-updates {} {
    $self instvar neighbors_ ifs_ rtObject_
    foreach peer [array names neighbors_] {
	if { $ifs_($peer) up? == "up" } {
	    set update ""
	    set nn [Node set nn_]
	    for {set i 0} {i < $nn} {incr i} {
		set dest [get-node-by-id $i]
		set nhop [$rtObject_ nextHop? $dest]
		if {$nhop != "" && $nhop != $peer} {
		    lappend update [$rtObject_ metric? $dest]
		} else {
		    lappend update [$class set $INFINITY]
		}
	    }
	    eval $self send-to-peer $nn $update
	}
    }
}

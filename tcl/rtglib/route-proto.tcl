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
    set ns [Simulator info instances]
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
    set ns [Simulator info instances]
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
    
    set ns [Simulator info instances]
    $ns attach-agent $node $rtProtos_($proto)
}

rtObject instproc rtProto? proto {
    $self instvar rtProtos_
    return $rtProtos_($proto)
}

rtObject instproc compute-routes {} {
    $self instvar rtProtos_
    foreach p $rtProtos_ {
	$p compute-routes
    }
}

rtObject instproc recompute-routes {} {
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
    foreach node [Simulator all-nodes-list] {
	if { [$p info vars nextHop_($node)] && [info exists nextHop_($node)] } {
	    set pnh [$p set nextHop_($node)]
	    set ppf [$p set rtpref_($node)]
	    set pmt [$p set metric_($node)]
	    if { $pnh != $nextHop_($node) &&
	         { $ppf < $rtpref_($node) ||
		   { $ppf == $rtpref_($node) && $pmt < $metric_($node) }}} {
		set nextHop_($node) $pnh
		set rtpref_($node)  $ppf
		set metric_($node)  $pmt
		incr changes
	    }
	}
    }
    return changes
}

rtObject instproc install-routes {} {
    $self instvars node_ nextHop_
    foreach node [Simulator all-nodes-list] {
	$node_ add-route $node [$nextHop_($node) head]
    }
}

rtObject instproc lookup dest {
    return $nextHop_($dest)
}

rtObject instproc intf-changed {} {
    $self instvar rtProtos_

    $self compute-routes
    if [$self recompute-routes] {
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
	if {$nextHop_($nbr) == "" && [$ifs_($nbr) status?]} {
	    set ifstat_($nbr) 1
	    set nextHop_($nbr) $ifs_($nbr)
	    set metric_($nbr) 1
	    incr rtsChanged_
	} elsif {$nextHop_($nbr) != "" && ![$ifs_($nbr) status?]} {
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
    [Simulator info instances] compute-routes
}

#
# Session based unicast routing
#
Class rtProto/Session -superclass rtProto

rtProto/Session proc init-all args {
    [Simulator info instances] compute-routes
}

rtProto/Session proc compute-all {} {
    [Simulator info instances] compute-routes
}

#
# Distance Vector Route Computation
#

Class rtProto/DV -superclass rtProto

rtProto/DV set preference_ 120

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
	set saddr [$srtproto addr?]
	foreach nbr [$node neighbors] {
	    set nrtproto [[$nbr rtObject?] rtProto? DV]
	    if { $nrtproto != "" } {
		$srtproto add-neighbor [$nrtproto addr?]
	    }
	}
    }
}

rtProto/DV instproc init node {
    $self next $node
    $self instvar preference_ rtpref_ nextHop_ metric_

    foreach node [Simulator all-nodes-list] {
	set rtpref_($node) $preference_
	set nextHop_($node) ""
	set metric_($node) -1
    }
}

rtProto/DV instproc add-neighbor nagent {
}

rtProto/DV instproc compute-routes {} {
}

rtProto/DV instproc send-updates {} {
}

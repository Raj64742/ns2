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

SimpleLink instproc cost c {
    $self instvar cost_
    set cost_ $c
}

SimpleLink instproc cost? {} {
    $self instvar cost_
    if ![info exists cost_] {
	set cost_ 1
    }
    set cost_
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
	    eval Agent/rtProto/$proto init-all $rtprotos_($proto)
	}
    } else {
	if {[llength $args] == 0} {
	    Agent/rtProto/Static init-all
	} else {
	    switch [lindex $args 0] {
		Static {
		    Agent/rtProto/Static init-all
		    set rtprotos_(Static) 1
		}
		Dynamic {
		    if { [llength $args] == 1 } {
			Agent/rtProto/DV init-all
			set rtprotos_(DV) 1
		    } else {
			set proto [lindex $args 1]
			set pargs [lrange $args 2 end]
			Agent/rtProto/$proto $pargs
			set rtprotos_($proto) 1
		    }
		}
		Session {
		    Agent/rtProto/Session init-all
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
    set node [Simulator get-node-by-id $nodeid]
    if { [$node rtObject?] != "" } {
	set dest [Simulator get-node-by-id $destid]
	[$node set rtObject_] lookup $dest
    } else {
	$self cmd lookup $nodeid $destid
    }
}

RouteLogic instproc notify {} {
    $self instvar rtprotos_
    foreach i [array names rtprotos_] {
	Agent/rtProto/$i compute-all
    }
}


Node instproc rtObject obj {
    $self instvar rtObject_
    if ![info exists rtObject_] {
	$self set rtObject_ $obj
    }
    $self set rtObject_
}

Node instproc rtObject? {} {
    $self instvar rtObject_
    if ![info exists rtObject_] {
	return ""
    } else {
	return $rtObject_
    }
}


#
Class rtObject

rtObject set maxpref_   255
rtObject set unreach_	 -1

rtObject proc init-all args {
    foreach node $args {
	if { [$node rtObject?] == "" } {
	    new rtObject $node
	}
    }
}

rtObject instproc init node {
    $self next
    $self instvar nextHop_ rtpref_ metric_ node_ rtVia_ rtProtos_

    $node rtObject $self
    set node_ $node
    foreach dest [Simulator all-nodes-list] {
	set nextHop_($dest) ""
	set rtpref_($dest) [$class set maxpref_]
	set metric_($dest) [$class set unreach_]
	set rtVia_($dest)    ""
    }
    set nextHop_($node) ""
    set rtpref_($node) 0
    set metric_($node) 0
    set rtVia_($dest) "Agent/rtProto/Local"	;# to make dump happy
    $self add-proto Direct $node
    $self compute-proto-routes
    $self compute-routes
}

rtObject instproc add-proto {proto node} {
    $self instvar rtProtos_
    set rtProtos_($proto) [new Agent/rtProto/$proto $node]
    
    set ns [Simulator instance]
    $ns attach-agent $node $rtProtos_($proto)
    set rtProtos_($proto)
}

rtObject instproc lookup dest {
    $self instvar nextHop_
    if ![info exists nextHop_($dest)] {
	return ""
    } else {
        return $nextHop_($dest)
    }
}

rtObject instproc compute-proto-routes {} {
    # Each protocol computes its best routes
    $self instvar rtProtos_
    foreach p [array names rtProtos_] {
	$rtProtos_($p) compute-routes
    }
}

rtObject instproc compute-routes { } {
    # rtObject chooses the best route to each destination from all protocols
    $self instvar rtProtos_ nextHop_ rtpref_ metric_ rtVia_

    set protos ""
    foreach p [array names rtProtos_] {
	if [$rtProtos_($p) set rtsChanged_] {
	    lappend protos $rtProtos_($p)
	    $rtProtos_($p) set rtsChanged_ 0
	}
    }
    if {[llength $protos] == 0} {
	return
    }

    set changes 0
    set all-nodes [Simulator all-nodes-list]
    foreach dest ${all-nodes} {
	if {$nextHop_($dest) != ""} {
	    if {$nextHop_($dest) != [$rtVia_($dest) set nextHop_($dest)]} {
		incr changes
	    }
	    set nextHop_($dest) [$rtVia_($dest) set nextHop_($dest)]
	    set metric_($dest) [$rtVia_($dest) set metric_($dest)]
	    set rtpref_($dest) [$rtVia_($dest) set rtpref_($dest)]
	}
	foreach p $protos {
	    set pnh [$p set nextHop_($dest)]
	    set ppf [$p set rtpref_($dest)]
	    set pmt [$p set metric_($dest)]
	    if { $pnh != "" && ($ppf < $rtpref_($dest) ||		   \
		    ($ppf == $rtpref_($dest) && $pmt < $metric_($dest)) || \
		    $metric_($dest) < 0) } {	    
		set nextHop_($dest) $pnh
		set rtpref_($dest)  $ppf
		set metric_($dest)  $pmt
		set rtVia_($dest)   $p
		incr changes
	    }
	}
    }

    # If new routes were found, install the routes, and send updates
    if {$changes > 0} {
	$self instvar node_ nextHop_
	foreach dest ${all-nodes} {
	    if { $nextHop_($dest) != "" } {
		$node_ add-route [$dest id] [$nextHop_($dest) head]
	    }
	}

	foreach proto [array names rtProtos_] {
	    $rtProtos_($proto) send-updates
	}
    }
    
    $self instvar node_
}

rtObject instproc intf-changed {} {
    $self instvar rtProtos_ rtVia_ nextHop_
    foreach p [array names rtProtos_] {
	$rtProtos_($p) intf-changed
    }
    foreach dest [Simulator all-nodes-list] {
	if { $nextHop_($dest) != "" &&				\
		[$rtVia_($dest) set nextHop_($dest)] != $nextHop_($dest) } {
	    set nextHop_($dest) ""
	    set rtpref_($dest) [$class set maxpref_]
	    set metric_($dest) [$class set unreach_]
	}
    }
    $self compute-proto-routes
    $self compute-routes
}

rtObject instproc dump-routes chan {
    $self instvar node_ nextHop_ rtpref_ metric_ rtVia_

    set ns [Simulator instance]
    if {$ns != ""} {
	set time [$ns now]
    } else {
	set time 0.0
    }
    puts $chan [concat "Node:\t${node_}([$node_ id])\tat t ="		\
	    [format "%4.2f" $time]]
    puts $chan "Dest\tnextHop\tPref\tMetric\tProto"
    foreach dest [Simulator all-nodes-list] {
	if {$nextHop_($dest) != ""} {
	    set p [split [$rtVia_($dest) info class] /]
	    set proto [lindex $p [expr [llength $p] - 1]]
	    puts $chan [concat "${dest}([$dest id])\t"			      \
		    "$nextHop_($dest)([[$nextHop_($dest) set toNode_] id])\t" \
		    "$rtpref_($dest)\t$metric_($dest)\t$proto"]
	}
    }
}

rtObject instproc rtProto? proto {
    $self instvar rtProtos_
    if [info exists rtProtos_($proto)] {
	return $rtProtos_($proto)
    } else {
	return ""
    }
}

rtObject instproc nextHop? dest {
    $self instvar nextHop_
    $self set nextHop_($dest)
}

rtObject instproc rtpref? dest {
    $self instvar rtpref_
    $self set rtpref_($dest)
}

rtObject instproc metric? dest {
    $self instvar metric_
    $self set metric_($dest)
}

#
Class rtPeer

rtPeer instproc init {addr cls} {
    $self next
    $self instvar addr_ metric_ rtpref_
    set addr_ $addr
    foreach dest [Simulator all-nodes-list] {
	set metric_($dest) [$cls set INFINITY]
	set rtpref_($dest) [$cls set preference_]
    }
}

rtPeer instproc addr? {} {
    $self instvar addr_
    return $addr_
}

rtPeer instproc metric {dest val} {
    $self instvar metric_
    set metric_($dest) $val
}

rtPeer instproc metric? dest {
    $self instvar metric_
    return $metric_($dest)
}

rtPeer instproc preference {dest val} {
    $self instvar rtpref_
    set rtpref_($dest) $val
}

rtPeer instproc preference? dest {
    $self instvar rtpref_
    return $rtpref_($dest)
}

#
#Class Agent/rtProto -superclass Agent

Agent/rtProto set preference_ 200		;# global default preference

Agent/rtProto proc init-all args {
    abort "No initialisation defined"
}

Agent/rtProto instproc init node {
    $self next

    $self instvar preference_ node_ ifs_ ifstat_ rtObject_
    catch "set preference_ [[$self info class] set preference_]" ret
    if { $ret == "" } {
	set preference_ [$class set preference_]
    }
    foreach nbr [$node set neighbor_] {
	set link [Simulator link $node $nbr]
	set ifs_($nbr) $link
	set ifstat_($nbr) [$link up?]
    }
    set rtObject_ [$node rtObject?]
}

Agent/rtProto instproc compute-routes {} {
    abort "No route computation defined"
}

Agent/rtProto instproc intf-changed {} {
    #NOTHING
}

Agent/rtProto instproc send-updates {} {
    #NOTHING
}

Agent/rtProto proc compute-all {} {
    #NOTHING
}

Class Agent/rtProto/Direct -superclass Agent/rtProto

Agent/rtProto/Direct set preference_ 100

Agent/rtProto/Direct instproc init node {
    $self next $node
    $self instvar rtpref_ nextHop_ metric_ ifs_

    foreach node [Simulator all-nodes-list] {
	set rtpref_($node) 255
	set nextHop_($node) ""
	set metric_($node) -1
    }
    foreach node [array names ifs_] {
	set rtpref_($node) [$class set preference_]
    }
}

Agent/rtProto/Direct instproc compute-routes {} {
    $self instvar ifs_ ifstat_ nextHop_ metric_ rtsChanged_
    set rtsChanged_ 0
    foreach nbr [array names ifs_] {
	if {$nextHop_($nbr) == "" && [$ifs_($nbr) up?] == "up"} {
	    set ifstat_($nbr) 1
	    set nextHop_($nbr) $ifs_($nbr)
	    set metric_($nbr) 1
	    incr rtsChanged_
	} elseif {$nextHop_($nbr) != "" && [$ifs_($nbr) up?] != "up"} {
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
Class Agent/rtProto/Static -superclass Agent/rtProto

Agent/rtProto/Static proc init-all args {
    # The Simulator knows the entire topology.
    # Hence, the current compute-routes method in the Simulator class is
    # well suited.  We use it as is.
    [Simulator instance] compute-routes
}

#
# Session based unicast routing
#
Class Agent/rtProto/Session -superclass Agent/rtProto

Agent/rtProto/Session proc init-all args {
    [Simulator instance] compute-routes
}

Agent/rtProto/Session proc compute-all {} {
    [Simulator instance] compute-routes
}

#
#########################################################################
#
# Code below this line is experimental, and should be considered work
# in progress.  None of this code is used in production test-suites, or
# in the release yet, and hence should not be a problem to anyone.
#
#
# Distance Vector Route Computation
#
# Class Agent/rtProto/DV -superclass Agent/rtProto

Agent/rtProto/DV set preference_	120
Agent/rtProto/DV set INFINITY		 32
Agent/rtProto/DV set advertInterval	  2
Agent/rtProto/DV set mid_		  0

Agent/rtProto/DV proc init-all args {
    if { [llength $args] == 0 } {
	set nodeslist [Simulator all-nodes-list]
    } else {
	eval "set nodeslist $args"
    }
    eval rtObject init-all $nodeslist
    foreach node $nodeslist {
	set proto($node) [[$node rtObject?] add-proto DV $node]
    }
    foreach node $nodeslist {
	foreach nbr [$node neighbors] {
	    set rtobj [$nbr rtObject?]
	    if { $rtobj != "" } {
		set rtproto [$rtobj rtProto? DV]
		if { $rtproto != "" } {
		    $proto($node) add-peer $nbr [$rtproto set addr_]
		}
	    }
	}
    }
}

Agent/rtProto/DV instproc init node {
    $self next $node
    $self instvar preference_ rtpref_ nextHop_ nextHopPeer_ metric_

    set INFINITY [$class set INFINITY]
    foreach dest [Simulator all-nodes-list] {
	set rtpref_($dest) $preference_
	set nextHop_($dest) ""
	set nextHopPeer_($dest) ""
	set metric_($dest)  $INFINITY
    }
    set updateTime [uniform 0.0 0.5]
    set ns [Simulator instance]
    $ns at $updateTime "$self send-periodic-update"
}

Agent/rtProto/DV instproc add-peer {nbr agentAddr} {
    $self instvar peers_
    $self set peers_($nbr) [new rtPeer $agentAddr $class]
}

Agent/rtProto/DV instproc send-periodic-update {} {
    $self send-updates
    
    set ns [Simulator instance]
    set updateTime [expr [$ns now] + \
	    ([$class set advertInterval] * [uniform 0.9 1.1])]
    $ns at $updateTime "$self send-periodic-update"
}

Agent/rtProto/DV instproc compute-routes {} {
    $self instvar ifs_ peers_ rtsChanged_ rtpref_ metric_ nextHop_ nextHopPeer_

    set INFINITY [$class set INFINITY]
    set rtsChanged_ 0
    foreach nbr [array names peers_] {
	foreach dest [Simulator all-nodes-list] {
	    set ppf [$peers_($nbr) preference? $dest]
	    set pmt [$peers_($nbr) metric? $dest]
	    if { $pmt > 0 && $pmt < $INFINITY &&			\
		    ($ppf < $rtpref_($dest) ||				\
		    ($ppf == $rtpref_($dest) && $pmt < $metric_($dest))) } {
		set rtpref_($dest) $ppf
		set metric_($dest) $pmt
		set nextHop_($dest) $ifs_($nbr)
		set nextHopPeer_($dest) $peers_($nbr)
		incr rtsChanged_
	    }
	}
    }
    set rtsChanged_
}

Agent/rtProto/DV instproc intf-changed {} {
    $self instvar peers_ ifs_ ifstat_ peers_ nextHop_ nextHopPeer_ metric_
    set INFINITY [$class set INFINITY]
    foreach nbr [array names peers_] {
	set state [$ifs_($nbr) up?]
	if {$state != $ifstat_($nbr)} {
	    switch $ifstat_($nbr) {
		up {
		    if ![info exists all-nodes] {
			set all-nodes [Simulator all-nodes-list]
		    }
		    foreach dest ${all-nodes} {
			$peers_($nbr) metric $dest $INFINITY
			if {$nextHopPeer_($dest) == $peers_($nbr)} {
			    set nextHopPeer_($dest) ""
			    set nextHop_($dest) ""
			    set metric_($dest) $INFINITY
			}
		    }
		}
		down {
		    $self send-to-peer $nbr
		}
	    }
	    set ifstat_($nbr) $state
	}
    }
}

Agent/rtProto/DV proc get-next-mid {} {
    set ret [Agent/rtProto/DV set mid_]
    Agent/rtProto/DV set mid_ [expr $ret + 1]
    set ret
}

Agent/rtProto/DV proc retrieve-msg id {
    set ret [Agent/rtProto/DV set msg_($id)]
    Agent/rtProto/DV unset msg_($id)
    set ret
}

Agent/rtProto/DV instproc send-updates {} {
    $self instvar peers_ ifs_
    foreach nbr [array names peers_] {
	if { [$ifs_($nbr) up?] == "up" } {
	    $self send-to-peer $nbr
	}
    }
}

Agent/rtProto/DV instproc send-to-peer nbr {
    $self instvar rtObject_ ifs_ peers_
    foreach dest [Simulator all-nodes-list] {
	set nhop [$rtObject_ nextHop? $dest]
	set metric [$rtObject_ metric? $dest]
	if {$metric < 0 || $nhop == $ifs_($nbr)} {
	    set update($dest) [$class set INFINITY]
	} else {
	    set update($dest) [$rtObject_ metric? $dest]
	}
    }
    set id [$class get-next-mid]
    $class set msg_($id) [array get update]
    $self send-update [$peers_($nbr) addr?] $id [array size update]
    set n [$rtObject_ set node_]
#    puts stderr [concat [format "%4.2f" [[Simulator instance] now]]	\
	    "${n}([$n id]) send update to ${nbr}([$nbr id])"		\
	    "id = $id" "{[$class set msg_($id)]}"]
}

Agent/rtProto/DV instproc recv-update {peerAddr id} {
    $self instvar peers_ ifs_
    set INFINITY [$class set INFINITY]
    set msg [$class retrieve-msg $id]
    array set metrics $msg
    $self instvar rtObject_
    set n [$rtObject_ set node_]
#    puts stderr [concat [format "%4.2f" [[Simulator instance] now]]	\
	    "${n}([$n id]) recv update from peer $peerAddr id = $id {$msg}"]
    foreach nbr [array names peers_] {
	if {[$peers_($nbr) addr?] == $peerAddr} {
	    set peer $peers_($nbr)
	    if { [array size metrics] > [Node set nn_] } {
		abort "$class::$proc update $peerAddr:$msg:$count is larger than the simulation topology"
	    }
	    set metricsChanged 0
	    foreach dest [array names metrics] {
                set metric [expr $metrics($dest) + [$ifs_($nbr) cost?]]
		if {$metric > $INFINITY} {
		    set metric $INFINITY
		}
		if {$metric != [$peer metric? $dest]} {
		    $peer metric $dest $metric
		    incr metricsChanged
		}
	    }
	    if $metricsChanged {
		$self compute-routes
		$self instvar rtsChanged_ rtObject_
		if $rtsChanged_ {
		    $rtObject_ compute-routes
		}
	    }
	    return
	}
    }
    abort "$class::$proc update $peerAddr:$msg:$count from unknown peer"
}

Agent/rtProto/DV proc compute-all {} {
    # Because proc methods are not inherited from the parent class.
}

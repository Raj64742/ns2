#
#  Copyright (c) 1997 by the University of Southern California
#  All rights reserved.
#
#  Permission to use, copy, modify, and distribute this software and its
#  documentation in source and binary forms for non-commercial purposes
#  and without fee is hereby granted, provided that the above copyright
#  notice appear in all copies and that both the copyright notice and
#  this permission notice appear in supporting documentation. and that
#  any documentation, advertising materials, and other materials related
#  to such distribution and use acknowledge that the software was
#  developed by the University of Southern California, Information
#  Sciences Institute.  The name of the University may not be used to
#  endorse or promote products derived from this software without
#  specific prior written permission.
#
#  THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
#  the suitability of this software for any purpose.  THIS SOFTWARE IS
#  PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
#  INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#  Other copyrights might apply to parts of this software and are so
#  noted when applicable.
#

#
# Maintainer: <kannan@isi.edu>.
#

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

Simulator instproc cost {n1 n2 c} {
    $self instvar link_
    $link_([$n1 id]:[$n2 id]) cost $c
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
    if [info exists rtprotos_($proto)] {
	eval lappend rtprotos_($proto) $args
    } else {
	set rtprotos_($proto) $args
    }
}

RouteLogic proc configure args {
    set ns [Simulator instance]
    eval [$ns get-routelogic] config-protos $args
}

RouteLogic instproc config-protos args {
    $self instvar rtprotos_
    if [info exists rtprotos_] {
	if { [llength $args] != 0 } {
	    puts stderr [concat {Routing protocol overspecification.  } \
		    {Arguments to `$Simulator run' ignored.}]
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
			eval Agent/rtProto/$proto init-all $pargs
			set rtprotos_($proto) 1
		    }
		}
		Session {
		    Agent/rtProto/Session init-all
		    set rtprotos_(Session) 1
		}
		default {
		    error "unknown rtProto argument: [lindex $args 0]"
		}
	    }
	}
    }
}

RouteLogic instproc lookup { nodeid destid } {
    if { $nodeid == $destid } {
	return $nodeid
    }

    set ns [Simulator instance]
    set node [$ns get-node-by-id $nodeid]
    set rtobj [$node rtObject?]
    if { $rtobj != "" } {
	$rtobj lookup [$ns get-node-by-id $destid]
    } else {
	$self cmd lookup $nodeid $destid
    }
}

RouteLogic instproc notify {} {
    $self instvar rtprotos_
    foreach i [array names rtprotos_] {
	Agent/rtProto/$i compute-all
    }
    set ns [Simulator instance]
    if {[$ns info class] == "MultiSim"} {
	foreach i [CtrMcastComp info instances] {
	    $i notify
	}
    }
}


Node set multiPath_ 0

Node instproc init-routing rtObject {
    $self instvar multiPath_ routes_ rtObject_
    set multiPath_ [$class set multiPath_]
    set nn [$class set nn_]
    for {set i 0} {$i < $nn} {incr i} {
	set routes_($i) 0
    }
    if ![info exists rtObject_] {
	$self set rtObject_ $rtObject
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

Node instproc add-routes {id ifs} {
    $self instvar classifier_ multiPath_ routes_ mpathClsfr_
    if {[llength $ifs] > 1 && ! $multiPath_} {
	puts stderr "$class::$proc cannot install multiple routes"
	set ifs [llength $ifs 0]
	set routes_($id) 0
    }
    if { $routes_($id) <= 0 && [llength $ifs] == 1 &&	\
	    ![info exists mpathClsfr_($id)] } {
	# either we really have no route, or
	# only one route that must be replaced.
	$self add-route $id [$ifs head]
	incr routes_($id)
    } else {
	if ![info exists mpathClsfr_($id)] {
	    set mclass [new Classifier/MultiPath]
	    if {$routes_($id) > 0} {
		array set current [$classifier_ adjacents]
		foreach i [array names current] {
		    if {$current($i) == $id} {
			$mclass installNext $i
			break
		    }
		}
	    }
	    $classifier_ install $id $mclass
	    set mpathClsfr_($id) $mclass
	}
	foreach L $ifs {
	    $mpathClsfr_($id) installNext [$L head]
	    incr routes_($id)
	}
    }
}

Node instproc delete-routes {id ifs nullagent} {
    $self instvar mpathClsfr_ routes_
    if [info exists mpathClsfr_($id)] {
	array set eqPeers [$mpathClsfr_($id) adjacents]
	foreach L $ifs {
	    set link [$L head]
	    if [info exists eqPeers($link)] {
		$mpathClsfr_($id) clear $eqPeers($link)
		unset eqPeers($link)
		incr routes_($id) -1
	    }
	}
    } else {
	$self add-route $id $nullagent
	incr routes_($id) -1
    }
}

Classifier instproc install {slot val} {
    $self instvar elements_
    set elements_($val) $slot
    $self cmd install $slot $val
}

Classifier instproc installNext val {
    $self instvar elements_
    set elements_($val) [$self cmd installNext $val]
    return $elements_($val)
}

Classifier instproc adjacents {} {
    $self instvar elements_
    return [array get elements_]
}

#
Class rtObject

rtObject set maxpref_   255
rtObject set unreach_	 -1

rtObject proc init-all args {
    foreach node $args {
	if { [$node rtObject?] == "" } {
	    set rtobj($node) [new rtObject $node]
	}
    }
    foreach node $args {	;# XXX
        $rtobj($node) compute-routes
    }
}

rtObject instproc init node {
    $self next
    $self instvar ns_ nullAgent_
    $self instvar nextHop_ rtpref_ metric_ node_ rtVia_ rtProtos_

    set ns_ [Simulator instance]
    set nullAgent_ [$ns_ set nullAgent_]

    $node init-routing $self
    set node_ $node
    foreach dest [$ns_ all-nodes-list] {
	set nextHop_($dest) ""
	if {$node == $dest} {
	    set rtpref_($dest) 0
	    set metric_($dest) 0
	    set rtVia_($dest) "Agent/rtProto/Local" ;# make dump happy
	} else {
	    set rtpref_($dest) [$class set maxpref_]
	    set metric_($dest) [$class set unreach_]
	    set rtVia_($dest)    ""
	    $node add-route [$dest id] $nullAgent_
	}
    }
    $self add-proto Direct $node
    $self compute-proto-routes
#    $self compute-routes
}

rtObject instproc add-proto {proto node} {
    $self instvar ns_ rtProtos_
    set rtProtos_($proto) [new Agent/rtProto/$proto $node]
    $ns_ attach-agent $node $rtProtos_($proto)
    set rtProtos_($proto)
}

rtObject instproc lookup dest {
    $self instvar nextHop_ node_
    if {![info exists nextHop_($dest)] || $nextHop_($dest) == ""} {
	return -1
    } else {
	return [[$nextHop_($dest) set toNode_] id]
    }
}

rtObject instproc compute-proto-routes {} {
    # Each protocol computes its best routes
    $self instvar rtProtos_
    foreach p [array names rtProtos_] {
	$rtProtos_($p) compute-routes
    }
}

rtObject instproc compute-routes {} {
    # choose the best route to each destination from all protocols
    $self instvar ns_ node_ rtProtos_ nullAgent_
    $self instvar nextHop_ rtpref_ metric_ rtVia_
    set protos ""
    set changes 0
    foreach p [array names rtProtos_] {
	if [$rtProtos_($p) set rtsChanged_] {
	    incr changes
	    $rtProtos_($p) set rtsChanged_ 0
	}
	lappend protos $rtProtos_($p)
    }
    if !$changes return

    set changes 0
    foreach dst [$ns_ all-nodes-list] {
	if {$dst == $node_} continue
	set nh ""
	set pf [$class set maxpref_]
	set mt [$class set unreach_]
	set rv ""
	foreach p $protos {
	    set pnh [$p set nextHop_($dst)]
	    if { $pnh == "" } continue

	    set ppf [$p set rtpref_($dst)]
	    set pmt [$p set metric_($dst)]
	    if {$ppf < $pf || ($ppf == $pf && $pmt < $mt) || $mt < 0} {
		set nh  $pnh
		set pf  $ppf
		set mt  $pmt
		set rv  $p
	    }
	}
	if { $nh == "" } {
	    # no route...  delete any existing routes
	    if { $nextHop_($dst) != "" } {
		$node_ delete-routes [$dst id] $nextHop_($dst) $nullAgent_
		set nextHop_($dst) $nh
		set rtpref_($dst)  $pf
		set metric_($dst)  $mt
		set rtVia_($dst)   $rv
		incr changes
	    }
	} else {
	    if { $rv == $rtVia_($dst) } {
		# Current protocol still has best route.
		# See if changed
		if { $nh != $nextHop_($dst) } {
		    $node_ delete-routes [$dst id] $nextHop_($dst) $nullAgent_
		    set nextHop_($dst) $nh
		    $node_ add-routes [$dst id] $nextHop_($dst)
		    incr changes
		}
		if { $mt != $metric_($dst) } {
		    set metric_($dst) $mt
		    incr changes
		}
		if { $pf != $rtpref_($dst) } {
		    set rtpref_($dst) $pf
		}
	    } else {
		if { $rtVia_($dst) != "" } {
		    set nextHop_($dst) [$rtVia_($dst) set nextHop_($dst)]
		    set rtpref_($dst)  [$rtVia_($dst) set rtpref_($dst)]
		    set metric_($dst)  [$rtVia_($dst) set metric_($dst)]
		}
		if {$rtpref_($dst) != $pf || $metric_($dst) != $mt} {
		    # Then new prefs must be better, or
		    # new prefs are equal, and new metrics are lower
		    $node_ delete-routes [$dst id] $nextHop_($dst) $nullAgent_
		    set nextHop_($dst) $nh
		    set rtpref_($dst)  $pf
		    set metric_($dst)  $mt
		    set rtVia_($dst)   $rv
		    $node_ add-routes [$dst id] $nextHop_($dst)
		    incr changes
		}
	    }
	}
    }
    foreach proto [array names rtProtos_] {
	$rtProtos_($proto) send-updates $changes
    }
    #
    # XXX
    # detailed multicast routing hooks must come here.
    # My idea for the hook will be something like:
    # set mrtObject [$node_ mrtObject?]
    # if {$mrtObject != ""} {
    #    $mrtObject recompute-mroutes $changes
    # }
    # $changes == 0	if only interfaces changed state.  Look at how
    #			Agent/rtProto/DV handles ifsUp_
    # $changes > 0	if new unicast routes were installed.
    #
    $self flag-multicast $changes
}

rtObject instproc flag-multicast changes {
    $self instvar node_
    if ![catch "$node_ getArbiter" mrtObject] {
	$mrtObject notify $changes
    }
}

rtObject instproc intf-changed {} {
    $self instvar ns_ node_ rtProtos_ rtVia_ nextHop_ rtpref_ metric_
    foreach p [array names rtProtos_] {
	$rtProtos_($p) intf-changed
    }
    $self compute-proto-routes
    $self compute-routes
}

proc TclObjectCompare {a b} {
    set o1 [string range $a 2 end]
    set o2 [string range $b 2 end]
    if {$o1 < $o2} {
	return -1
    } elseif {$o1 == $o2} {
	return 0
    } else {
	return 1
    }
}

rtObject instproc dump-routes chan {
    $self instvar ns_ node_ nextHop_ rtpref_ metric_ rtVia_

    if {$ns_ != ""} {
	set time [$ns_ now]
    } else {
	set time 0.0
    }
    puts $chan [concat "Node:\t${node_}([$node_ id])\tat t ="		\
	    [format "%4.2f" $time]]
    puts $chan "  Dest\t\t nextHop\tPref\tMetric\tProto"
    foreach dest [lsort -command TclObjectCompare [$ns_ all-nodes-list]] {
	if {[llength $nextHop_($dest)] > 1} {
	    set p [split [$rtVia_($dest) info class] /]
	    set proto [lindex $p [expr [llength $p] - 1]]
	    foreach rt $nextHop_($dest) {
		puts $chan [format "%-5s(%d)\t%-5s(%d)\t%3d\t%4d\t %s"	 \
			$dest [$dest id] $rt [[$rt set toNode_] id]	 \
			$rtpref_($dest) $metric_($dest) $proto]
	    }
	} elseif {$nextHop_($dest) != ""} {
	    set p [split [$rtVia_($dest) info class] /]
	    set proto [lindex $p [expr [llength $p] - 1]]
	    puts $chan [format "%-5s(%d)\t%-5s(%d)\t%3d\t%4d\t %s"	 \
		    $dest [$dest id]					 \
		    $nextHop_($dest) [[$nextHop_($dest) set toNode_] id] \
		    $rtpref_($dest) $metric_($dest) $proto]
	} elseif {$dest == $node_} {
	    puts $chan [format "%-5s(%d)\t%-5s(%d)\t%03d\t%4d\t %s"	\
		    $dest [$dest id] $dest [$dest id] 0 0 "Local"]
	} else {
	    puts $chan [format "%-5s(%d)\t%-5s(%s)\t%03d\t%4d\t %s"	\
		    $dest [$dest id] "" "-" 255 32 "Unknown"]
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
    foreach dest [[Simulator instance] all-nodes-list] {
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
    error "No initialisation defined"
}

Agent/rtProto instproc init node {
    $self next

    $self instvar ns_ node_ rtObject_ preference_ ifs_ ifstat_
    set ns_ [Simulator instance]

    catch "set preference_ [[$self info class] set preference_]" ret
    if { $ret == "" } {
	set preference_ [$class set preference_]
    }
    foreach nbr [$node set neighbor_] {
	set link [$ns_ link $node $nbr]
	set ifs_($nbr) $link
	set ifstat_($nbr) [$link up?]
    }
    set rtObject_ [$node rtObject?]
}

Agent/rtProto instproc compute-routes {} {
    error "No route computation defined"
}

Agent/rtProto instproc intf-changed {} {
    #NOTHING
}

Agent/rtProto instproc send-updates args {
    #NOTHING
}

Agent/rtProto proc compute-all {} {
    #NOTHING
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
Class Agent/rtProto/Direct -superclass Agent/rtProto

Agent/rtProto/Direct set preference_ 100

Agent/rtProto/Direct instproc init node {
    $self next $node
    $self instvar ns_ rtpref_ nextHop_ metric_ ifs_

    foreach node [$ns_ all-nodes-list] {
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
# Distance Vector Route Computation
#
# Class Agent/rtProto/DV -superclass Agent/rtProto

Agent/rtProto/DV set preference_	120
Agent/rtProto/DV set INFINITY		 32
Agent/rtProto/DV set UNREACHABLE	[rtObject set unreach_]
Agent/rtProto/DV set advertInterval	  2
Agent/rtProto/DV set mid_		  0

Agent/rtProto/DV proc init-all args {
    if { [llength $args] == 0 } {
	set nodeslist [[Simulator instance] all-nodes-list]
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
    $self instvar ns_ rtObject_ ifsUp_
    $self instvar preference_ rtpref_ nextHop_ nextHopPeer_ metric_ multiPath_

    set UNREACHABLE [$class set UNREACHABLE]
    foreach dest [$ns_ all-nodes-list] {
	set rtpref_($dest) $preference_
	set nextHop_($dest) ""
	set nextHopPeer_($dest) ""
	set metric_($dest)  $UNREACHABLE
    }
    set ifsUp_ ""
    set multiPath_ [[$rtObject_ set node_] set multiPath_]
    set updateTime [uniform 0.0 0.5]
    $ns_ at $updateTime "$self send-periodic-update"
}

Agent/rtProto/DV instproc add-peer {nbr agentAddr} {
    $self instvar peers_
    $self set peers_($nbr) [new rtPeer $agentAddr $class]
}

Agent/rtProto/DV instproc send-periodic-update {} {
    $self instvar ns_
    $self send-updates 1	;# Anything but 0
    set updateTime [expr [$ns_ now] + \
	    ([$class set advertInterval] * [uniform 0.9 1.1])]
    $ns_ at $updateTime "$self send-periodic-update"
}

Agent/rtProto/DV instproc compute-routes {} {
    $self instvar ns_ ifs_ rtpref_ metric_ nextHop_ nextHopPeer_
    $self instvar peers_ rtsChanged_ multiPath_

    set INFINITY [$class set INFINITY]
    set MAXPREF  [rtObject set maxpref_]
    set UNREACH	 [rtObject set unreach_]
    set rtsChanged_ 0
    foreach dst [$ns_ all-nodes-list] {
	set p [lindex $nextHopPeer_($dst) 0]
	if {$p != ""} {
	    set metric_($dst) [$p metric? $dst]
	    set rtpref_($dst) [$p preference? $dst]
	}

	set pf $MAXPREF
	set mt $INFINITY
	set nh(0) 0
	foreach nbr [array names peers_] {
	    set pmt [$peers_($nbr) metric? $dst]
	    set ppf [$peers_($nbr) preference? $dst]

	    # if peer metric not valid	continue
	    # if peer pref higher		continue
	    # if peer pref lower		set to latest values
	    # else peer pref equal
	    #	if peer metric higher	continue
	    #	if peer metric lower	set to latest values
	    #	else peer metrics equal	append latest values

	    if { $pmt < 0 || $pmt >= $INFINITY || $ppf > $pf || $pmt > $mt } \
		    continue
	    if { $ppf < $pf || $pmt < $mt } {
		set pf $ppf
		set mt $pmt
		unset nh	;# because we must compute *new* next hops
	    }
	    set nh($ifs_($nbr)) $peers_($nbr)
	}
	catch "unset nh(0)"
	if { $pf == $MAXPREF && $mt == $INFINITY } continue
	if { $pf > $rtpref_($dst) ||				\
		($metric_($dst) >= 0 && $mt > $metric_($dst)) }	\
		continue
	if {$mt >= $INFINITY} {
	    set mt $UNREACH
	}

	incr rtsChanged_
	if { $pf < $rtpref_($dst) || $mt < $metric_($dst) } {
	    set rtpref_($dst) $pf
	    set metric_($dst) $mt
	    set nextHop_($dst) ""
	    set nextHopPeer_($dst) ""
	    foreach n [array names nh] {
		lappend nextHop_($dst) $n
		lappend nextHopPeer_($dst) $nh($n)
		if !$multiPath_ break;
	    }
	    continue
	}
	
	set rtpref_($dst) $pf
	set metric_($dst) $mt
	set newNextHop ""
	set newNextHopPeer ""
	foreach rt $nextHop_($dst) {
	    if [info exists nh($rt)] {
		lappend newNextHop $rt
		lappend newNextHopPeer $nh($rt)
		unset nh($rt)
	    }
	}
	set nextHop_($dst) $newNextHop
	set nextHopPeer_($dst) $newNextHopPeer
	if { $multiPath_ || $nextHop_($dst) == "" } {
	    foreach rt [array names nh] {
		lappend nextHop_($dst) $rt
		lappend nextHopPeer_($dst) $nh($rt)
		if !$multiPath_ break
	    }
	}
    }
    set rtsChanged_
}

Agent/rtProto/DV instproc intf-changed {} {
    $self instvar ns_ peers_ ifs_ ifstat_ ifsUp_ nextHop_ nextHopPeer_ metric_
    set INFINITY [$class set INFINITY]
    set ifsUp_ ""
    foreach nbr [array names peers_] {
	set state [$ifs_($nbr) up?]
	if {$state != $ifstat_($nbr)} {
	    set ifstat_($nbr) $state
	    if {$state != "up"} {
		if ![info exists all-nodes] {
		    set all-nodes [$ns_ all-nodes-list]
		}
		foreach dest ${all-nodes} {
		    $peers_($nbr) metric $dest $INFINITY
		}
	    } else {
		lappend ifsUp_ $nbr
	    }
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

Agent/rtProto/DV instproc send-updates changes {
    $self instvar peers_ ifs_ ifsUp_

    if $changes {
	set to-send-to [array names peers_]
    } else {
	set to-send-to $ifsUp_
    }
    set ifsUp_ ""
    foreach nbr ${to-send-to} {
	if { [$ifs_($nbr) up?] == "up" } {
	    $self send-to-peer $nbr
	}
    }
}

Agent/rtProto/DV instproc send-to-peer nbr {
    $self instvar ns_ rtObject_ ifs_ peers_
    set INFINITY [$class set INFINITY]
    foreach dest [$ns_ all-nodes-list] {
	set metric [$rtObject_ metric? $dest]
	if {$metric < 0} {
	    set update($dest) $INFINITY
	} else {
	    set update($dest) [$rtObject_ metric? $dest]
	    foreach nh [$rtObject_ nextHop? $dest] {
		if {$nh == $ifs_($nbr)} {
		    set update($dest) $INFINITY
		}
	    }
	}
    }
    set id [$class get-next-mid]
    $class set msg_($id) [array get update]
#    set n [$rtObject_ set node_];					\
    puts stderr [concat [format ">>> %7.5f" [$ns_ now]]			\
	    "${n}([$n id]/[$self set addr_]) send update"		\
	    "to ${nbr}([$nbr id]/[$peers_($nbr) addr?]) id = $id"];	\
    set j 0;								\
    foreach i [lsort -command TclObjectCompare [array names update]] {	\
	puts -nonewline "\t${i}([$i id]) $update($i)";			\
        if {$j == 3} {							\
	    puts "";							\
	};								\
	set j [expr ($j + 1) % 4];					\
    };									\
    if $j { puts ""; }

    # XXX Note the singularity below...
    $self send-update [$peers_($nbr) addr?] $id [array size update]
}

Agent/rtProto/DV instproc recv-update {peerAddr id} {
    $self instvar peers_ ifs_ nextHopPeer_ metric_
    $self instvar rtsChanged_ rtObject_

    set INFINITY [$class set INFINITY]
    set UNREACHABLE  [$class set UNREACHABLE]
    set msg [$class retrieve-msg $id]
    array set metrics $msg
#    set n [$rtObject_ set node_];					\
    puts stderr [concat [format "<<< %7.5f" [[Simulator instance] now]]	\
	    "${n}([$n id]) recv update from peer $peerAddr id = $id"]
    foreach nbr [array names peers_] {
	if {[$peers_($nbr) addr?] == $peerAddr} {
	    set peer $peers_($nbr)
	    if { [array size metrics] > [Node set nn_] } {
		error "$class::$proc update $peerAddr:$msg:$count is larger than the simulation topology"
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
		incr rtsChanged_ $metricsChanged
		$rtObject_ compute-routes
	    } else {
		# dynamicDM multicast hack.
		# If we get a message from a neighbour, then something
		# at that neighbour has changed.  While this may not
		# cause any unicast changes on our end, dynamicDM
		# looks at neighbour's routing tables to compute
		# parent-child relationships, and has to do them
		# again.
		#
		$rtObject_ flag-multicast -1
	    }
	    return
	}
    }
    error "$class::$proc update $peerAddr:$msg:$count from unknown peer"
}

Agent/rtProto/DV proc compute-all {} {
    # Because proc methods are not inherited from the parent class.
}

### Local Variables:
### mode: tcl
### tcl-indent-level: 4
### tcl-default-application: ns
### End:

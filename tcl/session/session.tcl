Class SessionSim -superclass Simulator

SessionSim instproc node {} {
    $self instvar Node_
    set node [new SessionNode]
    set Node_([$node id]) $node
    return $node
}

SessionSim instproc create-session { node agent } {
    $self instvar session_

    set nid [$node id]
    set dst [$agent set dst_]
    set session_($nid:$dst) [new Classifier/Replicator/Demuxer]
    $agent target $session_($nid:$dst)
    return $session_($nid:$dst)
}

SessionSim instproc join-group { agent group } {
    $self instvar session_

    foreach index [array names session_] {
	set pair [split $index :]
	if {[lindex $pair 1] == $group} {
	    #Note: must insert the chain of loss, delay, 
	    # and destination agent in this order:

	    #1. insert destination agent into session replicator
	    $session_($index) insert $agent

	    #2. find accumulated bandwidth and delay
	    set src [lindex $pair 0]
	    set dst [[$agent set node_] id]
	    set accu_bw [$self get-bw $dst $src]
	    set delay [$self get-delay $dst $src]

	    #3. set up a constant delay module
	    set random_variable [new RandomVariable/Constant]
	    $random_variable set val_ $delay
	    set delay_module [new DelayModel]
	    $delay_module bandwidth $accu_bw
	    $delay_module ranvar $random_variable
	    
	    #4. insert the delay module in front of the dest agent
	    $session_($index) insert-module $delay_module $agent

	    #5. set up a constant loss module
	    #set loss_random_variable [new RandomVariable/Constant]
	    #$loss_random_variable set avg_ 2
	    #set loss_module [new ErrorModel]
	    # when ranvar avg_ < erromodel rate_ pkts are dropped
	    #$loss_module drop-target [new Agent/Null]
	    #$loss_module set rate_ 1
	    #$loss_module ranvar $loss_random_variable

	    #6. insert the loss module in front of the delay module
	    #$session_($index) insert-module $loss_module $delay_module

	    # puts "add-dst $accu_bw $delay $src $dst"
	}
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
	}
    }
}

SessionSim instproc simplex-link { n1 n2 bw delay type } {
    $self instvar link_ delay_
    set sid [$n1 id]
    set did [$n2 id]

    set link_($sid:$did) [expr [string trimright $bw Mb] * 1000000]
    set delay_($sid:$did) [expr [string trimright $delay ms] * 0.001]
}

SessionSim instproc simplex-link-of-interfaces { n1 n2 bw delay type } {
    $self simplex-link $n1 $n2 $bw $delay $type
}

SessionSim instproc duplex-link-of-interfaces { n1 n2 bw delay type } {
    $self simplex-link $n1 $n2 $bw $delay $type
    $self simplex-link $n2 $n1 $bw $delay $type
}

SessionSim instproc compute-routes {} {
	$self instvar routingTable_ link_
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

SessionSim instproc run args {
	#$self compute-routes
	$self rtmodel-configure			;# in case there are any
	eval RouteLogic configure $args
	$self instvar scheduler_ Node_
	#
	# Reset every node, which resets every agent
	#
	foreach nn [array names Node_] {
		$Node_($nn) reset
	}
        return [$scheduler_ run]
}
############## SessionNode ##############
Class SessionNode -superclass Node
SessionNode instproc init {} {
    $self instvar id_ np_
    set id_ [Node getid]
    set np_ 0
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
    $self instvar id_

    $agent set node_ $self
    set port [$self alloc-port]
    $agent set addr_ [expr $id_ << 8 | $port]
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

Classifier/Replicator/Demuxer instproc insert-module {module target} {
        $self instvar active_ index_

        if ![info exists active_($target)] {
	    puts "error: insert module, but $target does not exist in any slot of replicator $self."
	    exit 0
        } elseif {!$active_($target)} {
	    puts "error: insert module, but $target is not active in replicator $self."
	    exit 0
	}
        
	$module target $target
	set n $index_($target)
        $self install $n $module
        set active_($module) 1
	set index_($module) $n
	unset active_($target)
}


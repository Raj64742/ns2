Class SessionSim -superclass Simulator

SessionSim instproc bw_parse { bspec } {
        if { [scan $bspec "%f%s" b unit] == 1 } {
                set unit bps
        }
        switch $unit {
        b  { return $b }
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

SessionSim instproc node { {shape "circle"} {color "black"} } {
    $self instvar Node_ namtraceAllFile_
    set node [new SessionNode]
    set Node_([$node id]) $node

	if [info exists namtraceAllFile_] {
		$node trace $namtraceAllFile_ $shape $color
	}
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

            $self update-dependency $dst $src $group
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

Simulator instproc update-dependency { src dst group } {
    $self instvar routingTable_ Node_
    set tmp $src
    while {$tmp != $dst} {
        set next [$routingTable_ lookup $tmp $dst]
        set nextnode $Node_($next)
        $nextnode insert-child $dst $group $Node_($tmp)
        set tmp $next
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

    set link_($sid:$did) [$self bw_parse $bw]
    set delay_($sid:$did) [$self delay_parse $delay]
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

SessionSim instproc run {} {
	#$self compute-routes
	$self rtmodel-configure			;# in case there are any
	[$self get-routelogic] configure
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
    $self instvar id_ agents_

    $agent set node_ $self
    lappend agents_ $agent
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

Node instproc insert-child { src group child } {
    $self instvar child_
    set group [expr $group]
    set childlist [$self get-child $src $group]
    if {[lsearch $childlist $child] < 0} {
        lappend childlist $child
        set child_($src:$group) $childlist
    }
}

Node instproc get-child { src group } {
    $self instvar child_
    set group [expr $group]
    if [info exists child_($src:$group)] {
        return $child_($src:$group)
    } else {
        return ""
    }
}

Node instproc get-dependency { src group } {
    $self instvar child_
    set group [expr $group]
    set returnlist ""
    set allchild "$self"

    while { $allchild != "" } {
        set tmp [lindex $allchild 0]
        set allchild [lreplace $allchild 0 0]
        lappend returnlist $tmp
        foreach child [$tmp get-child $src $group] {
            lappend allchild $child
        }
    }
    return $returnlist
}

Node instproc dump-dependency { src group } {
    $self instvar child_
    set group [expr $group]
    set allchild "$self:0"

    while { $allchild != "" } {
        set fixed ""
        set tmp [lindex $allchild 0]
        set allchild [lreplace $allchild 0 0]
        set tmp [split $tmp :]
        for {set i 0} {$i < [lindex $tmp 1]} {incr i} {
            puts -nonewline "\t"
        }
        puts [[lindex $tmp 0] id]
        foreach child [[lindex $tmp 0] get-child $src $group] {
            lappend fixed $child:[expr [lindex $tmp 1] + 1]
        }
        set allchild [concat $allchild $fixed]
    }
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

Classifier/Replicator/Demuxer instproc insert-loss {loss_module target} {
    $self instvar index_

    if [info exists index_($target)] {
        set current_target [$self slot $index_($target)]
        $self insert-module $loss_module $current_target
    }
}

Classifier/Replicator/Demuxer instproc insert-depended-loss {loss_module target src group} {

    $self instvar index_ active_
    set group [expr $group]
    set src [[$src set node_] id]
    set alldepended [[$target set node_] get-dependency $src $group]

    #puts "found bad node dependency: $alldepended"
    #puts -nonewline "real member in dependency: "

    set newrep [new Classifier/Replicator/Demuxer]

    foreach depended $alldepended {
	# puts -nonewline "$depended "
	foreach agent [$depended getAgents] {
	    # puts "insert loss for [$agent info class]"
	    if [info exists index_($agent)] {
		set current_target [$self slot $index_($agent)]
		$newrep insert $current_target
		$self disable $current_target
	    }
	}
    }
    if [$newrep is-active] {
	$self insert $loss_module
	$loss_module target $newrep
    } else {
	delete $newrep
    }
}

Node instproc getAgents {} {
    $self instvar agents_

    if [info exists agents_] {
	return $agents_
    } else {
	return ""
    }
}

### to insert loss module to regular links in detailed Simulator
Simulator instproc lossmodel {lossobj from to} {
    set link [$self link $from $to]
    set head [$link head]
    # puts "[[$head target] info class]"
    $lossobj target [$head target]
    $head target $lossobj
    # puts "[[$head target] info class]"
}
    



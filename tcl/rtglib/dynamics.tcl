Class rtQueue

DynamicLink set status_ 1

Simulator instproc rtmodel { dist parms args } {
    set ret ""
    if { [rtModel info subclass rtModel/$dist] != "" } {
	$self instvar rtq_ traceAllFile_
	if ![info exists rtq_] {
	    set rtq_ [new rtQueue $self]
	}
	set ret [eval new rtModel/$dist $self]
	eval $ret set-elements $args
	eval $ret set-parms $parms
	if [info exists traceAllFile_] {
	    $ret trace $self $traceAllFile_
	}
    }
    return $ret
}


SimpleLink instproc dynamic {} {
    $self instvar dynamics_ queue_ head_ enqT_ drpT_

    set dynamics_ [new DynamicLink]
    $dynamics_ target $head_
    set head_ $dynamics_

    if [info exists drpT_] {
	$dynamics_ down-target $drpT_
    } else {
        $dynamics_ down-target [[Simulator instance] set nullAgent_]
    }
    $self all-connectors dynamic
}

Link instproc up { } {
    $self instvar dynamics_ dynT_
    $dynamics_ set status_ 1
    if [info exists dynT_] {
	foreach tr $dynT_ {
	    $tr format link-up {$src_} {$dst_}
	}
    }
}

Link instproc down { } {
    $self instvar dynamics_ dynT_
    $dynamics_ set status_ 0
    $self all-connectors reset
    if [info exists dynT_] {
	foreach tr $dynT_ {
	    $tr format link-down {$src_} {$dst_}
	}
    }
}

Link instproc up? {} {
    $self instvar dynamics_
    if [info exists dynamics_] {
	return [$dynamics_ status?]
    } else {
	return "up"
    }
}

Link instproc all-connectors op {
    foreach c [$self info vars] {
	if {$c == "fromNode_" || $c == "toNode_"} {
	} else {
	    $self instvar $c
	    catch "$$c $op"   ;# in case the instvar is not a connector
	}
    }
}

SimpleLink instproc trace-dynamics { ns f } {
    $self instvar dynT_ fromNode_ toNode_
    lappend dynT_ [$ns create-trace Generic $f $fromNode_ $toNode_]
}

Node instproc intf-changed { } {
    $self instvar rtObject_
    if [info exists rtObject_] {	;# i.e. detailed dynamic routing
        $rtObject_ intf-changed
    }
}

#
rtQueue instproc init ns {
    $self next
    $self instvar ns_
    set ns_ $ns
}

rtQueue instproc insq { interval obj iproc args } {
    $self instvar rtq_ ns_
    set time [expr $interval + [$ns_ now]]
    if ![info exists rtq_($time)] {
	$ns_ at $time "$self runq $time"
    }
    lappend rtq_($time) "$obj $iproc $args"
    return $time
}

rtQueue instproc delq { time obj } {
    $self instvar rtq_
    set ret ""
    set nevent ""
    if [info exists rtq_($time)] {
	foreach event $rtq_($time) {
	    if {[lindex $event 0] != $obj} {
		lappend nevent $event
	    } else {
		set ret $event
	    }
	}
	set rtq_($time) $nevent		;# XXX
    }
    return ret
}

rtQueue instproc runq { time } {
    $self instvar rtq_
    set objects ""
    foreach event $rtq_($time) {
	set obj   [lindex $event 0]
	set iproc [lindex $event 1]
	set args  [lrange $event 2 end]
	eval $obj $iproc $args
	lappend objects $obj
    }
    foreach obj $objects {
	$obj notify
    }
    unset rtq_($time)
}

#
Class rtModel

rtModel set rtq_ ""
rtModel set quiesceTime_ 0.5

rtModel instproc init ns {
    $self next
    if { [rtModel set rtq_] == "" } {
	rtModel set rtq_ [$ns set rtq_]
    }
}

rtModel instproc set-elements args {
    $self instvar links_ nodes_
    set ns [Simulator instance]
    if { [llength $args] == 2 } {
	set n0 [lindex $args 0]
	set n1 [lindex $args 1]

	set l0 [$ns link $n0 $n1]
	set l1 [$ns link $n1 $n0]
	$l0 dynamic
	$l1 dynamic

	set n0id [$n0 id]
	set n1id [$n1 id]
	set nodes_($n0id) $n0
	set nodes_($n1id) $n1
	set links_($n0id:$n1id) $l0
	set links_($n1id:$n0id) $l1
    } else {
	set n0 [lindex $args 0]
	set n0id [$n0 id]
	set nodes_($n0id) $n0
	foreach nbr [$n0 set neighbor_] {
	    set n1 $nbr

	    set l0 [$ns link $n0 $n1]
	    set l1 [$ns link $n1 $n0]
	    $l0 dynamic
	    $l1 dynamic

	    set n1id [$n1 id]
	    set nodes_($n1id) $n1
	    set links_($n0id:$n1id) $l0
	    set links_($n1id:$n0id) $l1
	}
    }
}

rtModel instproc up {} {
    $self instvar links_
    foreach l [array names links_] {
	$links_($l) up
    }
}

rtModel instproc down {} {
    $self instvar links_
    foreach l [array names links_] {
	$links_($l) down
    }
}

rtModel instproc notify {} {
    $self instvar nodes_
    foreach n [array names nodes_] {
	$nodes_($n) intf-changed
    }
    [RouteLogic info instances] notify
}

rtModel instproc trace { ns f } {
    $self instvar links_
    foreach l [array names links_] {
	$links_($l) trace-dynamics $ns $f
    }
}

#
# Exponential link failure/recovery models
#

Class rtModel/Exponential -superclass rtModel

rtModel/Exponential set upInterval_   10.0
rtModel/Exponential set downInterval_  1.0

rtModel/Exponential instproc init ns {
    eval $self next $ns

    $self instvar upInterval_ downInterval_
    set upInterval_ [$class set upInterval_]
    set downInterval_ [$class set downInterval_]

    $self instvar firstEv_
    set interval			\
	    [expr [rtModel set quiesceTime_] + [exponential] * $upInterval_]
    set firstEv_ [[rtModel set rtq_] insq $interval $self "down"]
}

rtModel/Exponential instproc set-parms args {
    $self instvar upInterval_ downInterval_ firstEv_

    set changes 0
    if { [llength $args] >= 1 } {
	set ui [lindex $args 0]
	if {$ui != "-"} {
	    set upInterval_ [lindex $args 0]
	    incr changes
	}
    }

    if { [llength $args] >= 2 } {
	set downInterval_ [lindex $args 1]
	incr changes
    }
    
    if {$changes} {
        [rtModel set rtq_] delq $firstEv_ $self
        set interval			\
	        [expr [rtModel set quiesceTime_] + [exponential] * $upInterval_]
        [rtModel set rtq_] insq $interval $self "down"
    }
}

rtModel/Exponential instproc up { } {
    $self next
    $self instvar upInterval_
    set interval [expr [exponential] * $upInterval_]
    [rtModel set rtq_] insq $interval $self "down"
}

rtModel/Exponential instproc down { } {
    $self next
    $self instvar downInterval_
    set interval [expr [exponential] * $downInterval_]
    [rtModel set rtq_] insq $interval $self "up"
}

#
# Deterministic link failure/recovery models
#

Class rtModel/Deterministic -superclass rtModel

rtModel/Deterministic set upInterval_   2.0
rtModel/Deterministic set downInterval_ 1.0

rtModel/Deterministic instproc init ns {
    eval $self next $ns

    $self instvar upInterval_ downInterval_
    set upInterval_ [$class set upInterval_]
    set downInterval_ [$class set downInterval_]

    $self instvar firstEv_
    set interval [expr $upInterval_ + [rtModel set quiesceTime_]]
    set firstEv_ [[rtModel set rtq_] insq $interval $self "down"]
}


rtModel/Deterministic instproc set-parms args {
    $self instvar upInterval_ downInterval_ firstEv_

    set changes 0
    if { [llength $args] >= 1 } {
	set ui [lindex $args 0]
	if {$ui != "-"} {
	    set upInterval_ [lindex $args 0]
	    incr changes
	}
    }
    if { [llength $args] >= 2 } {
	set downInterval_ [lindex $args 1]
	incr changes
    }

    if {$changes} {
	[rtModel set rtq_] delq $firstEv_ $self
	set interval [expr $upInterval_ + [rtModel set quiesceTime_]]
	[[rtModel set rtq_] insq $interval $self "down"]
    }
}

rtModel/Deterministic instproc up { } {
    $self next
    $self instvar upInterval_

    [rtModel set rtq_] insq $upInterval_ $self "down"
}

rtModel/Deterministic instproc down { } {
    $self next
    $self instvar downInterval_

    [rtModel set rtq_] insq $downInterval_ $self "up"
}

Class rtModel/Trace -superclass rtModel

rtModel/Trace instproc init ns {
    $self next $ns
    $self instvar ns_
    set ns_ [Simulator instance]
}

rtModel/Trace instproc get-next-event {} {
    $self instvar tracef_ links_
    while {[gets $tracef_ event] >= 0} {
	set toks [split $event]
	if [info exists links_([lindex $toks 3]:[lindex $toks 4])] {
	    return $toks
	}
    }
    return ""
}

rtModel/Trace instproc set-events {} {
    $self instvar ns_ nextEvent_ evq_
    
    set time [lindex $nextEvent_ 1]
    set interval [expr $time - [$ns_ now]]
    while {$nextEvent_ != ""} {
	set nextTime [lindex $nextEvent_ 1]
	if {$nextTime < $time} {
	    puts stderr		\
		    "event $nextEvent_  is before current time $time. ignored."
	    continue
	}
	if {$nextTime > $time} {
	    break
	}
	if ![info exists evq_($time)] {
	    set op [string range [lindex $nextEvent_ 2] 5 end]
	    [rtModel set rtq_] insq $interval $self $op
	    set evq_($time) 1
	}
	set nextEvent_ [$self get-next-event]
    }
}


rtModel/Trace instproc set-parms traceFile {
    $self instvar tracef_ nextEvent_
    set tracef_ [open $traceFile "r"]
    set nextEvent_ [$self get-next-event]
    if {$nextEvent_ == ""} {
	error "no relevant events in $traceFile"
    }
    $self set-events
}

rtModel/Trace instproc up {} {
    $self next
    $self set-events
}

rtModel/Trace instproc down {} {
    $self next
    $self set-events
}


Class rtQueue

Dynamic/Link set status_ 1

Simulator instproc rtmodel { dist parms args } {
    set ret ""
    if { [rtModel info subclass rtModel/$dist] != "" } {
	$self instvar rtq_ traceAllFile_
	if ![info exists rtq_] {
	    set rtq_ [new rtQueue $self]
	}
	set ret [eval new rtModel/$dist $self {$parms} $args]
	if [info exists traceAllFile_] {
	    $ret trace $self $traceAllFile_
	}
    }
    return $ret
}


SimpleLink instproc dynamic ns {
    $self instvar dynamics_ queue_ head_ drpT_

    set dynamics_ [new Dynamic/Link]
    $dynamics_ target $head_
    set head_ $dynamics_
    if [info exists drpT_] {
	$dynamics_ down-target $drpT_
    } else {
        $dynamics_ down-target [$ns set nullAgent_]
    }
    $self all-connectors dynamic
}

SimpleLink instproc up { } {
    $self instvar dynamics_
    $dynamics_ set status_ 1
}

SimpleLink instproc down { } {
    $self instvar dynamics_
    $dynamics_ set status_ 0
    $self all-connectors reset
}

SimpleLink instproc up? {} {
    $self instvar dynamics_
    if [info exists dynamics_] {
	return [$dynamics_ status?]
    } else {
	return "up"
    }
}

SimpleLink instproc all-connectors op {
    foreach c [$self info vars] {
	if {$c == "fromNode_" || $c == "toNode_"} {
	} else {
	    $self instvar $c
	    catch "$$c $op"   ;# in case the instvar is not a connector
	}
    }
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
    if [info exists rtq_($time)] {
	foreach $event $rtq_($time) {
	    if {[lindex $event 0] != $obj} {
		lappend nevent $event
	    } else {
		set ret $event
	    }
	}
	set rtq_($time) $nevent
    }
    return ret
}

rtQueue instproc runq { time } {
    $self instvar rtq_
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

rtModel instproc init { ns args } {
    $self next

    $self instvar links_ nodes_ element_
    if { [rtModel set rtq_] == "" } {
	rtModel set rtq_ [$ns set rtq_]
    }
    set lc 0
    set nc 0
    if { [llength $args] == 2 } {
	set element_ "link"			;# Link($n0:$n1) is dynamic
	set n0 [lindex $args 0]
	set nodes_($nc) $n0
	incr nc

	set n1 [lindex $args 1]
	set nodes_($nc) $n1
	incr nc

	set links_($lc) [$ns link $n0 $n1]
	$links_($lc) dynamic $ns
	incr lc

	set links_($lc) [$ns link $n1 $n0]
	$links_($lc) dynamic $ns
	incr lc
    } else {
	set element_ "node"			;# Node($n0) is dynamic
	set n0 [lindex $args 0]
	set nodes_($nc) $n0
	incr nc
	foreach nbr [$n0 set neighbor_] {
	    set n1 $nbr
	    set nodes_($nc) $n1
	    incr nc

	    set links_($lc) [$ns link $n0 $n1]
	    $links_($lc) dynamic $ns
	    incr lc

	    set links_($lc) [$ns link $n1 $n0]
	    $links_($lc) dynamic $ns
	    incr lc
	}
    }
}

rtModel instproc up {} {
    $self instvar links_ trace_
    foreach i [array names links_] {
	$links_($i) up
    }
    if [info exists trace_] {
	$self log-dynamic up
    }
}

rtModel instproc down {} {
    $self instvar links_ trace_
    foreach i [array names links_] {
	$links_($i) down
    }
    if [info exists trace_] {
	$self log-dynamic down
    }
}

rtModel instproc notify {} {
    $self instvar nodes_
    foreach i [array names nodes_] {
	$nodes_($i) intf-changed
    }
    [RouteLogic info instances] notify
}

rtModel instproc trace { ns f } {
    $self instvar trace_ element_ nodes_

    $self set trace_ [switch $element_ {
	link { $ns create-trace Generic $f $nodes_(0) $nodes_(1) }
	node { $ns create-trace Generic $f $nodes_(0) $nodes_(0) }
    }]
}

rtModel instproc log-dynamic op {
    $self instvar trace_ element_ links_ nodes_

    switch $element_ {
	link {
	    $trace_ format link-$op {$src_} {$dst_} 
	    $trace_ format link-$op {$dst_} {$src_}
	}
	node {
	    $trace_ format node-$op {$src_}
	    set nodecnt [array size nodes_]
	    for {set i 1} {$i < $nodecnt} {incr i} {
		set dst [$nodes_($i) id] 
		$trace_ format link-$op {$src_} $dst
		$trace_ format link-$op  $dst  {$src_}
	    }
	}
    }
}

rtModel instproc create-trace f {
    $self instvar trace_
    puts stderr "$self/$proc/$class So, why do I come here?"
    set trace_ $f
}

#
# Exponential link failure/recovery models
#

Class rtModel/Exponential -superclass rtModel

rtModel/Exponential set upInterval_   10.0
rtModel/Exponential set downInterval_  1.0

#Code to generate random numbers here
proc exponential {} {
    return [expr - log ([ns-random] / 0x7fffffff)]
}

rtModel/Exponential instproc init { ns parms args } {
    $self next $ns $args

    $self parms $parms

    $self instvar upInterval_
    set interval [expr [rtModel set quiesceTime_] +
			[exponential] * $upInterval_]
    [rtModel set rtq_] insq $interval $self "down" $args
}

rtModel/Exponential instproc parms args {
    $self instvar upInterval_ downInterval_

    if { [llength $args] >= 1 } {
	set upInterval_ [lindex args 0]
    } else {
	set upInterval_ [$class set upInterval_]
    }

    if { [llength $args] >= 2 } {
	set downInterval_ [lindex args 1]
    } else {
	set downInterval_ [$class set downInterval_]
    }
}

rtModel/Exponential instproc up { } {
    $self next
    $self instvar upInterval_
    set interval [expr [exponential] * $upInterval_]
    [rtModel set rtq_] insq $interval $self "down" $args
}

rtModel/Exponential instproc down { } {
    $self next
    $self instvar downInterval_
    set interval [expr [exponential] * $downInterval_]
    [rtModel set rtq_] insq $interval $self "up" $args
}

#
# Deterministic link failure/recovery models
#

Class rtModel/Deterministic -superclass rtModel

rtModel/Deterministic set upInterval_   2.0
rtModel/Deterministic set downInterval_ 1.0

rtModel/Deterministic instproc init { ns parms args } {
    eval $self next $ns $args
    $self instvar upInterval_

    eval $self parms $parms
    set interval [expr $upInterval_ + [rtModel set quiesceTime_]]
    [rtModel set rtq_] insq $interval $self "down"
}


rtModel/Deterministic instproc parms args {
    $self instvar upInterval_ downInterval_

    if { [llength $args] >= 1 } {
	set upInterval_ [lindex $args 0]
    } else {
	set upInterval_ [$class set upInterval_]
    }

    if { [llength $args] >= 2 } {
	set downInterval_ [lindex $args 1]
    } else {
	set downInterval_ [$class set downInterval_]
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

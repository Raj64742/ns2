Trace instproc init type {
    $self next $type
    $self instvar type_
    set type_ $type
}

Trace instproc format args {
    # The strange puts construction below helps us write formats such as
    # 	$traceObject format {$src_} {$dst_} 
    # that will then put the source or destination id in the desired place.

    $self instvar type_ fp_ src_ dst_

    set ns [Simulator instance]
    puts $fp_ [eval list $type_ [$ns now] [eval concat $args]]
}

Trace instproc attach fp {
    $self instvar fp_

    set fp_ $fp
    $self cmd attach $fp_
}

Class Trace/Hop -superclass Trace
Trace/Hop instproc init {} {
    $self next "h"
}

Class Trace/Enque -superclass Trace
Trace/Enque instproc init {} {
    $self next "+"
}

Class Trace/Deque -superclass Trace
Trace/Deque instproc init {} {
    $self next "-"
}

Class Trace/Drop -superclass Trace
Trace/Drop instproc init {} {
    $self next "d"
}

Class Trace/Generic -superclass Trace
Trace/Generic instproc init {} {
    $self next "v"
}

proc gc o {
    if { $o != "" } {
	return [$o info class]
    } else {
	return "NULL_OBJECT"
    }
}

Node instproc tn {} {
    $self instvar id_
    return "${self}(id $id_)"
}

Simulator instproc gen-map {} {
    $self instvar Node_ link_

    set nn [Node set nn_]
    for {set i 0} {$i < $nn} {incr i} {
	set n $Node_($i)
	puts "Node [$n tn]"
	foreach nc [$n info vars] {
	    switch $nc {
		"ns_" -
		"id_" -
		"neighbor_" -
		"agents_" -
		"np_" { continue }
		default {
		    set v [$n set $nc]
		    puts "\t\t$nc${v}([gc $v])"
		}
	    }
	}
	foreach li [array names link_ [$n id]:*] {
	    set L [split $li :]
	    set nbr [[$self get-node-by-id [lindex $L 1]] entry]
	    set ln $link_($li)
	    puts "\tLink $ln, fromNode_ [[$ln set fromNode_] tn] -> toNode_ [[$ln set toNode_] tn]"
	    puts "\tComponents (in order) head first"
	    for {set c [$ln head]} {$c != $nbr} {set c [$c target]} {
		puts "\t\t$c\t[gc $c]"
	    }
	}
	# Would be nice to dump agents attached to the dmux here?
	if {[llength [$n set agents_]] > 0} {
	    puts ""
	    puts "\tAgents at node (possibly in order of creation):"
	    foreach a [$n set agents_] {
		puts "\t\t$a\t[gc $a]\t\tportID: [$a set portID_]([$a set addr_])"
	    }
	}
	puts "---"
    }
}

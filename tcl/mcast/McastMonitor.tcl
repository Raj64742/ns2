Class McastMonitor
McastMonitor set period_ 0.03

McastMonitor instproc init sim {
    $self set ns_	$sim
    $self set period_	[$class set period_]
}

McastMonitor instproc trace-topo {source group} {
    $self instvar ns_ period_ links_

    if ![info exists links_($source:$group)] {
	set links_($source:$group) [$ns_ all-links-list]
    }
    $self trace-links $source $group
    $self print-trace $source $group
    $ns_ after $period "$self $proc $source $group"
}

McastMonitor instproc trace-tree {source group args} {
    $self instvar ns_ links_ period_

    if ![info exists links_($source:$group)] {
	set links_($source:$group) ""
	foreach member $args {
	    set tmpupstream -1
	    set tmp $member
	    while {$tmpupstream != $source} {
		set tmpupstream [$tmp rpf-nbr $source]
		set tmpupstream [[$ns_ upstream-node $tmp $source] id]
		set tmplink [$ns_ RPF-link $source $tmpupstream $tmp]
		# XXX need to add the other direction
		if {[lsearch $links_($source:$group) $tmplink] >= 0} {
		    break
		}
		lappend links_($source:$group) $tmplink
		set tmpfrom [[$tmplink set fromNode_] id]
		set tmpto [[$tmplink set toNode_] id]
		lappend links_($source:$group) [$ns_ set link_($tmpto:$tmpfrom)]
		set tmp $tmpupstream
	    }
	}
    }
    $self trace-links $source $group
    $self print-trace $source $group
    eval $ns after $period "$self $proc $source $group $args"
}

McastMonitor instproc trace-links {source group} {
    $self instvar ns_ links_

    foreach l $links_($source:$group) {
	$self pktintran $l
    }
}

McastMonitor instproc pktintran link {
	$self instvar pre_ post_
	array set preCounters [$pre_($link) dump-counters]
	array set postCounters [$post_($link) dump-counters]
	foreach type [array names preCounters] {
		set inTransit($type) [expr $preCounters($type) - $postCounters($type)]
	}
	array get inTransit($type)
}

McastMonitor instproc print-trace {source group} {
    $self instvar ns_ prune_ graft_ register_ data_ period_ links_
    set prune 0
    set graft 0
    set register 0
    set data 0
    
    foreach l $links_($source:$group) {
	set from [[$l set fromNode_] id]
	set to [[$l set toNode_] id]
	incr prune $prune_($source:$group:$from:$to)
	incr graft $graft_($source:$group:$from:$to)
	incr register $register_($source:$group:$from:$to)
	incr data $data_($source:$group:$from:$to)
    }
    puts "[$ns_ now] $prune $graft $register $data $source $group"
}
	

Simulator instproc McastMonitor {} {
    $self instvar mcastmonitor_ link_

    set mcastmonitor_ [new McastMonitor $self]
    foreach l [array names link_] {
	$link_($l) dynamic
    }
    return $mcastmonitor_
}

Simulator instproc all-links-list {} {
    $self instvar link_
    set links ""
    foreach n [array names link_] {
	lappend links $link_($n)
    }
    set links
}

DelayLink instproc puttrace {prune graft register data source group from to} {
    $self instvar ns_

    set ns_ [Simulator instance]
    set mmonitor [$ns_ set mcastmonitor_]
    
    $mmonitor instvar prune_ graft_ register_ data_
    set prune_($source:$group:$from:$to) $prune
    set graft_($source:$group:$from:$to) $graft
    set register_($source:$group:$from:$to) $register
    set data_($source:$group:$from:$to) $data
}


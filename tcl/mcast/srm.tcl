# This is the port of the initial SRM implementation in ns-1 by
# Kannan Varadahan <kannan@isi.edu>. Most of the code is in C++. Since
# then, there has been another implementation. The new implemenatation
# has better c++/tcl separation. Check later releases for new
# implementation. This code has been checked in for archival reasons.
# -Puneet Sharma <puneet@isi.edu>


proc mvar args {
	upvar self _s
	uplevel $_s instvar $args
}

Agent/SRM set uniq_srcid 0
Agent/SRM  proc alloc_srcid {} {
	set id [Agent/SRM set uniq_srcid]
	Agent/SRM set uniq_srcid [expr $id+1]
	return $id
}


Agent/SRM instproc init {} {
	$self instvar ns_
	$self next 
	$self set packet-size 1024
	set ns_ [Simulator instance]
	mvar srcid_ localsrc_
	set srcid_ [Agent/SRM alloc_srcid]
	set localsrc_ [new SRMSource $srcid_]
	puts "created srm agent $srcid_\n"
	$self localsrc $srcid_
}

Agent/SRM instproc start {} {
	mvar group_
	if ![info exists group_] {
		puts "error: can't transmit before joining group!"
		exit 1
	}
	$self start_
}


Agent/SRM instproc attach-to { node } {
	$self instvar ns_
	$ns_ attach-agent $node $self
	$self set node_ $node
}

Agent/SRM instproc join-group { g } {
	set g [expr $g]
	$self set group_ $g
	mvar node_
	$self set dst_ $g
	$node_ join-group $self $g
}

Agent/SRM instproc leave-group { } {
	mvar group_ node_
	$node_ leave-group $self $group_
	$self unset group_
}

# Not used currently, for rate controlling session messages
Agent/SRM instproc session_bw { bspec } {
	set b [bw_parse $bspec]

	$self set session_bw_ $b
}

# Defaults 
Agent/SRM set seqno -1
Agent/SRM set maxpkts 10
Agent/SRM set c1 2.0
Agent/SRM set c2 2.0
Agent/SRM set d1 1.0
Agent/SRM set d2 1.0
Agent/SRM set packet-size 1024

SRMSource set srcid_ -1

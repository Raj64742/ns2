#
# XXX hack: add some methods to the internal SimpleLink class
# to get at queue delay.  XXX only works if queue monitoring
# was enabled on this link
#
# Returns 2 element list of mean bytes and mean number of packets in queue
#
SimpleLink instproc sample-queue-size {} {
	$self instvar qMonitor_ lastSample_ qBytes_ qPkts_
	global ns
	set now [$ns now]
	
	set qBytesMonitor_ [$qMonitor_ get-bytes-integrator]
	set qPktsMonitor_ [$qMonitor_ get-pkts-integrator]

	$qBytesMonitor_ newpoint $now [$qBytesMonitor_ set lasty_]
	set bsum [$qBytesMonitor_ set sum_]

	$qPktsMonitor_ newpoint $now [$qPktsMonitor_ set lasty_]
	set psum [$qPktsMonitor_ set sum_]

	if ![info exists lastSample_] {
		set lastSample_ 0
	}
	set dur [expr $now - $lastSample_]
	if { $dur != 0 } {
		set meanBytesQ [expr $bsum / $dur]
		set meanPktsQ [expr $psum / $dur]
	} else {
		set meanBytesQ 0
		set meanPktsQ 0
	}
	$qBytesMonitor_ set sum_ 0.0
	$qPktsMonitor_ set sum_ 0.0
	set lastSample_ $now

	set qBytes_ $meanBytesQ
 	set qPkts_ $meanPktsQ
	if { ($qBytes_ * $qPkts_ == 0) && ($qBytes_ || $qPkts_) } {
		puts "inconsistent: bytes: $qBytes_ pkts: $qPkts_"
	}
	return "$meanBytesQ $meanPktsQ"
}

SimpleLink instproc sample-queue-size-old {} {
        $self instvar qMonitor_ lastSample_
        global ns
        set now [$ns now]
        $qMonitor_ newpoint $now [$qMonitor_ set lasty_]
        set sum [$qMonitor_ set sum_]
        #XXX
        if ![info exists lastSample_] {
                set lastSample_ 0
        }
        set dur [expr $now - $lastSample_]
        if { $dur != 0 } {
                set meanq [expr $sum / $dur]
        } else {
                set meanq 0
        }
        $qMonitor_ set sum_ 0.0
        set lastSample_ $now

        return $meanq
}


set queueSampleInterval 1.

# This routine is called every $queueSampleInterval seconds
# to update our estimator of the queue size on this link.
#
#
# The default output file is "queue.dat" for the queue lengths
# Format of file is:
#        <time> (node_i,node_j) <size_in_bytes> <num_packets>


set qfile_ [open "queue.dat" w]
SimpleLink instproc queue-sample-timeout { } {
	global ns queueSampleInterval
	global qfile_ 
	
	$self instvar qBytesEstimate_
	$self instvar qPktsEstimate_

	set qlist [$self sample-queue-size]
	set qBytes_ [lindex $qlist 0]
	set qPkts_ [lindex $qlist 1]

# We don't really want an EWMA here since the individual samples are 
# themselves integrated (averaged) estimates over $queueSampleInterval seconds.

#	set qBytesEstimate_ [expr 0.9 * $qBytesEstimate_ + 0.1 * $qBytes_]
#	set qPktsEstimate_ [expr 0.9 * $qPktsEstimate_ + 0.1 * $qPkts_]

	set qBytesEstimate_ $qBytes_
	set qPktsEstimate_ $qPkts_

	$self instvar fromNode_ toNode_
 	puts $qfile_ [format "%6.3f n%d:n%d %8.2f %6.2f" [$ns now] [$fromNode_ id] [$toNode_ id] $qBytesEstimate_ $qPktsEstimate_]
	if { ($qBytes_ * $qPkts_ == 0) && ($qBytes_ || $qPkts_) } {
		puts "inconsistent: bytes: $qBytes_ pkts: $qPkts_"
	}
	$ns at [expr [$ns now] + $queueSampleInterval] \
		"$self queue-sample-timeout"
}

# This is useful if you're experimenting with asymmetric networks and
# connections and tracing them.

Simulator instproc trace-simplex-link { n1 n2 bw delay type } {
	$self simplex-link $n1 $n2 $bw $delay $type
	$self instvar traceAllFile_
	if [info exists traceAllFile_] {
		$self trace-queue $n1 $n2 $traceAllFile_
	}
#	puts "Delay: $delay"
}

# This returns the link that connects a node and its specified neighbour
# XXX This really wants to be in ns-node.tcl

Node instproc get-link neighbor {
        global ns
        return [$ns set link_([$self id]:[$neighbor id])]
}

#
# OTcl methods for the Agent base class
#

Agent instproc port {} {
	$self instvar portID_
	return $portID_
}

#
# Lower 8 bits of dst_ are portID_.  this proc supports setting the interval
# for delayed acks
#       
Agent instproc dst-port {} {
	$self instvar dst_
	return [expr $dst_%256]
}

#
# Add source of type s_type to agent and return the source
#
Agent instproc attach-source {s_type} {
	set source [new Source/$s_type]
	$source set agent_ $self
	$self set type_ $s_type
	return $source
}

#
# OTcl support for classes derived from Agent
#
Class Agent/Null -superclass Agent

Agent/Null instproc init args {
    eval $self next $args
}

Agent/LossMonitor instproc log-loss {} {
}

Agent/TCPSimple instproc opencwnd {} {
	$self instvar cwnd_
	set cwnd_ [expr $cwnd_ + 1.0 / $cwnd_]
}

Agent/TCPSimple instproc closecwnd {} {
	$self instvar cwnd_
	set cwnd_ [expr 0.5 * $cwnd_]
}


#
# A simple timer class.  You can derive a subclass of Timer
# to provide a simple mechanism for scheduling events:
#
# 	$self sched $delay -- causes "$self timeout" to be called
#				$delay seconds in the future
#	$self cancel	   -- cancels any pending scheduled callback
# 
Class Timer

Timer instproc init ns {
	$self set ns_ $ns
	$self next
}

# sched is the same as resched; the previous setting is cancelled
# and another event is scheduled. No state is kept for the timers.
# This is different than the C++ timer API in timer-handler.cc,h; where a 
# sched aborts if the timer is already set. C++ timers maintain state 
# (e.g. IDLE, PENDING..etc) that is checked before the timer is scheduled.
Timer instproc sched delay {
	global ns
	$self instvar ns_ id_
	$self cancel
	set id_ [$ns_ after $delay "$self timeout"]
}

Timer instproc destroy {} {
	$self cancel
}

Timer instproc cancel {} {
	$self instvar ns_ id_
	if [info exists id_] {
		$ns_ cancel $id_
		unset id_
	}
}

# resched and expire are added to have a similar API to C++ timers.
Timer instproc resched delay {
	$self sched $delay 
}

# the subclass must provide the timeout function
Timer instproc expire {} {
	$self timeout
}

Timer instproc timeout {} {
	# NOTHING.  The subclass must provide the timeout function
}
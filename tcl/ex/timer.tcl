#
# A simple timer class.  You can derive a subclass of Timer
# to provide a simple mechanism for scheduling events:
#
# 	$self sched $delay -- causes "$self timeout" to be called
#				$delay seconds in the future
#	$self cancel	   -- cancels any pending scheduled callback
# 
Class Timer

Timer instproc sched delay {
	global ns
	$self instvar id_
	$self cancel
	set id_ [$ns at [expr [$ns now] + $delay] "$self timeout"]
}

Timer instproc destroy {} {
	$self cancel
}

Timer instproc cancel {} {
	global ns
	$self instvar id_
	if [info exists id_] {
		$ns cancel $id_
		unset id_
	}
}

# This test suite is for validating scheduler simultaneous event ordering in ns
#
# To run all tests:  test-all-scheduler
#
# To run individual tests:
# ns test-suite-scheduler.tcl List
# ns test-suite-scheduler.tcl Calendar
# ns test-suite scheduler.tcl Heap
#
# To view a list of available tests to run with this script:
# ns test-suite-scheduler.tcl
#


# What does this simple test do?
#   - it schedules $TIMES batches of events.  Each batch contains $SIMUL events, 
#     all of which will occur at the same time.  All events are permuted and
#     scheduled in a random order.  The output should be a list of integers 
#     from 1 to ($TIMES*$SIMUL) in increasing order.
Class TestSuite

TestSuite instproc init {} {
	$self instvar ns_ rng_
	set ns_ [new Simulator]
	set rng_ [new RNG]
}

TestSuite instproc run {} {
	$self instvar ns_ rng_
	set TIMES 20  ;# $TIMES different times for events
	set SIMUL 50  ;# each occurs $SIMUL times
	set TIMEMIN 0 ;# random events are taken from [TIMEMIN, TIMEMAX]
	set TIMEMAX 5

	set f [open "temp.rands" w]

	# generate random event timings and put them in increasing order by time to occur
	for {set i 0 } { $i < $TIMES } { incr i } {
		lappend timings [list [$rng_ uniform $TIMEMIN $TIMEMAX] $i $SIMUL]
	}
	set stimings [lsort -command "comp" $timings]
	for {set i 0 } { $i < $TIMES } { incr i } {
		set e [lindex $stimings $i]
		set idx [lsearch $timings $e]
		set timings [lreplace $timings $idx $idx [lappend e $i]]
	}

	while 1 {
		# pull out timings in random order
		set i [expr int([$rng_ uniform 0 [llength $timings]])]
		set e [lindex $timings $i]
		
		set order [lindex $e 3]
		set left  [lindex $e 2]
		set label [expr $SIMUL - $left + 1 + $order*$SIMUL]
		# puts "schedule at [lindex $e 0] -> $label" 
		$ns_ at [lindex $e 0] "puts $f $label"

		incr left -1
		if {$left==0} {
			set timings [lreplace $timings $i $i]
			if {$timings == ""} break
		} else {
			set e [lreplace $e 2 2 $left]
			set timings [lreplace $timings $i $i $e]
		}
	}
	$ns_ at $TIMEMAX "exit 0"
	$ns_ run
}

proc comp { a b} {
	set a1 [lindex $a 0]
	set b1 [lindex $b 0]
	if {$a1 > $b1} {
		return 1
	}
	return 0
}

proc outp {f arg} {
	puts $f $arg
}


# List
Class Test/List -superclass TestSuite
Test/List instproc init {} {
	$self instvar ns_
	$self next
	$ns_ use-scheduler List
}
# Calendar
Class Test/Calendar -superclass TestSuite
Test/Calendar instproc init {} {
	$self instvar ns_
	$self next
	$ns_ use-scheduler Calendar
}
# Heap
Class Test/Heap -superclass TestSuite
Test/Heap instproc init {} {
	$self instvar ns_
	$self next
	$ns_ use-scheduler Heap
}

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
	puts stderr "Valid tests are:\t[get-subclasses TestSuite Test/]"
	exit 1
}

proc isProc? {cls prc} {
	if [catch "Object info subclass $cls/$prc" r] {
		global argv0
		puts stderr "$argv0: no such $cls: $prc"
		usage
	}
}

proc get-subclasses {cls pfx} {
	set ret ""
	set l [string length $pfx]

	set c $cls
	while {[llength $c] > 0} {
		set t [lindex $c 0]
		set c [lrange $c 1 end]
		if [string match ${pfx}* $t] {
			lappend ret [string range $t $l end]
		}
		eval lappend c [$t info subclass]
	}
	set ret
}
TestSuite proc runTest {} {
	global argc argv
	if { $argc==1 || $argc==2 } {
		set test [lindex $argv 0]
		isProc? Test $test
	} else {
		usage
	}

	set t [new Test/$test]
	$t run
}

TestSuite runTest

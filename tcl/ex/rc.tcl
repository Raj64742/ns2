#
# A simple (but not too realistic) rate-based congestion control
# scheme for Homework 3 in CS268.
#
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/ex/rc.tcl,v 1.2 1997/03/28 08:52:33 mccanne Exp $
#

source timer.tcl

set ns [new Simulator]

proc build_topology { ns which } {
	foreach i "0 1 2 3" {
		global n$i
		set n$i [$ns node]
	}
	$ns duplex-link $n0 $n2 5Mb 2ms DropTail
	$ns duplex-link $n1 $n2 5Mb 2ms DropTail
	if { $which == "FIFO" } {
		$ns duplex-link $n2 $n3 1.5Mb 10ms DropTail
	} elseif { $which == "RED" } {
		$ns duplex-link $n2 $n3 1.5Mb 10ms RED
	} else {
		$ns duplex-link $n2 $n3 1.5Mb 10ms FQ
	}
}

Class Agent/Message/Sender -superclass {Agent/Message Timer}
Class Agent/Message/Receiver -superclass Agent/Message

Agent/Message/Sender instproc init {} {
	$self next
	$self instvar cbr_ seq_
	set cbr_ [new Agent/CBR]
	set seq_ 1
	$self sched [$self randomize 0.05]
}

Agent/Message/Sender instproc randomize v {
	return [expr $v * (1.0 + [ns-random] / 2147483647.)]
}

Agent/Message/Sender instproc timeout {} {
	global ns
	$self send "probe [$ns now]"
	$self sched [$self randomize 0.05]
}

Agent/Message/Sender instproc handle msg {
	set type [lindex $msg 0]
	if { $type == "congested" } {
		$self decrease
	} else {
		$self increase
	}
}

Agent/Message/Sender instproc increase {} {
	$self instvar cbr_
	set delta [$cbr_ set interval_]
	set delta [expr 1.0 / (1.0 / $delta + 100)]
	$cbr_ set interval_ $delta
}

Agent/Message/Sender instproc decrease {} {
	$self instvar cbr_
	set delta [$cbr_ set interval_]
	set delta [expr 2 * $delta]
	$cbr_ set interval_ $delta
}

Agent/Message/Receiver instproc init {} {
	$self next
	$self instvar mon_ congested_
	set mon_ [new Agent/LossMonitor]
	$mon_ proc log-loss {} "$self log-loss"
	set congested_ 0
}

Agent/Message/Receiver instproc log-loss {} {
	global ns
	$self instvar congested_
	if !$congested_ {
		$self send "congested 0"
		set congested_ 1
	}
}

Agent/Message/Receiver instproc handle msg {
	$self instvar congested_
	if $congested_ {
		set congested_ 0
	} else {
		$self send "uncongested [lindex $msg 1]"
	}
}

proc build_conn { from to startTime } {
	global ns
	set src [new Agent/Message/Sender]
	set sink [new Agent/Message/Receiver]
	$ns attach-agent $from $src
	$ns attach-agent $to $sink
	$ns connect $src $sink
	$ns attach-agent $from [$src set cbr_]
	$ns attach-agent $to [$sink set mon_]
	$ns connect [$src set cbr_] [$sink set mon_]
	$ns at $startTime "[$src set cbr_] start"
	return [$src set cbr_]
}

set f [open out.tr w]
$ns trace-all $f

build_topology $ns FIFO

set c1 [build_conn $n0 $n3 0.1]
$c1 set class_ 1
set c2 [build_conn $n1 $n3 0.1]
$c2 set class_ 2

$ns at 5.0 "finish"

proc finish {} {
	global ns f
	$ns flush-trace
	close $f

	puts "converting output to nam format..."
	exec awk -f ../nam-demo/nstonam.awk out.tr > rc-nam.tr 
	exec rm -f out
	#XXX
	puts "running nam..."
	exec nam rc-nam &
	exit 0
}

$ns run


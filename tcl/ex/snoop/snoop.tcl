#!/bin/sh
# \
[ -x ../../ns ] && nshome=../../
# \
exec ${nshome}ns "$0" "$@"

set nshome ""
if [file executable ../../ns] {
	set nshome ../../
}

source ${nshome}tcl/lan/ns-mac.tcl
source ${nshome}tcl/lan/ns-lan.tcl
set env(PATH) "${nshome}bin:$env(PATH)"

set opt(tr)	out
set opt(stop)	20
set opt(num)	3
set opt(seed)	0
set tcptrace    [open tcp.out w]

set opt(bw)	2Mb
set opt(delay)	3ms
set opt(ll)	LL/Snoop
set opt(ifq)	Queue/DropTail
set opt(mac)	Mac/Csma/Ca
set opt(chan)	Channel/Duplex
set opt(tp)	TCP/Reno
set opt(sink)	TCPSink
set opt(source)	FTP
set opt(cbr)	0
set errorrate   0.1

LL/Snoop set bandwidth_ 1Mb
LL/Snoop set delay_ 3ms
LL/Snoop set snoopDisable_ 0
LL/Snoop set srtt_ 50ms
LL/Snoop set rttvar_ 100ms

if {$argc == 0} {
	puts "Usage: $argv0 \[-stop sec\] \[-seed value\] \[-num nodes\]"
	puts "\t\[-tr tracefile\] \[-g\]"
	puts "\t\[-ll lltype\] \[-ifq ifqtype\] \[-mac mactype\] \[-chan chan\]"
	puts "\t\[-bw $opt(bw)] \[-delay $opt(delay) \[-e\]"
	exit 1
}

proc getopt {argc argv} {
	global opt
	lappend optlist tr stop num seed
	lappend optlist bw delay ll ifq mac chan tp sink source cbr

	for {set i 0} {$i < $argc} {incr i} {
		set arg [lindex $argv $i]
		if {[string range $arg 0 0] != "-"} continue

		set name [string range $arg 1 end]
		if {$name == "ll"} {
			set opt(ll) LL/[lindex $argv [incr i]]
		} elseif {$name == "ifq"} {
			set opt(ifq) Queue/[lindex $argv [incr i]]
		} elseif {$name == "mac"} {
			set opt(mac) Mac/[lindex $argv [incr i]]
		} elseif {[lsearch $optlist $name] >= 0} {
			set opt($name) [lindex $argv [incr i]]
		} else {
			set opt($name) [lindex $argv [expr $i+1]]
		}
	}
}

proc cat {filename} {
	set fd [open $filename r]
	while {[gets $fd line] >= 0} {
		puts $line
	}
	close $fd
}

proc finish {} {
	global env nshome
	global ns opt trfd tcptrace

	$ns flush-trace
	close $trfd
	flush $tcptrace
	close $tcptrace

	exec perl ${nshome}bin/trsplit -f -tt r -pt tcp -c "$opt(num) $opt(bw) $opt(delay) $opt(ll) $opt(ifq) $opt(mac) $opt(chan) $opt(seed)" $opt(tr) 2>$opt(tr)-bwt >> $opt(tr)-bw
	exec cat $opt(tr)-bwt >> $opt(tr)-bw
	cat $opt(tr)-bwt
	exec rm $opt(tr)-bwt

	if [info exists opt(g)] {
		eval exec xgraph -nl -M -display $env(DISPLAY) \
				[lsort [glob $opt(tr).*]]
	}
	exit
}


proc create-trace {trfile} {
	global trfd
	if [file exists $trfile] {
		exec touch $trfile.
		eval exec rm $trfile [glob $trfile.*]
	}
	set trfd [open $trfile w]
}

proc trace-mac {lan trfd} {
	set channel [$lan set channel_]
	set trHop [new TraceIp/Mac/Hop]
	$trHop attach $trfd
	$trHop target $channel
	$channel trace-target $trHop
}

proc create-topology {num} {
	global ns opt
	global lan node s

	# nodes on wireless LAN; $node(0) is base station
	for {set i 0} {$i <= $num} {incr i} {
		set node($i) [$ns node]
		lappend nodelist $node($i)
	}
	Queue set limit_ 50
	set lan [$ns make-lan $nodelist $opt(bw) $opt(delay) \
			$opt(ll) $opt(ifq) $opt(mac) $opt(chan)]
#	puts "WLAN: $lan $opt(bw) $opt(delay) $opt(ll) $opt(ifq) $opt(mac) $opt(chan)"

	set s(0) [$ns node]
	$ns duplex-link $s(0) $node(0) 10Mb 1ms DropTail
	
}

proc create-source {num} {
	global ns opt
	global lan node s source tcptrace

	for {set i 1} {$i <= $num} {incr i} {
		set tp($i) [$ns create-connection $opt(tp) \
				$s(0) $opt(sink) $node($i) 0]
		set source($i) [$tp($i) attach-source $opt(source)]
		$ns at [expr $i/1000.0] "$source($i) start"
		$tp($i) trace $tcptrace
	}

	for {set i 0} {$i < $opt(cbr)} {incr i} {
		set dst [expr $i % $num + 1]
		set cbr($i) [$ns create-connection CBR \
				$s(0) Null $node($dst) 0]
		$ns at 0 "$cbr($i) start"
	}
}


## MAIN ##
getopt $argc $argv
if {$opt(seed) > 0} {
	ns-random $opt(seed)
}
set ns [new Simulator]

create-trace $opt(tr)
$ns trace-all $trfd

create-topology $opt(num)
$lan trace $ns $trfd

if [info exists opt(tracemac)] { trace-mac $lan $trfd }

if [info exists opt(e)] { 
	source ber.tcl
	create-error $opt(num) $errorrate
}

create-source $opt(num)

$ns at $opt(stop) "finish"
$ns run

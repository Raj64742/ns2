#!/bin/sh
# the next line finds ns \
nshome=`dirname $0`; [ ! -x $nshome/ns ] && [ -x ../../ns ] && nshome=../..
# the next line starts ns \
export nshome; exec $nshome/ns "$0" "$@"

if [info exists env(nshome)] {
	set nshome $env(nshome)
} elseif [file executable ../../ns] {
	set nshome ../..
} elseif {[file executable ./ns] || [file executable ./ns.exe]} {
	set nshome "[pwd]"
} else {
	puts "$argv0 cannot find ns directory"
	exit 1
}
set env(PATH) "$nshome/bin:$env(PATH)"
source $nshome/tcl/http/http.tcl

set opt(trsplit) "-"
set opt(tr)	out
set opt(stop)	20
set opt(node)	3
set opt(seed)	0

set opt(bw)	1.8Mb
set opt(delay)	1ms
set opt(ll)	LL
set opt(ifq)	Queue/DropTail
set opt(mac)	Mac/Csma/Cd
set opt(chan)	Channel
set opt(tcp)	TCP/Reno
set opt(sink)	TCPSink

# number of traffic source (-1 means the number of nodes $opt(node))
set opt(telnet) 0
set opt(cbr)	0
set opt(ftp)	0
set opt(http)	-1

# set opt(webModel) ""
set opt(webModel) $nshome/tcl/http/data
set opt(phttp) 0

if {$argc == 0} {
	puts "Usage: $argv0 \[-stop sec\] \[-seed value\] \[-node numNodes\]"
	puts "\t\[-tr tracefile\] \[-g\]"
	puts "\t\[-ll lltype\] \[-ifq qtype\] \[-mac mactype\]"
	puts "\t\[-bw $opt(bw)] \[-delay $opt(delay)\]"
	exit 1
}

proc getopt {argc argv} {
	global opt
	for {set i 0} {$i < $argc} {incr i} {
		set key [lindex $argv $i]
		if ![string match {-*} $key] continue
		set key [string range $key 1 end]
		set val [lindex $argv [incr i]]
		set opt($key) $val
		if [string match {-[A-z]*} $val] {
			decr i
			continue
		}
		switch $key {
			ll  { set opt($key) LL/$val }
			ifq { set opt($key) Queue/$val }
			mac { set opt($key) Mac/$val }
		}
	}
}

proc swap {upvar1 upvar2} {
	upvar $upvar1 var1 $upvar2 var2
	set temp $var1
	set var1 $var2
	set var2 $temp
}

proc cat {filename} {
	set fd [open $filename r]
	while {[gets $fd line] >= 0} {
		puts $line
	}
	close $fd
}


proc finish {} {
	global env nshome pwd
	global ns opt trfd

	$ns flush-trace
	close $trfd
	foreach key {node bw delay ll ifq mac seed} {
		lappend comment $opt($key)
	}
	exec perl $nshome/bin/trsplit -tt r -pt tcp -c "$comment" \
			$opt(trsplit) $opt(tr) 2>$opt(tr)-bwt > $opt(tr)-bw
	cat $opt(tr)-bwt
	exec cat $opt(tr)-bwt >> $opt(tr)-bw
	exec rm $opt(tr)-bwt

	if [info exists opt(g)] {
		eval exec xgraph -nl -M -display $env(DISPLAY) \
				[lsort [glob $opt(tr).*]]
	}
	exit 0
}


proc create-trace {trfile} {
	if [file exists $trfile] {
		exec touch $trfile.
		eval exec rm -f $trfile ${trfile}-bw [glob $trfile.*]
	}
	return [open $trfile w]
}


proc create-topology {} {
	global ns opt
	global lan node source

	# Nodes:
	#  0		a router from LAN to external network
	#  1 to n	nodes on the LAN
	#  n+1		node outside of the LAN,
	set num $opt(node)
	set node(0)  [$ns node]
	for {set i 1} {$i <= $num} {incr i} {
		set node($i) [$ns node]
		lappend nodelist $node($i)
	}
	set node($i) [$ns node]
	$ns duplex-link $node(0) $node($i) 10Mb 50ms DropTail

	set lan [$ns newLan $nodelist $opt(bw) $opt(delay) -llType $opt(ll) \
			-ifqType $opt(ifq) -macType $opt(mac)]
	$lan addNode $node(0) $opt(bw) $opt(delay)

	set ifq0 [[$lan netIface $node(0)] set ifq_]
	$ifq0 set limit_ [expr $opt(node) * [$ifq0 set limit_]]
}


proc create-source {} {
	global ns opt
	global lan node source

	# make node($num+1) the source of all connection
	set src $node(0)
	foreach ttype {http ftp cbr telnet} {
		set num $opt($ttype)
		if {$num < 0} {
			set num $opt(node)
		}
		for {set i 0} {$i < $num} {incr i} {
			set dst $node([expr 1 + $i % $opt(node)])
			new_$ttype $i $src $dst
		}
	}
}

proc newWebModel dir {
	set rv [new RandomVariable/Empirical]
	puts "$rv loadCDF $dir/HttpConnections.cdf"
	$rv loadCDF $dir/HttpConnections.cdf
	lappend webm -rvNumImg $rv

	set rv [new RandomVariable/Empirical]
	puts "$rv loadCDF $dir/HttpReplyLength.cdf"
	$rv loadCDF $dir/HttpReplyLength.cdf
	lappend webm -rvRepLen $rv
	lappend webm -rvImgLen $rv

	set rv [new RandomVariable/Empirical]
	puts "$rv loadCDF $dir/HttpRequestLength.cdf"
	$rv loadCDF $dir/HttpRequestLength.cdf
	lappend webm -rvReqLen $rv

	set rv [new RandomVariable/Empirical]
	puts "$rv loadCDF $dir/HttpThinkTime.cdf"
	$rv loadCDF $dir/HttpThinkTime.cdf
	lappend webm -rvClientTime $rv
}

proc new_http {i server client} {
	global ns opt http webm
	set webopt "-srcType $opt(tcp) -snkType $opt(sink) -phttp $opt(phttp)"
	if {$opt(webModel) != ""} {
		if ![info exists webm] {
			set webm [newWebModel $opt(webModel)]
		}
		set webopt "$webopt $webm"
	}
	set http($i) [eval new Http $ns $client $server $webopt]
	$ns at [expr 0 + $i/1000.0] "$http($i) start"
}

proc new_cbr {i src dst} {
	global ns opt cbr
	set cbr($i) [$ns create-connection CBR $src Null $dst 0]
	$ns at [expr 0.0001 + $i/1000.0] "$cbr($i) start"
}

proc new_ftp {i src dst} {
	global ns opt ftp
	set tcp [$ns create-connection $opt(tcp) $src $opt(sink) $dst 0]
	set ftp($i) [$tcp attach-source FTP]
	$ns at [expr 0.0002 + $i/1000.0] "$ftp($i) start"
}

proc new_telnet {i src dst} {
	global ns opt telnet
	set tcp [$ns create-connection $opt(tcp) $src $opt(sink) $dst 0]
	set telnet($i) [$tcp attach-source Telnet]
	$ns at [expr 0.0003 + $i/1000.0] "$telnet($i) start"
}


## MAIN ##
getopt $argc $argv
if {[info exists opt(f)] || [info exists opt(g)]} {
	set opt(trsplit) "-f"
}
if {$opt(seed) > 0} {
	ns-random $opt(seed)
}
if [info exists opt(qsize)] {
	Queue set limit_ $opt(qsize)
}

set ns [new Simulator]

set trfd [create-trace $opt(tr)]
$ns trace-all $trfd

create-topology
create-source

$lan trace $ns $trfd

$ns at $opt(stop) "finish"
$ns run

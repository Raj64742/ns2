#!/bin/sh
# \
test -x ../../../ns && nshome=../../../
# \
exec ${nshome}ns "$0" "$@"

set nshome ""
if [file executable ../../../ns] {
	set nshome ../../../
}

# Test script for the snoop protocol (and other link protocols).

# Example ways of running this script are:
# ns snoop.tcl -e 0.2 -eu time -stop 20 -ll Snoop -r
# ns snoop.tcl -e 32768 -eu byte -stop 20 -ll Snoop -r
# ns snoop.tcl -e 100 -eu pkt -stop 20 -ll Snoop -r

# These are for the time-, byte- and packet-based error models.  
# In the first case, 0.2 is the mean number for seconds between errors,
# in the second case 32768 is the mean # of bytes, and in the third
# case 100 is the mean number of packets between errors.
# All errors are exponentially distributed according to the specified rates.
# This uses the error model support in ns-ber.tcl

#
# Copyright (c) 1996 Regents of the University of California.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
# 	This product includes software developed by the Daedalus Research
# 	Group at the University of California Berkeley.
# 4. Neither the name of the University nor of the Research Group may be
#    used to endorse or promote products derived from this software without
#    specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

source ${nshome}tcl/lan/ns-mac.tcl
source ${nshome}tcl/lan/ns-lan.tcl
source ${nshome}tcl/lib/ns-errmodel.tcl
source ${nshome}tcl/ex/snoop/util.tcl

set env(PATH) "${nshome}bin:$env(PATH)"

set opt(tr)	out
set opt(ltr)    lan
set opt(stop)	20
set opt(num)	1
set tcptrace    [open tcp.tr w]

set opt(seed)   0
set opt(bw)	2Mb
set opt(delay)	3ms
set opt(qsize)  20
set opt(ll)	LL
set opt(ifq)	Queue/DropTail
set opt(mac)	Mac/Csma/Ca
set opt(chan)	Channel
set opt(tp)	TCP/Reno
set opt(sink)	TCPSink
set opt(source)	FTP
set opt(cbr)	0
set opt(em)     expo
set opt(eu)     time
set opt(trans)  ""

LL/Snoop set snoopTick_ [expr [Agent/TCP set tcpTick_]/5]
LL/Snoop set bandwidth_ 2Mb
LL/Snoop set delay_ 0
LL/Snoop set snoopDisable_ 0
LL/Snoop set srtt_ 50ms
LL/Snoop set rttvar_ 100ms

if {$argc == 0} {
	puts "Usage: $argv0 \[-stop sec\] \[-r value\] \[-num nodes\]"
	puts "\t\[-tr tracefile\] \[-g\]"
	puts "\t\[-ll lltype\] \[-ifq ifqtype\] \[-mac mactype\] \[-chan chan\]"
	puts "\t\[-bw $opt(bw)] \[-delay $opt(delay) \[-e\]"
	exit 1
}

proc getopt {argc argv} {
	global opt
	lappend optlist tr stop num r
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
		} elseif {$name == "r"} {
			set opt(r) 1
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

#	exec perl ${nshome}bin/trsplit -f -tt r -pt tcp -c "$opt(num) $opt(bw) $opt(delay) $opt(ll) $opt(ifq) $opt(mac) $opt(chan)" $opt(tr) 2>$opt(tr)-bwt >> $opt(tr)-bw
#	exec cat $opt(tr)-bwt >> $opt(tr)-bw
#	cat $opt(tr)-bwt
#	exec rm $opt(tr)-bwt

	if [info exists opt(g)] {
		eval exec xgraph -nl -M -display $env(DISPLAY) \
				[lsort [glob $opt(tr).*]]
	}
	exit
}


proc create-trace {trfile} {
	# ltrfile is the trace of the LAN events
	# need LAN id?
	global trfd ltrfd
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

# Topology
# s(0) --------- node(0) - - - - node(1)
# Snoop runs at node(0).  Make sure opt(ll) is set to LL/Snoop
#
proc create-topology {num} {
	global ns opt
	global lan node s

	# nodes on wireless LAN; $node(0) is base station
	for {set i 0} {$i <= $num} {incr i} {
		set node($i) [$ns node]
		lappend nodelist $node($i)
	}
	Queue set limit_ $opt(qsize)
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
		setupTcpTracing $tp($i) $tcptrace
		$tp($i) set tcpTick_ 0.5
		LL/Snoop set snoopTick_ 0.1
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
if { [info exists opt(r)] } {
	ns-random $opt(seed)
	$defaultRNG seed 0
}

set ns [new Simulator]

create-trace $opt(tr)
$ns trace-all $trfd

create-topology $opt(num)

$lan trace $ns $trfd

if [info exists opt(tracemac)] { trace-mac $lan $ltrfd }

if { [info exists opt(e)] } {
	for {set i 1} {$i <= $opt(num)} {incr i} {
		lappend dstlist $node($i)
	}
	$lan create-error $node(0) $dstlist $opt(em) $opt(e) $opt(eu) \
			$opt(trans)
}

create-source $opt(num)

$ns at $opt(stop) "finish"
$ns run

#!/bin/sh
# the next line finds ns \
nshome=../../..
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

# Test script for the snoop protocol (and other link protocols).

# Example ways of running this script are:
# ns snoop.tcl -e 0.2 -eu time -stop 20 -ll LLSnoop -r
# ns snoop.tcl -e 32768 -eu byte -stop 20 -ll LLSnoop -r
# ns snoop.tcl -e 100 -eu pkt -stop 20 -ll LLSnoop -r

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

source $nshome/tcl/lan/ns-mac.tcl
#source $nshome/tcl/lan/ns-lan.tcl
source $nshome/tcl/lan/ns-ll.tcl
source $nshome/tcl/lib/ns-errmodel.tcl
source $nshome/tcl/lib/ns-trace.tcl
source $nshome/tcl/ex/snoop/util.tcl
source $nshome/tcl/http/http.tcl
source $nshome/tcl/lan/vlan.tcl

set env(PATH) "${nshome}bin:$env(PATH)"

set opt(tr)	out
set opt(ltr)    lan
set opt(stop)	20
set opt(num)	1
set opt(nsrc)	1
set tcptrace    [open tcp.tr w]

set opt(seed)   0
set opt(ll)	LL
set opt(ifq)	Queue/DropTail
set opt(qtype)	DropTail
set opt(mac)	Mac/Csma/Ca
set opt(chan)	Channel

set opt(tp)	TCP/Reno
set opt(sink)	TCPSink
set opt(source)	FTP
set opt(cbr)	0
set opt(em)     expo
set opt(eu)     time
set opt(trans)  ""

set opt(nsrv)  1
set opt(ncli)  1
set opt(ncbr)  0
set opt(ngw)   2

set opt(bw)    2Mb
set opt(delay) 3ms
set opt(ibw)    10Mb
set opt(idelay) 1ms

set opt(qsize) 20
set opt(win)   8

set opt(webmodel) $nshome/tcl/http/data
set opt(phttp) 0

Snoop set snoopTick_ [expr [Agent/TCP set tcpTick_]/5]
Snoop set bandwidth_ 2Mb
Snoop set delay_ 0
Snoop set snoopDisable_ 0
Snoop set srtt_ 50ms
Snoop set rttvar_ 100ms

if {$argc == 0} {
	puts "Usage: $argv0 \[-stop sec\] \[-r value\] \[-num nodes\]"
	puts "\t\[-tr tracefile\] \[-g\]"
	puts "\t\[-ll lltype\] \[-ifq ifqtype\] \[-mac mactype\] \[-chan chan\]"
	puts "\t\[-bw $opt(bw)] \[-delay $opt(delay) \[-e\]"
	exit 1
}

proc getopt {argc argv} {
	global opt
	lappend optlist tr stop num
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

proc finish {} {
	global env nshome
	global ns opt trfd tcptrace 

	$ns flush-trace
	close $trfd
	flush $tcptrace
	close $tcptrace

	if [info exists opt(g)] {
		eval exec xgraph -nl -M -display $env(DISPLAY) \
				[lsort [glob $opt(tr)*]]
	}
	exit
}


proc create-trace {trfile} {
	# ltrfile is the trace of the LAN events
	# need LAN id?
	global trfd ltrfd
	if [file exists $trfile] {
#		exec touch $trfile.
#		eval exec rm $trfile [glob $trfile.*]
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

proc create-error { lan src dstlist emname rate unit {trans ""}} {
	if { $trans == "" } {
		set trans [list 0.5 0.5]
	}

	# default is exponential errmodel
	if { $emname == "uniform" } {
		set e1 [new ErrorModel/Uniform $rate $unit]
		set e2 [new ErrorModel/Uniform $rate $unit]
	} elseif { $emname == "2state" } {
		set e1 [new ErrorModel/TwoStateMarkov $rate $unit $trans]
		set e2 [new ErrorModel/TwoStateMarkov $rate $unit $trans]
	} elseif { $emname == "emp" } {
		# rate0-3 are actually filenames here!
		set e1 [new ErrorModel/Empirical]
		$e1 initrv [list [lindex $rate 0] [lindex $rate 1]]
		set e2 [new ErrorModel/Empirical]
		$e2 initrv [list [lindex $rate 2] [lindex $rate 3]]
	} else {
		set e1 [new ErrorModel/Expo $rate $unit]
		set e2 [new ErrorModel/Expo $rate $unit]
	}

	foreach dst $dstlist {
		add-error $lan $src $dst $e1
		add-error $lan $dst $src $e2
	}
}

proc add-error {lan src dst errmodel} {
	$lan instvar lanIface_

	set nif $lanIface_([$dst id])
	set filter [new Filter/Field]
	$nif add-receive-filter $filter
	$filter filter-target $errmodel
	$errmodel target [$filter target]
	$filter set offset_ [PktHdr_offset PacketHeader/Mac macSA_]
	$filter set match_ [$src node-addr] # will not work with hier. routing
}

# Topology
# s(0) --------- node(0) - - - - node(1)
# Snoop runs at node(0).  Make sure opt(ll) is set to LL/LLSnoop
#
proc create-topology {num} {
	global ns opt
	global lan node s

	# nodes on wireless LAN; $node(0) is base station
	for {set i 0} {$i <= $num} {incr i} {
		set node($i) [$ns node]
		if { $i > 0 } {
			lappend nodelist $node($i)
		}
	}
	Queue set limit_ $opt(qsize)

	set lan [$ns make-lan $nodelist $opt(bw) $opt(delay) \
			LL $opt(ifq) $opt(mac) $opt(chan)]
	$lan addNode [list $node(0)] $opt(bw) $opt(delay) $opt(ll) $opt(ifq) \
			$opt(mac)

	set s(0) [$ns node]
	$ns duplex-link $s(0) $node(0) $opt(ibw) $opt(idelay) DropTail
}

proc create-source {nsrc} {
	global ns opt
	global lan node s source tcptrace

	for {set i 1} {$i <= $nsrc} {incr i} {
		set tp($i) [$ns create-connection $opt(tp) \
				$s(0) $opt(sink) $node($i) 0]
		set source($i) [$tp($i) attach-source $opt(source)]
		$ns at [expr $i/1000.0] "$source($i) start"
		setupTcpTracing $tp($i) $tcptrace
		$tp($i) set tcpTick_ 0.5
		Snoop set snoopTick_ 0.1
		$tp($i) trace $tcptrace
	}

	for {set i 0} {$i < $opt(cbr)} {incr i} {
		set dst [expr $i % $nsrc + 1]
		set cbr($i) [$ns create-connection CBR \
				$s(0) Null $node($dst) 0]
		$ns at 0 "$cbr($i) start"
	}
}


proc create-www-topology { } {
	global ns opt c s cb lan lanlist gw

	Queue set limit_ $opt(qsize)

	# Set up Web servers
	for { set i 0 } { $i < $opt(nsrv) } { incr i } {
		set s($i) [$ns node]
	}

	# Set up Web clients
	for { set i 0 } { $i < $opt(ncli) } { incr i } {
		set c($i) [$ns node]
		lappend lanlist $c($i)
	}

	# Set up CBR cross-traffic sources
	for { set i 0 } { $i < $opt(ncbr) } { incr i } {
		set cb($i) [$ns node]
	}

	# Set up intermediate gateways and hook them up
	for { set i 0 } { $i < $opt(ngw) } { incr i } {
		set gw($i) [$ns node]
		if { $i > 0 } {
			$ns duplex-link $gw([expr $i-1]) $gw($i) \
					$opt(bw) $opt(delay) $opt(qtype)
		}
	}

	# Hook up sources to gateways
	for { set i 0 } { $i < $opt(nsrv) } { incr i } {
		$ns duplex-link $s($i) $gw(0) 10Mb 2ms $opt(qtype)
	}

	# Hook up clients to last gateway (base station) on wireless LAN
	for { set i 0 } { $i < $opt(ncli) } { incr i } {
		set lan [$ns make-lan $lanlist $opt(bw) $opt(delay) \
				LL $opt(ifq) $opt(mac) $opt(chan)]
		$lan addNode [list $gw([expr $opt(ngw) - 1])] \
				$opt(bw) $opt(delay) $opt(ll) $opt(ifq) \
				$opt(mac) LL
	}

	# Hook up CBR sources to gateways
	for { set i 0 } { $i < $opt(ncbr) } { incr i } {
		$ns duplex-link $cb($i) $gw(0) 10Mb 2ms $opt(qtype)
	}


}

proc create-webModel path {
	Http setCDF rvClientTime 0
#	Http setCDF rvClientTime $path/HttpThinkTime.cdf
	Http setCDF rvReqLen $path/HttpRequestLength.cdf
	Http setCDF rvNumImg $path/HttpConnections.cdf
	set rv [Http setCDF rvRepLen $path/HttpReplyLength.cdf]
	Http setCDF rvImgLen $rv
}

proc new_http {i server client} {
	global ns opt http webm
	set webopt "-srcType $opt(tp) -snkType $opt(sink) -phttp $opt(phttp)"
	set http($i) [eval new Http $ns $client $server $webopt]
	puts "i $i"
	$ns at [expr 0 + $i/1000.0] "$http($i) start"
}

proc create-www-source { } {
	global ns opt c s cb tcptrace

	create-webModel $opt(webmodel)

	for {set i 0} {$i < $opt(ncli)} {incr i} {
		# pick a random server to go to
		set srv [expr round([uniform 0 [expr $opt(nsrv)-1]])]
		new_http $i $s($srv) $c($i)
	}
	
	for { set i 0 } { $i < $opt(ncbr) } { incr i } {
		set cbr($i) [new Agent/CBR]
		$cbr($i) set interval_ $opt(cbr)
		$ns attach-agent $cb($i) $cbr($i)

		set null($i) [new Agent/Null]
		# pick a random sink for the CBR traffic
		set dst [expr round([uniform 0 [expr $opt(ncli)-1]])]
		$ns attach-agent $c($dst) $null($i)
		$ns connect $cbr($i) $null($i)
		# start cbr's at a time randomly chosen in [0,1]
		$ns at [uniform 0 1] "$cbr($i) start"
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

if { [info exists opt(www)] } {
	create-www-topology
} else {
	create-topology $opt(num)
}

#$lan trace $ns $trfd

if [info exists opt(tracemac)] { trace-mac $lan $ltrfd }

if { [info exists opt(e)] } {
	if { $opt(e) == 0 || $opt(e) == "" } {
		puts "error rate 0 ==> no errors"
	} else {
		if { [info exists opt(www)] } {
			create-error $lan $gw([expr $opt(ngw) - 1]) $lanlist \
					$opt(em) $opt(e) $opt(eu) $opt(trans)
		} else {
			for {set i 1} {$i <= $opt(num)} {incr i} {
				lappend dstlist $node($i)
			}
			create-error $lan $node(0) $dstlist $opt(em) $opt(e) \
					$opt(eu) $opt(trans)
		}
	}
}

if { [info exists opt(www)] } {
	create-www-source
} else {
	create-source $opt(nsrc)
}

$ns at $opt(stop) "finish"
$ns run

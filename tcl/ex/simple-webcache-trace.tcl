#
#  Copyright (c) 1997 by the University of Southern California
#  All rights reserved.
#
#  Permission to use, copy, modify, and distribute this software and its
#  documentation in source and binary forms for non-commercial purposes
#  and without fee is hereby granted, provided that the above copyright
#  notice appear in all copies and that both the copyright notice and
#  this permission notice appear in supporting documentation. and that
#  any documentation, advertising materials, and other materials related
#  to such distribution and use acknowledge that the software was
#  developed by the University of Southern California, Information
#  Sciences Institute.  The name of the University may not be used to
#  endorse or promote products derived from this software without
#  specific prior written permission.
#
#  THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
#  the suitability of this software for any purpose.  THIS SOFTWARE IS
#  PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
#  INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#  Other copyrights might apply to parts of this software and are so
#  noted when applicable.
# 
#  $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/ex/simple-webcache-trace.tcl,v 1.1 1998/12/16 21:30:17 haoboy Exp $

# Demo of simple trace-driven web sim

set ns [new Simulator]

# Create topology/routing
set node(c) [$ns node] 
set node(e) [$ns node]
set node(s) [$ns node]
$ns duplex-link $node(s) $node(e) 1.5Mb 50ms DropTail
$ns duplex-link $node(e) $node(c) 10Mb 2ms DropTail 
$ns rtproto Session

# HTTP logs
set log [open "http.log" w]

# Use PagePool/Proxy Trace
set pgp [new PagePool/ProxyTrace]
# Set trace files. There are two files; one for request stream, the other for 
# page information, e.g., size and id
#
# XXX Assuming current directory is ~ns/tcl/ex. Use traces under ~ns/tcl/test
$pgp set-reqfile "../test/webtrace-reqlog"
$pgp set-pagefile "../test/webtrace-pglog"
# Set number of clients that will use this page pool. It's used to assign
# requests to clients
$pgp set-client-num 1
# Set the ratio of hot pages in all pages. Because no page modification
# data is available in most traces, we assume a bimodal page age distribution
$pgp bimodal-ratio 0.1
# Dynamic (hot) page age generator
set tmp [new RandomVariable/Exponential] ;# Age generator
$tmp set avg_ 5 ;# average page age
$pgp ranvar-dp $tmp
# Static page age generator
set tmp [new RandomVariable/Constant]
$tmp set val_ 10000
$pgp ranvar-sp $tmp

set server [new Http/Server $ns $node(s)]
$server set-page-generator $pgp
$server log $log

set cache [new Http/Cache $ns $node(e)]
$cache log $log

set client [new Http/Client $ns $node(c)]
# XXX When trace-driven, don't assign a request interval generator
$client set-page-generator $pgp
$client log $log

set startTime 1 ;# simulation start time
set finishTime 50 ;# simulation end time
$ns at $startTime "start-connection"
$ns at $finishTime "finish"

proc start-connection {} {
	global ns server cache client
	$client connect $cache
	$cache connect $server
	$client start-session $cache $server
}

proc finish {} {
	global ns log
	$ns flush-trace
	flush $log
	close $log
	exit 0
}

$ns run

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
#  $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/ex/simple-webcache.tcl,v 1.1 1998/08/25 17:34:48 haoboy Exp $

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

# Use PagePool/Math
set pgp [new PagePool/Math]
set tmp [new RandomVariable/Constant] ;# Size generator
$tmp set val_ 1024  ;# average page size
$pgp ranvar-size $tmp
set tmp [new RandomVariable/Exponential] ;# Age generator
$tmp set avg_ 5 ;# average page age
$pgp ranvar-age $tmp

set server [new Http/Server $ns $node(s)]
$server set-page-generator $pgp
$server log $log

set cache [new Http/Cache $ns $node(e)]
$cache log $log

set client [new Http/Client $ns $node(c)]
set tmp [new RandomVariable/Exponential] ;# Poisson process
$tmp set avg_ 5 ;# average request interval
$client set-interval-generator $tmp
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

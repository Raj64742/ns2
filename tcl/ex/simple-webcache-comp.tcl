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
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/ex/simple-webcache-comp.tcl,v 1.1 1999/02/13 21:26:43 haoboy Exp $
#
# Example script illustrating the usage of complex page in webcache. 
# Thanks to Xiaowei Yang <yxw@mercury.lcs.mit.edu> for motivation and 
# an original version of this script.

Agent/TCP/FullTcp set segsize_ 1460
Http set TRANSPORT_ FullTcp

set ns [new Simulator]

set f [open "comp.tr" w]
$ns namtrace-all $f

set log [open "comp.log" w]

$ns rtproto Session
set node(c) [$ns node]
set node(r) [$ns node]
set node(s) [$ns node]
$ns duplex-link $node(s) $node(r) 100Mb 25ms DropTail
$ns duplex-link $node(r) $node(c) 10Mb 25ms DropTail

[$ns link $node(s) $node(r)] set queue-limit 10000
[$ns link $node(r) $node(s)] set queue-limit 10000
[$ns link $node(c) $node(r)] set queue-limit 10000
[$ns link $node(r) $node(c)] set queue-limit 10000

# Use PagePool/CompMath to generate compound page, which is a page 
# containing multiple embedded objects.
set pgp [new PagePool/CompMath]
$pgp set main_size_ 1000	;# Size of main page
$pgp set comp_size_ 5000	;# Size of embedded object
$pgp set num_pages_ 3		;# Number of objects per compoud page

# Average age of component object: 100s
set tmp [new RandomVariable/Constant]
$tmp set val_ 100
$pgp ranvar-obj-age $tmp

# Average age of the main page: 50s
set tmp [new RandomVariable/Constant]
$tmp set val_ 50
$pgp ranvar-main-age $tmp

set server [new Http/Server/Compound $ns $node(s)]
$server set-page-generator $pgp
$server log $log

set client [new Http/Client/Compound $ns $node(c)]
set tmp [new RandomVariable/Constant]
$tmp set val_ 10
$client set-interval-generator $tmp
$client set-page-generator $pgp
$client log $log

set startTime 1 ;# simulation start time
set finishTime 500 ;# simulation end time
$ns at $startTime "start-connection"
$ns at $finishTime "finish"

proc start-connection {} {
	global ns server  client
	$client connect $server

	# Comment out following line to continuously send requests
	$client start-session $server $server

	# Comment out following line to send out ONE request
	# DON't USE with start-session!!!
	#set pageid $server:[lindex [$client gen-request] 1]
	#$client send-request $server GET $pageid
}

proc finish {} {
	global ns log f client server

	$client stop-session $server
	$client disconnect $server

	$ns flush-trace
	flush $log
	close $log
	exit 0
}

$ns run

#
# Copyright (C) 1997 by USC/ISI
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 

#
# Maintainer: Kannan Varadhan <kannan@isi.edu>
# Version Date: $Date: 1997/10/23 20:53:34 $
#
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/ex/srm-session.tcl,v 1.2 1997/10/23 20:53:34 kannan Exp $ (USC/ISI)
#

if [string match {*.tcl} $argv0] {
	set prog [string range $argv0 0 [expr [string length $argv0] - 5]]
} else {
	set prog $argv0
}

source ../mcast/srm-nam.tcl		;# to separate control messages.
#source ../mcast/srm-debug.tcl		;# to trace delay compute fcn. details.

set ns [new SessionSim]

# make the nodes
for {set i 0} {$i <= 3} {incr i} {
	set n($i) [$ns node]
}

# now the links
for {set i 1} {$i <= 3} {incr i} {
	$ns duplex-link $n($i) $n(0) 1.5Mb 10ms DropTail
}

set group 0x8000

# now the multicast, and the agents
set srmStats [open srmStatsSes.tr w]
set srmEvents [open srmEventsSes.tr w]

set fid 0
foreach i [array names n] {
	set srm($i) [new Agent/SRM]
	$srm($i) set dst_ $group
	$srm($i) set fid_ [incr fid]
	$srm($i) log $srmStats
	$srm($i) trace $srmEvents

	$ns attach-agent $n($i) $srm($i)
        $ns create-session $n($i) $srm($i) ;# add this line to add session helpers
}

# Attach a data source to srm(1)
set packetSize 210
set s0 [new Agent/CBR/UDP]
set exp0 [new Traffic/Expoo]
$exp0 set packet-size $packetSize
$exp0 set burst-time 500ms
$exp0 set idle-time 500ms
$exp0 set rate 100k
$s0 set fid_ 0
$s0 attach-traffic $exp0

$srm(0) traffic-source $s0
$srm(0) set packetSize_ $packetSize	;# so repairs are correct

$ns at 0.5 "$srm(0) start; $srm(0) start-source"
$ns at 1.0 "$srm(1) start"
$ns at 1.1 "$srm(2) start"
$ns at 1.2 "$srm(3) start"

proc distDump interval {
	global ns srm
	foreach i [array names srm] {
		set dist [$srm($i) distances?]
		if {$dist != ""} {
			puts "[format %7.4f [$ns now]] distances $dist"
		}
	}
	$ns at [expr [$ns now] + $interval] "distDump $interval"
}
$ns at 0.0 "distDump 1"

proc finish src {
	$src stop

	global prog ns srmStats srmEvents
	$ns flush-trace
	close $srmStats
	close $srmEvents

	exit 0
}
$ns at 5.0 "finish $s0"

#$ns gen-map
$ns run

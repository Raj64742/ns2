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
#

set ns [new Simulator]

set n0 [$ns node]
set n1 [$ns node]

set f [open out.tr w]
$ns trace-all $f

$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail
$ns queue-limit $n0 $n1 2

set cbr0 [new Agent/CBR]
$cbr0 set interval_ 1ms
$ns attach-agent $n0 $cbr0

set null1 [new Agent/Null]
$ns attach-agent $n1 $null1

$ns connect $cbr0 $null1

$ns rtmodel Deterministic {} $n0
[$ns link $n0 $n1] trace-dynamics $ns stdout

proc finish {} {
	global ns f
	close $f
	$ns flush-trace

	puts "converting output to nam format..."
	exec awk -f ../nam-demo/nstonam.awk out.tr > dynamic-nam.tr 
	exec rm -f out
	#XXX
	puts "running nam..."
	exec nam dynamic-nam &
	exit 0
}

$ns at 1.0 "$cbr0 start"
$ns at 8.0 "finish"
puts [$cbr0 set packetSize_]
puts [$cbr0 set interval_]
$ns run

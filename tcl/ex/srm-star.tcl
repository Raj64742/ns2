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
# Version Date: $Date: 1997/10/23 20:53:35 $
#
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/ex/srm-star.tcl,v 1.7 1997/10/23 20:53:35 kannan Exp $ (USC/ISI)
#

if [string match {*.tcl} $argv0] {
    set prog [string range $argv0 0 [expr [string length $argv0] - 5]]
} else {
    set prog $argv0
}

if {[llength $argv] > 0} {
    set srmSimType [lindex $argv 0]
} else {
    set srmSimType Probabilistic
}

source ../mcast/srm-nam.tcl		;# to separate control messages.
#source ../mcast/srm-debug.tcl		;# to trace delay compute fcn. details.

Simulator set NumberInterfaces_ 1
set ns [new Simulator]
Simulator set EnableMcast_ 1
$ns trace-all [open out.tr w]

# make the nodes
set nmax 8
for {set i 0} {$i <= $nmax} {incr i} {
    set n($i) [$ns node]
}

# now the links
for {set i 1} {$i <= $nmax} {incr i} {
    $ns duplex-link $n($i) $n(0) 1.5Mb 10ms DropTail
}

set group 0x8000
set cmc [$ns mrtproto CtrMcast {}]
$ns at 0.3 "$cmc switch-treetype $group"

# now the multicast, and the agents
set srmStats [open srmStats.tr w]
set srmEvents [open srmEvents.tr w]

set fid 0
for {set i 1} {$i <= $nmax} {incr i} {
    set srm($i) [new Agent/SRM/$srmSimType]
    $srm($i) set dst_ $group

    $srm($i) set fid_ [incr fid]
    $srm($i) log $srmStats
    $srm($i) trace $srmEvents
    $ns at 1.0 "$srm($i) start"

    $ns attach-agent $n($i) $srm($i)
}

# Attach a data source to srm(1)
set packetSize 800
set s [new Agent/CBR]
$s set interval_ 0.02
$s set packetSize_ $packetSize
$s set fid_ 0
$srm(1) traffic-source $s
$srm(1) set packetSize_ $packetSize
$ns at 3.5 "$srm(1) start-source"

#$ns rtmodel-at 3.519 down $n(0) $n(1)	;# this ought to drop exactly one
#$ns rtmodel-at 3.521 up   $n(0) $n(1)	;# data packet?
set loss_module [new SRMErrorModel]
$loss_module drop-packet 2 10 1
$loss_module drop-target [$ns set nullAgent_]
$ns at 1.25 "$ns lossmodel $loss_module $n(1) $n(0)"

$ns at 4.0 "finish $s"
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
#$ns at 0.0 "distDump 1"

proc finish src {
    global prog ns env srmStats srmEvents srm nmax
    
    $src stop
    $ns flush-trace		;# NB>  Did not really close out.tr...:-)
    close $srmStats
    close $srmEvents
    set avg_info [open srm-avg-info w]
    puts $avg_info "avg:\trep-delay\treq-delay\trep-dup\t\treq-dup"
    foreach index [lsort -integer [array name srm]] {
	set tmplist [$srm($index) array get stats_]
	puts $avg_info "$index\t[format %7.4f [lindex $tmplist 1]]\t\t[format %7.4f [lindex $tmplist 5]]\t\t[format %7.4f [lindex $tmplist 9]]\t\t[format %7.4f [lindex $tmplist 13]]"
    }
    flush $avg_info
    close $avg_info
    puts "converting output to nam format..."
    exec awk -f ../nam-demo/nstonam.awk out.tr > $prog-nam.tr 

    if [info exists env(DISPLAY)] {
	puts "running nam..."
	exec nam $prog-nam &
    } else {
	exec cat srmStats.tr >@stdout
    }
    exit 0
}

$ns run


# 
#  Copyright (c) 1999 by the University of Southern California
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
# Traffic source generator from CMU's mobile code.
#
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/indep-utils/cmu-scen-gen/cbrgen.tcl,v 1.1 1999/02/24 01:42:41 haoboy Exp $

# ======================================================================
# Default Script Options
# ======================================================================
set opt(nn)		0		;# Number of Nodes
set opt(seed)		0.0
set opt(mc)		0
set opt(pktsize)	512

set opt(rate)		0
set opt(interval)	0.0		;# inverse of rate
set opt(type)           ""

# ======================================================================

proc usage {} {
    global argv0

    puts "\nusage: $argv0 \[-type cbr|tcp\] \[-nn nodes\] \[-seed seed\] \[-mc connections\] \[-rate rate\]\n"
}

proc getopt {argc argv} {
	global opt
	lappend optlist nn seed mc rate type

	for {set i 0} {$i < $argc} {incr i} {
		set arg [lindex $argv $i]
		if {[string range $arg 0 0] != "-"} continue

		set name [string range $arg 1 end]
		set opt($name) [lindex $argv [expr $i+1]]
	}
}

proc create-cbr-connection { src dst } {
	global rng cbr_cnt opt

	set stime [$rng uniform 0.0 180.0]

	puts "#\n# $src connecting to $dst at time $stime\n#"

	puts "set cbr_($cbr_cnt) \[\$ns_ create-connection \
		CBR \$node_($src) CBR \$node_($dst) 0\]";

	puts "\$cbr_($cbr_cnt) set packetSize_ $opt(pktsize)"
	puts "\$cbr_($cbr_cnt) set interval_ $opt(interval)"
	puts "\$cbr_($cbr_cnt) set random_ 1"
	puts "\$cbr_($cbr_cnt) set maxpkts_ 10000"
	
	puts "\$ns_ at $stime \"\$cbr_($cbr_cnt) start\""

	incr cbr_cnt
}

proc create-tcp-connection { src dst } {
	global rng cbr_cnt opt

	set stime [$rng uniform 0.0 180.0]

	puts "#\n# $src connecting to $dst at time $stime\n#"

	puts "set tcp_($cbr_cnt) \[\$ns_ create-connection \
		TCP \$node_($src) TCPSink \$node_($dst) 0\]";
	puts "\$tcp_($cbr_cnt) set window_ 32"
	puts "\$tcp_($cbr_cnt) set packetSize_ $opt(pktsize)"

	puts "set ftp_($cbr_cnt) \[\$tcp_($cbr_cnt) attach-source FTP\]"


	puts "\$ns_ at $stime \"\$ftp_($cbr_cnt) start\""

	incr cbr_cnt
}

# ======================================================================

getopt $argc $argv

if { $opt(type) == "" } {
    usage
    exit
} elseif { $opt(type) == "cbr" } {
    if { $opt(nn) == 0 || $opt(seed) == 0.0 || $opt(mc) == 0 || $opt(rate) == 0 } {
	usage
	exit
    }

    set opt(interval) [expr 1 / $opt(rate)]
    if { $opt(interval) <= 0.0 } {
	puts "\ninvalid sending rate $opt(rate)\n"
	exit
    }
}

puts "#\n# nodes: $opt(nn), max conn: $opt(mc), send rate: $opt(interval), seed: $opt(seed)\n#"

set rng [new RNG]
$rng seed $opt(seed)

set u [new RandomVariable/Uniform]
$u set min_ 0
$u set max_ 100
$u use-rng $rng

set cbr_cnt 0
set src_cnt 0

for {set i 1} {$i <= $opt(nn) } {incr i} {

	set x [$u value]

	if {$x < 50} {continue;}

	incr src_cnt

	set dst [expr ($i+1) % [expr $opt(nn) + 1] ]
	if { $dst == 0 } {
		set dst [expr $dst + 1]
	}

	if { $opt(type) == "cbr" } {
		create-cbr-connection $i $dst
	} else {
		create-tcp-connection $i $dst
	}

	if { $cbr_cnt == $opt(mc) } {
		break
	}

	if {$x < 75} {continue;}

	set dst [expr ($i+2) % [expr $opt(nn) + 1] ]
	if { $dst == 0 } {
		set dst [expr $dst + 1]
	}

	if { $opt(type) == "cbr" } {
		create-cbr-connection $i $dst
	} else {
		create-tcp-connection $i $dst
	}

	if { $cbr_cnt == $opt(mc) } {
		break
	}
}

puts "#\n#Total sources/connections: $src_cnt/$cbr_cnt\n#"



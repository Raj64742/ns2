#
# Copyright (c) 1995 The Regents of the University of California.
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
#	This product includes software developed by the Computer Systems
#	Engineering Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/test-suite-cbq.tcl,v 1.1 1996/12/19 03:22:46 mccanne Exp $
#
#
# This test suite reproduces the tests from the following note:
# Floyd, S., Ns Simulator Tests for Class-Based Queueing, February 1996.
# URL ftp://ftp.ee.lbl.gov/papers/cbqsims.ps.Z.
#
# To run individual tests:
# ns test-suite-cbq.tcl cbqPRR 
# ns test-suite-cbq.tcl cbqWRR
#

# ~/newr/rm/testB
# Create a flat link-sharing structure.
#
proc create_flat { link } {

  	set topClass [ns_create_class none none 0.98 0.005 -1.0 8 1 0]
	# maxidle - higher values are better for AO,WRR link-sharing.
 	set vidClass [ns_create_class $topClass $topClass \
		0.32 0.006 -0.00014 1 0 0]
 	set audioClass [ns_create_class $topClass $topClass \
		0.03 0.01 -0.05 1 0 0]
	# changing minidle from -5.0 to -0.005 causes problems.
	# for PRR, -0.05 is ok.
	# for WRR, 
 	set dataClass [ns_create_class $topClass $topClass \
		0.65 0.01 -0.00017 2 0 0]

 	$link insert $topClass
	$link insert $vidClass
 	$link insert $audioClass
        $link insert $dataClass

	$link bind $vidClass 2
 	$link bind $audioClass 1
	$link bind $dataClass 3
}

#
# Create a CBQ class.  (Ns_create_class is in ns_default.tcl.)
#
proc ns_create_class2 { class parent borrow allot maxIdle minIdle priority depth extraDelay} {
        $class parent $parent
        $class borrow $borrow
        $class set allotment $allot
        $class set maxidle $maxIdle
        $class set minidle $minIdle
        $class set priority $priority
        $class set depth $depth
        $class set extradelay $extraDelay
        return $class
}

# ~/newr/rm/testH,A
# Create a two-agency link-sharing structure.
#
proc create_twoAgency { link CBQalgorithm } {

	set topClass [new class]
	set topAClass [new class]
	set topBClass [new class]

	if {$CBQalgorithm < 1} { 
	  # For Ancestor-Only link-sharing. 
	  # Maxidle should be smaller for AO link-sharing.
	  ns_create_class2 $topClass none none 0.97 0.001 -1.0 8 2 0
	  ns_create_class2 $topAClass $topClass $topClass 0.69 0.004 -1.0 8 1 0
	  ns_create_class2 $topBClass $topClass $topClass 0.29 0.004 -1.0 8 1 0
	}
	if { $CBQalgorithm == 1} { 
	  # For Top-Level link-sharing?
	  # borrowing from $topAClass is occuring before from $topClass
	  # yellow borrows from $topBClass
	  # Goes green borrow from $topClass or from $topAClass?
	  # When $topBClass is unsatisfied, there is no borrowing from
	  #  $topClass until a packet is sent from the yellow class.
	  ns_create_class2 $topClass none none 0.97 0.001 -1.0 8 2 0
	  ns_create_class2 $topAClass $topClass $topClass 0.69 0.004 -1.0 8 1 0
	  ns_create_class2 $topBClass $topClass $topClass 0.29 0.004 -1.0 8 1 0
	}
	if {$CBQalgorithm > 1} { 
	  # For Formal link-sharing
	  # The allocated bandwidth can be exact for parent classes.
	  ns_create_class2 $topClass none none 1.0 1.0 -1.0 8 2 0
	  ns_create_class2 $topAClass $topClass $topClass 0.7 1.0 -1.0 8 1 0
	  ns_create_class2 $topBClass $topClass $topClass 0.3 1.0 -1.0 8 1 0
	}

	set vidAClass [ns_create_class $topAClass $topAClass \
		0.3 0.01 -0.01 1 0 0] 
 	set dataAClass [ns_create_class $topAClass $topAClass \
		0.4 0.005 -0.01 2 0 0]
 	set vidBClass [ns_create_class $topBClass $topBClass \
		0.1 0.04 -0.01 1 0 0]
 	set dataBClass [ns_create_class $topBClass $topBClass \
		0.2 0.04 -0.01 2 0 0]

	$link insert $topClass
 	$link insert $topAClass
 	$link insert $topBClass
	$link insert $vidAClass
	$link insert $vidBClass
        $link insert $dataAClass
        $link insert $dataBClass

	$link bind $vidAClass 1
	$link bind $dataAClass 2
	$link bind $vidBClass 3
	$link bind $dataBClass 4
}

proc create_graph { stopTime cbqType queue } {

	global s1 s2 s3 s4 r1 k1 
	set s1 [ns node]
	set s2 [ns node]
	set s3 [ns node]
	set s4 [ns node]
	set r1 [ns node]
	set k1 [ns node]

	ns_duplex $s1 $r1 10Mb 5ms drop-tail
	ns_duplex $s2 $r1 10Mb 5ms drop-tail
	ns_duplex $s3 $r1 10Mb 5ms drop-tail
	ns_duplex $s4 $r1 10Mb 5ms drop-tail

 	set cbqLink [ns link $r1 $k1 $cbqType]
 	$cbqLink set bandwidth 1.5Mb
	set maxBytes 187500
	# for a maxburst of 5, 1000-byte packets, set maxidle to 
	#	0.002 (1/p - 1)
 	$cbqLink set delay 5ms
	cbrDump4 $cbqLink 1.0 $stopTime $maxBytes

        set link1 [ns link $k1 $r1 drop-tail]
        $link1 set bandwidth 1.5Mb
        $link1 set delay 5ms

        set L "$cbqLink $link1"
	[lindex $L 0] set queue-limit $queue 
	[lindex $L 1] set queue-limit $queue
}

proc finish file {
 
        set f [open temp.rands w]
        puts $f "TitleText: $file"
        puts $f "Device: Postscript" 

        exec rm -f temp.p temp.d
        exec touch temp.d temp.p
        #
        # split queue/drop events into two separate files.
        # we don't bother checking for the link we're interested in
        # since we know only such events are in our trace file
        #
        exec awk {
                { 
                        if (($1 == "+" || $1 == "-" ) && \
                            ($5 == "tcp" || $5 == "ack" || $5 == "cbr"))\
                                        print $2, $9 + ($11 % 90) * 0.01
                }
        } out.tr > temp.p  
        exec awk {
                {
                        if ($1 == "d")
                                print $2, $9 + ($11 % 90) * 0.01
                }
        } out.tr > temp.d

        puts $f \"packets
        flush $f
        exec cat temp.p >@ $f
        flush $f
        # insert dummy data sets so we get X's for marks in data-set 4
        puts $f [format "\n\"skip-1\n0 1\n\n\"skip-2\n0 1\n\n"]

        puts $f \"drops
        flush $f
        #
        # Repeat the first line twice in the drops file because
        # often we have only one drop and xgraph won't print marks
        # for data sets with only one point.
        #
        exec head -1 temp.d >@ $f
        exec cat temp.d >@ $f
        close $f
        exec xgraph -bb -tk -nl -m -x time -y packet temp.rands &
#	exec csh figure3.com $file

        exit 0

}

# display graph of results
proc finish1 file {

	set awkCode { { 
	  if ($1 == "maxbytes") maxbytes = $2;
	  if ($2 == class) print $1, $3/maxbytes >> "temp.t"; 
	} }
	set awkCodeAll { { 
	  if ($2 == class) { print time, sum >> "temp.t"; sum = 0; }
	  if ($1 == "maxbytes") maxbytes = $2;
	  sum += $3/maxbytes;
	  if (NF==3) time = $1; else time = 0;
	} }

	set f [open temp.rands w]
	puts $f "TitleText: $file"
	puts $f "Device: Postscript"
	
	exec rm -f temp.p 
	exec touch temp.p
	exec rm -f temp.t 
	exec touch temp.t
	exec awk $awkCode class=1 temp.s 
	exec cat temp.t >> temp.p
	exec echo " " >> temp.p
	exec mv temp.t temp.1
	exec awk $awkCode class=2 temp.s 
	exec cat temp.t >> temp.p
	exec echo " " >> temp.p
	exec mv temp.t temp.2
	exec awk $awkCode class=3 temp.s 
	exec cat temp.t >> temp.p
	exec echo " " >> temp.p
	exec mv temp.t temp.3
	exec awk $awkCode class=4 temp.s 
	exec cat temp.t >> temp.p
	exec echo " " >> temp.p
	exec mv temp.t temp.4
	exec awk $awkCodeAll class=1 temp.s 
	exec cat temp.t >> temp.p 
	exec echo " " >> temp.p  
	exec mv temp.t temp.5 

	exec cat temp.p >@ $f
	close $f
	exec xgraph -bb -tk -x time -y bandwidth temp.rands &
#	exec csh figure2.com $file
	
	exit 0
}

# display graph of results
proc finish2 file {

	set awkCode { { 
	  if ($1 == "maxbytes") maxbytes = $2;
	  if ($2 == class) print $1, $3/maxbytes >> "temp.t"; 
	} }
	set awkCodeAll { { 
	  if ($2 == class) { print time, sum >> "temp.t"; sum = 0; }
	  if ($1 == "maxbytes") maxbytes = $2;
	  sum += $3/maxbytes;
	  if (NF==3) time = $1; else time = 0;
	} }

	set f [open temp.rands w]
	puts $f "TitleText: $file"
	puts $f "Device: Postscript"
	
	exec rm -f temp.p 
	exec touch temp.p
	exec rm -f temp.t 
	exec touch temp.t
	exec awk $awkCode class=1 temp.s 
	exec cat temp.t >> temp.p
	exec echo " " >> temp.p
	exec mv temp.t temp.1
	exec awk $awkCode class=2 temp.s 
	exec cat temp.t >> temp.p
	exec echo " " >> temp.p
	exec mv temp.t temp.2
	exec awk $awkCodeAll class=1 temp.s 
	exec cat temp.t >> temp.p 
	exec echo " " >> temp.p  
	exec mv temp.t temp.5 

	exec cat temp.p >@ $f
	close $f
	exec xgraph -bb -tk -x time -y bandwidth temp.rands &
#	exec csh figure4.com $file
	
	exit 0
}

#
# Save the number of bytes sent by each class in each time interval.
#
proc cbrDump4 { linkno interval stopTime maxBytes } {
	global oldbytes1 oldbytes2 oldbytes3 oldbytes4
	proc cdump { lnk interval file }  {
	  global oldbytes1 oldbytes2 oldbytes3 oldbytes4
	  set timenow [ns now]
	  set bytes1 [$lnk stat 1 bytes]
	  set bytes2 [$lnk stat 2 bytes]
	  set bytes3 [$lnk stat 3 bytes]
	  set bytes4 [$lnk stat 4 bytes]
	  puts "$timenow"
	  # format: time, class, bytes
	  puts $file "$timenow 1 [expr $bytes1 - $oldbytes1]"
	  puts $file "$timenow 2 [expr $bytes2 - $oldbytes2]"
	  puts $file "$timenow 3 [expr $bytes3 - $oldbytes3]"
	  puts $file "$timenow 4 [expr $bytes4 - $oldbytes4]"
	  set oldbytes1 $bytes1
	  set oldbytes2 $bytes2
	  set oldbytes3 $bytes3
	  set oldbytes4 $bytes4
	  ns at [expr [ns now] + $interval] "cdump $lnk $interval $file"
	}
	set oldbytes1 0
	set oldbytes2 0
        set oldbytes3 0
	set oldbytes4 0
	set f [open temp.s w]
	puts $f "maxbytes $maxBytes"
	ns at 0.0 "cdump $linkno $interval $f"
	ns at $stopTime "close $f"
}

#
# Do not make out.tr.
#
proc openTrace2 { stopTime testName } {
	global r1 k1    
	ns at $stopTime \
		"finish1 $testName; exit"
}

#
# Do not make out.tr.
#
proc openTrace4 { stopTime testName } {
	global r1 k1    
	ns at $stopTime \
		"finish2 $testName; exit"
}

#
# Make graph of transmitted packets.
#
proc openTrace3 { stopTime testName } {
        exec rm -f out.tr temp.rands
        global r1 k1
        set traceFile [open out.tr w]
        ns at $stopTime \
                "close $traceFile ; finish $testName"
        set T [ns trace]
        $T attach $traceFile
        return $T
} 

#
# Create three CBR connections.
#
proc three_cbrs {} {
	global s1 s2 s3 s4 r1 k1
	set cbr0 [ns_create_cbr $s1 $k1 190 0.001 1]
        set cbr1 [ns_create_cbr $s2 $k1 500 0.002 2]
        set cbr2 [ns_create_cbr $s3 $k1 1000 0.005 3]

	ns at 0.0 "$cbr0 start"
	ns at 20.0 "$cbr0 stop"
        ns at 24.0 "$cbr0 start"
        ns at 0.0 "$cbr1 start"
        ns at 12.0 "$cbr1 stop"
        ns at 18.0 "$cbr1 start"
        ns at 0.0 "$cbr2 start"
        ns at 4.0 "$cbr2 stop"
        ns at 8.0 "$cbr2 start"
}

#
# Create four CBR connections.
#
proc four_cbrs {} {
	global s1 s2 s3 s4 r1 k1
	set cbr0 [ns_create_cbr $s1 $k1 190 0.001 1]
        set cbr1 [ns_create_cbr $s2 $k1 1000 0.005 2]
        set cbr2 [ns_create_cbr $s3 $k1 500 0.002 3]
	set cbr3 [ns_create_cbr $s4 $k1 1000 0.005 4] 

	ns at 0.0 "$cbr0 start"
	ns at 12.0 "$cbr0 stop"
        ns at 16.0 "$cbr0 start"
	ns at 36.0 "$cbr0 stop"
        ns at 0.0 "$cbr1 start"
        ns at 20.0 "$cbr1 stop"
        ns at 24.0 "$cbr1 start"
        ns at 0.0 "$cbr2 start"
        ns at 4.0 "$cbr2 stop"
        ns at 8.0 "$cbr2 start"
	ns at 36.0 "$cbr2 stop"
        ns at 0.0 "$cbr3 start"
        ns at 28.0 "$cbr3 stop"
        ns at 32.0 "$cbr3 start"
}

#
# Figure 10 from the link-sharing paper.
# ~/newr/rm/testB.com
# 
proc test_cbqWRR {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 28.1
	set CBQalgorithm 0
	create_graph $stopTime wrr-cbq 20
 	create_flat [ns link $r1 $k1]
	three_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace2 $stopTime test_cbqWRR

	ns run
}

#
# Figure 10, but packet-by-packet RR.
# 
proc test_cbqPRR {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 28.1
	set CBQalgorithm 0
	create_graph $stopTime cbq 20 
 	create_flat [ns link $r1 $k1]
	three_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace2 $stopTime test_cbqPRR

	ns run
}

#
# Figure 11 from the link-sharing paper.
# WRR, Ancestor-Only link-sharing.
# ~/newr/rm/testA.com
# 
proc test_cbqAO {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 40.1
	set CBQalgorithm 0
	create_graph $stopTime wrr-cbq 20
	create_twoAgency [ns link $r1 $k1] $CBQalgorithm
	four_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace2 $stopTime test_cbqAO

	ns run
}

#
# Figure 11 from the link-sharing paper.
# WRR, Top link-sharing.
# ~/newr/rm/testA.com
# Needs some debugging still?
proc test_cbqTL {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 40.1
	set CBQalgorithm 1
	create_graph $stopTime wrr-cbq 20
	create_twoAgency [ns link $r1 $k1] $CBQalgorithm
	four_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace2 $stopTime test_cbq6TL

	ns run
}

#
# Figure 11 from the link-sharing paper.
# WRR, Formal (new) link-sharing.
# ~/newr/rm/testA.com
proc test_cbqFor {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 40.1
	set CBQalgorithm 2
	create_graph $stopTime wrr-cbq 20
	create_twoAgency [ns link $r1 $k1] $CBQalgorithm
	four_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace2 $stopTime test_cbqFor

	ns run
}

#
# Figure 11 from the link-sharing paper.
# WRR, Formal (old) link-sharing.
# ~/newr/rm/testA.com
proc test_cbqForOld {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 40.1
	set CBQalgorithm 3
	create_graph $stopTime wrr-cbq 20
	create_twoAgency [ns link $r1 $k1] $CBQalgorithm
	four_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace2 $stopTime test_cbqForOld

	ns run
}

#
# To send five back-to-back packets for $audClass, 
#   maxidle should be 0.004 seconds
# To send 50 back-to-back packets, maxidle should be 0.25 seconds
proc test_cbqMax1 {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 2.1
	set CBQalgorithm 2
	set ns_link(queue-limit) 1000
	set queue 1000
	create_graph $stopTime cbq $queue

	set link [ns link $r1 $k1]
	set topClass [ns_create_class none none 0.97 1.0 -100.0 8 1 0]
        set audClass [ns_create_class $topClass none 0.3 0.25 -0.14 1 0 0]
	set dataClass [ns_create_class $topClass $topClass 0.3 6.0 -0.14 2 0 0]

	$link insert $topClass
        $link insert $audClass
	$link insert $dataClass

        $link bind $audClass 1
	$link bind $dataClass 2

        set cbr0 [ns_create_cbr $s1 $k1 1000 0.001 1]
        set cbr1 [ns_create_cbr $s2 $k1 1000 0.01 2]

	ns at 0.0 "$cbr0 start"
	ns at 0.002 "$cbr0 stop"
	ns at 1.0 "$cbr0 start"
	ns at 1.08 "$cbr0 stop"
	ns at 0.0 "$cbr1 start"
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	[ns link $r1 $k1] trace [openTrace3 $stopTime test_Max1,_25_pkts]

	ns run
}

#
# To send five back-to-back packets for $audClass, 
#   maxidle should be 0.004 seconds
# To send 50 back-to-back packets, maxidle should be 0.25 seconds
proc test_cbqMax2 {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 2.1
	set CBQalgorithm 2
	set ns_link(queue-limit) 1000
	set queue 1000
	create_graph $stopTime cbq $queue

	set link [ns link $r1 $k1]
	set topClass [ns_create_class none none 0.97 1.0 -100.0 8 1 0]
        #set audClass [ns_create_class $topClass none 0.3 0.25 -0.14 1 0 0]
	set audClass [ns_create_class $topClass none 0.3 0.004 -0.14 1 0 0]
	set dataClass [ns_create_class $topClass $topClass 0.3 6.0 -0.14 2 0 0]

	$link insert $topClass
        $link insert $audClass
	$link insert $dataClass

        $link bind $audClass 1
	$link bind $dataClass 2

        set cbr0 [ns_create_cbr $s1 $k1 1000 0.001 1]
        set cbr1 [ns_create_cbr $s2 $k1 1000 0.01 2]

	ns at 0.0 "$cbr0 start"
	ns at 0.002 "$cbr0 stop"
	ns at 1.0 "$cbr0 start"
	ns at 1.08 "$cbr0 stop"
	ns at 0.0 "$cbr1 start"
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	[ns link $r1 $k1] trace [openTrace3 $stopTime test_Max2,_5_pkts]

	ns run
}

#
# Set "extradelay" to 0.024 seconds for a steady-state burst of 2 
#
proc test_cbqExtra1 {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 2.1
	set CBQalgorithm 2
	set ns_link(queue-limit) 1000
	set queue 1000
	create_graph $stopTime cbq $queue

	set link [ns link $r1 $k1]
	set topClass [ns_create_class none none 0.97 1.0 -100.0 8 1 0]
        set audClass [ns_create_class $topClass none 0.3 0.004 -0.14 1 0 0.024]
	set dataClass [ns_create_class $topClass $topClass 0.3 6.0 -0.14 2 0 0]

	$link insert $topClass
        $link insert $audClass
	$link insert $dataClass

        $link bind $audClass 1
	$link bind $dataClass 2

        set cbr0 [ns_create_cbr $s1 $k1 1000 0.015 1]
        set cbr1 [ns_create_cbr $s2 $k1 1000 0.01 2]

	ns at 0.0 "$cbr0 start"
	ns at 0.0 "$cbr1 start"
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	[ns link $r1 $k1] trace [openTrace3 $stopTime test_Extra1_burst_2]

	ns run
}

#
# Set "extradelay" to 0.12 seconds for a steady-state burst of 8 
#
proc test_cbqExtra2 {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 2.1
	set CBQalgorithm 2
	set ns_link(queue-limit) 1000
	set queue 1000
	create_graph $stopTime cbq $queue

	set link [ns link $r1 $k1]
	set topClass [ns_create_class none none 0.97 1.0 -100.0 8 1 0]
        set audClass [ns_create_class $topClass none 0.3 0.004 -0.14 1 0 0.12]
	set dataClass [ns_create_class $topClass $topClass 0.3 6.0 -0.14 2 0 0]

	$link insert $topClass
        $link insert $audClass
	$link insert $dataClass

        $link bind $audClass 1
	$link bind $dataClass 2

        set cbr0 [ns_create_cbr $s1 $k1 1000 0.015 1]
        set cbr1 [ns_create_cbr $s2 $k1 1000 0.01 2]

	ns at 0.0 "$cbr0 start"
	ns at 0.0 "$cbr1 start"
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	[ns link $r1 $k1] trace [openTrace3 $stopTime test_Extra2_burst_8]

	ns run
}

# With Packet-by-Packet Round-robin, it is necessary either to
# set a positive value for extradelay, or a negative value for minidle
#
proc test_cbqMin1 {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 4.1
	set CBQalgorithm 2
	create_graph $stopTime cbq 20 
 	set link [ns link $r1 $k1]

	set topClass [ns_create_class none none 0.98 0.00005 -1.0 8 1 0]
        set audioClass [ns_create_class $topClass none 0.03 0.01 -0.05 1 0 0]
	set vidClass [ns_create_class $topClass $topClass \
		0.32 0.006 -0.00014 1 0 0] 
	set dataClass [ns_create_class $topClass $topClass \
		0.65 0.01 -0.00017 2 0 0]

 	$link insert $topClass
	$link insert $vidClass
 	$link insert $audioClass
        $link insert $dataClass

	$link bind $vidClass 2
 	$link bind $audioClass 1
	$link bind $dataClass 3

	three_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	#[ns link $r1 $k1] trace [openTrace3 $stopTime test_Min1]
	openTrace2 $stopTime test_cbqMin1_MinIdle_set

	ns run
}

proc test_cbqMin2 {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 4.1
	set CBQalgorithm 2
	create_graph $stopTime cbq 20 
 	set link [ns link $r1 $k1]

	set topClass [ns_create_class none none 0.98 0.00005 -1.0 8 1 0]
        set audioClass [ns_create_class $topClass none 0.03 0.01 0.0 1 0 0]
	set vidClass [ns_create_class $topClass $topClass \
		0.32 0.006 -0.00014 1 0 0] 
	set dataClass [ns_create_class $topClass $topClass \
		0.65 0.01 -0.00017 2 0 0]

 	$link insert $topClass
	$link insert $vidClass
 	$link insert $audioClass
        $link insert $dataClass

	$link bind $vidClass 2
 	$link bind $audioClass 1
	$link bind $dataClass 3

	three_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace2 $stopTime test_cbqMin2_bad_settings

	ns run
}

proc test_cbqMin3 {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 4.1
	set CBQalgorithm 2
	create_graph $stopTime cbq 20 
 	set link [ns link $r1 $k1]

	set topClass [ns_create_class none none 0.98 0.00005 -1.0 8 1 0]
        set audioClass [ns_create_class $topClass none 0.03 0.01 0.0 1 0 0.2]
	set vidClass [ns_create_class $topClass $topClass \
		0.32 0.006 -0.00014 1 0 0] 
	set dataClass [ns_create_class $topClass $topClass \
		0.65 0.01 -0.00017 2 0 0]

 	$link insert $topClass
	$link insert $vidClass
 	$link insert $audioClass
        $link insert $dataClass

	$link bind $vidClass 2
 	$link bind $audioClass 1
	$link bind $dataClass 3

	three_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace2 $stopTime test_cbqMin3_ExtraDelay_set

	ns run
}

# Min1, but with Ancestor-Only link-sharing.
# 
proc test_cbqMin4 {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 4.1
	set CBQalgorithm 0
	create_graph $stopTime cbq 20 
 	set link [ns link $r1 $k1]

	set topClass [ns_create_class none none 0.98 0.00005 -1.0 8 1 0]
        set audioClass [ns_create_class $topClass none 0.03 0.01 -0.05 1 0 0]
	set vidClass [ns_create_class $topClass $topClass \
		0.32 0.006 -0.00014 1 0 0] 
	set dataClass [ns_create_class $topClass $topClass \
		0.65 0.01 -0.00017 2 0 0]

 	$link insert $topClass
	$link insert $vidClass
 	$link insert $audioClass
        $link insert $dataClass

	$link bind $vidClass 2
 	$link bind $audioClass 1
	$link bind $dataClass 3

	three_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace2 $stopTime test_cbqMin4_MinIdle_set

	ns run
}

# 
# 
proc test_cbqMin5 {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 4.1
	set CBQalgorithm 0
	create_graph $stopTime cbq 20 
 	set link [ns link $r1 $k1]

	set topClass [ns_create_class none none 0.98 0.00005 -1.0 8 1 0]
        set audioClass [ns_create_class $topClass none 0.03 0.01 0.0 1 0 0]
	set vidClass [ns_create_class $topClass $topClass \
		0.32 0.006 -0.00014 1 0 0] 
	set dataClass [ns_create_class $topClass $topClass \
		0.65 0.01 -0.00017 2 0 0]

 	$link insert $topClass
	$link insert $vidClass
 	$link insert $audioClass
        $link insert $dataClass

	$link bind $vidClass 2
 	$link bind $audioClass 1
	$link bind $dataClass 3

	three_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace2 $stopTime test_cbqMin5_bad_settings

	ns run
}

# 
# 
proc test_cbqMin6 {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 4.1
	set CBQalgorithm 0
	create_graph $stopTime cbq 20 
 	set link [ns link $r1 $k1]

	set topClass [ns_create_class none none 0.98 0.00005 -1.0 8 1 0]
        set audioClass [ns_create_class $topClass none 0.03 0.01 0.0 1 0 0.2]
	set vidClass [ns_create_class $topClass $topClass \
		0.32 0.006 -0.00014 1 0 0] 
	set dataClass [ns_create_class $topClass $topClass \
		0.65 0.01 -0.00017 2 0 0]

 	$link insert $topClass
	$link insert $vidClass
 	$link insert $audioClass
        $link insert $dataClass

	$link bind $vidClass 2
 	$link bind $audioClass 1
	$link bind $dataClass 3

	three_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace2 $stopTime test_cbqMin6_ExtraDelay_set

	ns run
}

#
# With Formal link-sharing, the dataClass gets most of the bandwidth.
# With Ancestor-Only link-sharing, the audioClass generally gets
#   to borrow from the not-overlimit root class.
# With Top-Level link-sharing, the audioClass is often blocked
#   from borrowing by an underlimit dataClass
#
proc test_cbqTwoAO {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 8.1
	set CBQalgorithm 0
	create_graph $stopTime cbq 20
	set link [ns link $r1 $k1]

	set topClass [ns_create_class none none 0.98 0.000002 -1.0 8 1 0]
#set topClass [ns_create_class none none 0.98 0.005 -1.0 8 1 0]

        set audioClass [ns_create_class $topClass $topClass \
		0.001 0.1 -1.0 1 0 0]
	set dataClass [ns_create_class $topClass $topClass \
		0.999 0.000002 -1.0 2 0 0]

 	$link insert $topClass
 	$link insert $audioClass
        $link insert $dataClass

 	$link bind $audioClass 1
	$link bind $dataClass 2

        set cbr0 [ns_create_cbr $s1 $k1 190 0.001 1]
        set cbr1 [ns_create_cbr $s2 $k1 500 0.002 2]

        ns at 0.0 "$cbr0 start"
        ns at 0.0 "$cbr1 start"


	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace4 $stopTime test_cbqTwoAO

	ns run
}

proc test_cbqTwoAO2 {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 8.1
	set CBQalgorithm 0
	create_graph $stopTime cbq 20
	set link [ns link $r1 $k1]

	set topClass [ns_create_class none none 0.98 0.005 -1.0 8 1 0]

        set audioClass [ns_create_class $topClass $topClass \
		0.001 0.1 -1.0 1 0 0]
	set dataClass [ns_create_class $topClass $topClass \
		0.999 0.000002 -1.0 2 0 0]

 	$link insert $topClass
 	$link insert $audioClass
        $link insert $dataClass

 	$link bind $audioClass 1
	$link bind $dataClass 2

        set cbr0 [ns_create_cbr $s1 $k1 190 0.001 1]
        set cbr1 [ns_create_cbr $s2 $k1 500 0.002 2]

        ns at 0.0 "$cbr0 start"
        ns at 0.0 "$cbr1 start"


	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace4 $stopTime test_cbqTwoAO2

	ns run
}

proc test_cbqTwoAO3 {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 8.1
	set CBQalgorithm 0
	create_graph $stopTime cbq 20
	set link [ns link $r1 $k1]

	set topClass [ns_create_class none none 0.99 0.000002 -1.0 8 1 0]

        set audioClass [ns_create_class $topClass $topClass \
		0.001 0.1 -1.0 1 0 0]
	set dataClass [ns_create_class $topClass $topClass \
		0.999 0.000002 -1.0 2 0 0]

 	$link insert $topClass
 	$link insert $audioClass
        $link insert $dataClass

 	$link bind $audioClass 1
	$link bind $dataClass 2

        set cbr0 [ns_create_cbr $s1 $k1 190 0.001 1]
        set cbr1 [ns_create_cbr $s2 $k1 500 0.002 2]

        ns at 0.0 "$cbr0 start"
        ns at 0.0 "$cbr1 start"


	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace4 $stopTime test_cbqTwoAO3

	ns run
}

#
# With Formal link-sharing, the dataClass gets most of the bandwidth.
# With Ancestor-Only link-sharing, the audioClass generally gets
#   to borrow from the not-overlimit root class.
# With Top-Level link-sharing, the audioClass is often blocked
#   from borrowing by an underlimit dataClass
#
proc test_cbqTwoTL {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 8.1
	set CBQalgorithm 1
	create_graph $stopTime cbq 20
	set link [ns link $r1 $k1]

	set topClass [ns_create_class none none 1.0 0.000002 -1.0 8 1 0]
        set audioClass [ns_create_class $topClass $topClass \
		0.001 0.1 -1.0 1 0 0]
	set dataClass [ns_create_class $topClass $topClass \
		0.999 0.000002 -1.0 2 0 0]

 	$link insert $topClass
 	$link insert $audioClass
        $link insert $dataClass

 	$link bind $audioClass 1
	$link bind $dataClass 2

        set cbr0 [ns_create_cbr $s1 $k1 190 0.001 1]
        set cbr1 [ns_create_cbr $s2 $k1 500 0.002 2]

        ns at 0.0 "$cbr0 start"
        ns at 0.0 "$cbr1 start"


	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace4 $stopTime test_cbqTwoTL

	ns run
}

#
# With Formal link-sharing, the dataClass gets most of the bandwidth.
# With Ancestor-Only link-sharing, the audioClass generally gets
#   to borrow from the not-overlimit root class.
# With Top-Level link-sharing, the audioClass is often blocked
#   from borrowing by an underlimit dataClass
#
proc test_cbqTwoF {} {
	global s1 s2 s3 s4 r1 k1 
	set stopTime 8.1
	set CBQalgorithm 2
	create_graph $stopTime cbq 20
	set link [ns link $r1 $k1]

	set topClass [ns_create_class none none 1.0 0.000002 -1.0 8 1 0]
        set audioClass [ns_create_class $topClass $topClass \
		0.001 0.1 -1.0 1 0 0]
	set dataClass [ns_create_class $topClass $topClass \
		0.999 0.000002 -1.0 2 0 0]

 	$link insert $topClass
 	$link insert $audioClass
        $link insert $dataClass

 	$link bind $audioClass 1
	$link bind $dataClass 2

        set cbr0 [ns_create_cbr $s1 $k1 190 0.001 1]
        set cbr1 [ns_create_cbr $s2 $k1 500 0.002 2]

        ns at 0.0 "$cbr0 start"
        ns at 0.0 "$cbr1 start"


	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace4 $stopTime test_cbqTwoF

	ns run
}

if { $argc != 1 } {
	puts stderr {usage: ns test-suite.tcl [ tahoe1 tahoe2 ... reno reno2 ... ]}
	exit 1
}
if { "[info procs test_$argv]" != "test_$argv" } {
	puts stderr "test-suite.tcl: no such test: $argv"
}
test_$argv


#
# Copyright (c) 1995-1997 The Regents of the University of California.
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-cbq.tcl,v 1.2 1997/11/04 00:04:33 kfall Exp $
#
#
# This test suite reproduces the tests from the following note:
# Floyd, S., Ns Simulator Tests for Class-Based Queueing, February 1996.
# URL ftp://ftp.ee.lbl.gov/papers/cbqsims.ps.Z.
#
# To run individual tests:
# 	./ns test-suite-cbq.tcl cbqPRR 
# 	./ns test-suite-cbq.tcl cbqWRR
# 	...
#

puts "This test suite is still under construction"
exit 1

set dir [pwd]
catch "cd tcl/test"
source misc.tcl
source topologies.tcl
catch "cd $dir"

# ~/newr/rm/testB
# Create a flat link-sharing structure.
#
#	3 leaf classes:
#		vidClass	(32%), pri 1
#		audioClass	(03%), pri 1
#		dataClass	(65%), pri 2
#

TestSuite instproc create_flat { } {
	$self instvar topclass_ vidclass_ audioclass_ dataclass_

	set Mbps 1.5
	set topclass_ [new CBQClass]
	$topclass_ setparams none 0 0.98 auto 8 1 0 ;# Mbps

 	set vidclass_ [new CBQClass]
	$vidclass_ setparams $topclass_ 1 0.32 auto 1 2 0 ;# $Mbps

	set audioclass_ [new CBQClass]
	$audioclass_ setparams $topclass_ 1 0.03 auto 1 2 0;# $Mbps

	set dataclass_ [new CBQClass]
	$dataclass_ setparams $topclass_ 1 0.65 auto 2 2 0; #$Mbps
}

TestSuite instproc insert_flat { cbqlink } {
	$self instvar topclass_ vidclass_ audioclass_ dataclass_

	#
	# note: auto settings for maxidle are resolved in insert
	# (see tcl/lib/ns-queue.tcl)
	#

 	$cbqlink insert $topclass_
	$cbqlink insert $vidclass_
 	$cbqlink insert $audioclass_
        $cbqlink insert $dataclass_

	$cbqlink bind $vidclass_ 2;# fid 2
 	$cbqlink bind $audioclass_ 1;# fid 1
	$cbqlink bind $dataclass_ 3; # fid 3
}

# Create a two-agency link-sharing structure.
#
#	4 leaf classes for 2 "agencies":
#		vidAClass	(30%), pri 1
#		dataAClass	(40%), pri 2
#		vidBClass	(10%), pri 1
#		dataBClass	(20%), pri 2
#
TestSuite instproc create_twoagency { } {

	$self instvar cbqalgorithm_
	$self instvar topClass_ topAClass_ topBClass_

	set Mbps 1.5
	set topClass_ [new CBQClass]
	set topAClass_ [new CBQClass]
	set topBClass_ [new CBQClass]

	if { $cbqalgorithm_ == "ancestor-only" } { 
		# For Ancestor-Only link-sharing. 
		# Maxidle should be smaller for AO link-sharing.
		$topClass_ setparams none 0 0.97 auto 8 2 0;# Mbps 1.5
		$topAClass_ setparams $topClass 1 0.69 auto 8 1 0;# Mbps: 1.5
		$topBClass_ setparams $topClass 1 0.29 auto 8 1 0; # Mbps: 1.5
	} else if { $cbqalgorithm_ == "top-level" } {
		# For Top-Level link-sharing?
		# borrowing from $topAClass_ is occuring before from $topClass
		# yellow borrows from $topBClass_
		# Goes green borrow from $topClass_ or from $topAClass?
		# When $topBClass_ is unsatisfied, there is no borrowing from
		#  $topClass_ until a packet is sent from the yellow class.
		$topClass_ setparams none 0 0.97 0.001 8 2 0; #Mbps: 0
		$topAClass_ setparams $topClass 1 0.69 auto 8 1 0;# 1.5 Mbps
		$topBClass_ setparams $topClass 1 0.29 auto 8 1 0;#  1.5 Mbpx
	} else if { $cbqalgorithm_ == "formal" } {
		# For Formal link-sharing
		# The allocated bandwidth can be exact for parent classes.
		$topClass_ setparams none 0 1.0 1.0 8 2 0;# 0
		$topAClass_ setparams $topClass 1 0.7 1.0 8 1 0;# 0
		$topBClass_ setparams $topClass 1 0.3 1.0 8 1 0;# 0
	}

	$self instvar vidAClass_ vidBClass_ dataAClass_ dataBClass_

	set vidAClass_ [new CBQClass]
	$vidAClass_ setparams $topAClass 1 0.3 auto 1 0 0;# $Mbps
	set dataAClass_ [new CBQClass]
 	$dataAClass_ setparams $topAClass 1 0.4 auto 2 0 0;# $Mbps
 	set vidBClass_ [new CBQClass]
 	$vidBClass_ setparams $topBClass 1 0.1 auto 1 0 0;# $Mbps
 	set dataBClass_ [new CBQClass]
 	$dataBClass_ setparams $topBClass 1 0.2 auto 2 0 0;# $Mbps
}

TestSuite instproc insert_twoAgency { cbqlink } {

	$self instvar topClass_ topAClass_ topBClass_
	$self instvar vidAClass_ vidBClass_
	$self instvar dataAClass_ dataBClass_

	$cbqlink insert $topClass_
 	$cbqlink insert $topAClass_
 	$cbqlink insert $topBClass_
	$cbqlink insert $vidAClass_
	$cbqlink insert $vidBClass_
        $cbqlink insert $dataAClass_
        $cbqlink insert $dataBClass_

	$cbqlink bind $vidAClass_ 1
	$cbqlink bind $dataAClass_ 2
	$cbqlink bind $vidBClass_ 3
	$cbqlink bind $dataBClass_ 4
}

# display graph of results
TestSuite instproc finish testname {

puts "running FINISH (cbq)"

	set awkCode  {
	  if ($1 == "maxbytes") maxbytes = $2;
	  if ($2 == class) print $1, $3/maxbytes >> "temp.t"; 
	}
	set awkCodeAll { 
	  if ($2 == class) { print time, sum >> "temp.t"; sum = 0; }
	  if ($1 == "maxbytes") maxbytes = $2;
	  sum += $3/maxbytes;
	  if (NF==3) time = $1; else time = 0;
	}

	set f [open temp.rands w]
	puts $f "TitleText: $testname"
	puts $f "Device: Postscript"
	
	exec rm -f temp.p temp.t
	exec touch temp.p temp.t
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

#
# Save the number of bytes sent by each class in each time interval.
# File: temp.s
#
TestSuite instproc cbrDump4 { linkno interval stopTime maxBytes } {
	$self instvar oldbytes1 oldbytes2 oldbytes3 oldbytes4
	$self instvar ns_

	TestSuite instproc cdump { lnk interval file }  {
	  global oldbytes1 oldbytes2 oldbytes3 oldbytes4
	  set timenow [$ns_ now]
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
	  $ns_ at [expr $timenow + $interval] "cdump $lnk $interval $file"
	}
	set oldbytes1 0
	set oldbytes2 0
        set oldbytes3 0
	set oldbytes4 0

	set f [open temp.s w]
	puts $f "maxbytes $maxBytes"
	$ns_ at 0.0 "cdump $linkno $interval $f"
	$ns_ at $stopTime "close $f"

	TestSuite inst proc cdumpdel { lnk file }  {
	  set timenow [$ns_ now]
	  # format: time, class, delay
	  puts $file "$timenow 1 [$lnk stat 1 mean-qdelay]"
	  puts $file "$timenow 2 [$lnk stat 2 mean-qdelay]"
	  puts $file "$timenow 3 [$lnk stat 3 mean-qdelay]"
	  puts $file "$timenow 4 [$lnk stat 4 mean-qdelay]"
	}
	set f1 [open temp.q w]
	puts $f1 "delay"
	$ns_ at $stopTime "cdumpdel $linkno $f1"
	$ns_ at $stopTime "close $f1"
}

#
# Save the queueing delay seen by each class.
# File: temp.q
# Format: time, class, delay
#
# proc cbrDumpDel { linkno stopTime } {
# 	proc cdump { lnk file }  {
# 	  set timenow [ns now]
# 	  # format: time, class, delay
# 	  puts $file "$timenow 1 [$lnk stat 1 mean-qsize]"
# 	  puts $file "$timenow 2 [$lnk stat 2 mean-qsize]"
# 	  puts $file "$timenow 3 [$lnk stat 3 mean-qsize]"
# 	  puts $file "$timenow 4 [$lnk stat 4 mean-qsize]"
# 	}
# 	set f1 [open temp.q w]
# 	puts $f1 "delay"
# 	ns at $stopTime "cdump $linkno $f1"
# 	ns at $stopTime "close $f1"
# }

#
# Create three CBR connections.
#
TestSuite instproc three_cbrs {} {
	$self instvar ns_ node_
	set cbr1 [$ns_ create-connection CBR $node_(s1) LossMonitor $node_(r1) 1]
	$cbr1 set packetSize_ 190
	$cbr1 set interval_ 0.001

	set cbr2 [$ns_ create-connection CBR $node_(s2) LossMonitor $node_(r1) 2]
	$cbr2 set packetSize_ 500
	$cbr2 set interval_ 0.002

	set cbr3 [$ns_ create-connection CBR $node_(s3) LossMonitor $node_(r1) 3]
	$cbr3 set packetSize_ 1000
	$cbr3 set interval_ 0.005

	$ns_ at 0.0 "$cbr1 start; $cbr2 start; $cbr3 start"
        $ns_ at 4.0 "$cbr3 stop"
        $ns_ at 8.0 "$cbr3 start"
        $ns_ at 12.0 "$cbr2 stop"
        $ns_ at 18.0 "$cbr2 start"
	$ns_ at 20.0 "$cbr1 stop"
        $ns_ at 24.0 "$cbr1 start"
}

TestSuite instproc four_cbrs {} {
	$self instvar ns_ node_
	set cbr1 [$ns_ create-connection CBR $node_(s1) LossMonitor $node_(r1) 1]
	$cbr1 set packetSize_ 190
	$cbr1 set interval_ 0.001

	set cbr2 [$ns_ create-connection CBR $node_(s2) LossMonitor $node_(r1) 2]
	$cbr2 set packetSize_ 1000
	$cbr2 set interval_ 0.005

	set cbr3 [$ns_ create-connection CBR $node_(s3) LossMonitor $node_(r1) 3]
	$cbr3 set packetSize_ 500
	$cbr3 set interval_ 0.002

	set cbr4 [$ns_ create-connection CBR $node_(s3) LossMonitor $node_(r1) 4]
	$cbr4 set packetSize_ 1000
	$cbr4 set interval_ 0.005

	$ns_ at 0.0 "$cbr1 start; $cbr2 start; $cbr3 start; $cbr4 start"
	$ns_ at 12.0 "$cbr1 stop"
	$ns_ at 16.0 "$cbr1 start"
	$ns_ at 36.0 "$cbr1 stop"
	$ns_ at 20.0 "$cbr2 stop"
	$ns_ at 24.0 "$cbr2 start"
	$ns_ at 4.0 "$cbr3 stop"
	$ns_ at 8.0 "$cbr3 start"
	$ns_ at 36.0 "$cbr3 start"
	$ns_ at 28.0 "$cbr4 stop"
	$ns_ at 32.0 "$cbr4 start"
}

Class Test/WRR -superclass TestSuite
Test/WRR instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ cbq1-wrr
	set test_ CBQ_WRR
	$self next; # call ctor for TestSuite now
}

#
# Figure 10 from the link-sharing paper. 
# ~/newr/rm/testB.com
# 
Test/WRR instproc run {} {
	$self instvar cbqalgorithm_ ns_ net_ topo_
	set cbqalgorithm_ top-level
	set stopTime 28.1

	$self create_flat
	$self insert_flat [$topo_ set cbqlink_]
	$self three_cbrs

	$self openTrace $stopTime CBQ_WRR

	$ns_ run
}

##### I AM HERE

Class Test/PRR -superclass TestSuite
Test/PRR instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ cbq1
	set test CBQ_PRR
	$self next
}

#
# Figure 10, but packet-by-packet RR, and Formal.
# 
Test/PRR instproc run {} {
	$self instvar ns_



	global s1 s2 s3 s4 r1 k1 
	set qlen 20
	set stopTime 28.1
	set CBQalgorithm 2
 	$self create_flat [ns link $r1 $k1] $qlen
	$self three_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace2 $stopTime test_cbqPRR

	$ns_ run
}


# Figure 12 from the link-sharing paper.
# WRR, Ancestor-Only link-sharing.
# ~/newr/rm/testA.com
# 
TestSuite instproc test_cbqAO {} {
	global s1 s2 s3 s4 r1 k1 
	set qlen 20
	set stopTime 40.1
	set CBQalgorithm 0
	create_graph $stopTime wrr-cbq $qlen
	create_twoAgency [ns link $r1 $k1] $CBQalgorithm $qlen
	four_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace2 $stopTime test_cbqAO

	ns run
}

#
# Figure 13 from the link-sharing paper.
# WRR, Top link-sharing.
# ~/newr/rm/testA.com
#
TestSuite instproc test_cbqTL {} {
	global s1 s2 s3 s4 r1 k1 
	set qlen 20
	set stopTime 40.1
	set CBQalgorithm 1
	create_graph $stopTime wrr-cbq $qlen
	create_twoAgency [ns link $r1 $k1] $CBQalgorithm $qlen
	four_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace2 $stopTime test_cbqTL

	ns run
}

#
# Figure 11 from the link-sharing paper.
# WRR, Formal (new) link-sharing.
# ~/newr/rm/testA.com
#
TestSuite instproc test_cbqFor {} {
	global s1 s2 s3 s4 r1 k1 
	set qlen 20
	set stopTime 40.1
	set CBQalgorithm 2
	create_graph $stopTime wrr-cbq $qlen
	create_twoAgency [ns link $r1 $k1] $CBQalgorithm $qlen
	four_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace2 $stopTime test_cbqFor

	ns run
}

#
# Figure 11 from the link-sharing paper, but Formal (old) link-sharing.
# WRR. 
# ~/newr/rm/testA.com
#
proc test_cbqForOld {} {
	global s1 s2 s3 s4 r1 k1 
	set qlen 20
	set stopTime 40.1
	set CBQalgorithm 3
	create_graph $stopTime wrr-cbq $qlen
	create_twoAgency [ns link $r1 $k1] $CBQalgorithm $qlen
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
	global s1 s2 s3 s4 r1 k1 ns_link
	set Mbps 1.5
	set stopTime 2.1
	set CBQalgorithm 2
	set ns_link(queue-limit) 1000
	set queue 1000
	create_graph $stopTime cbq $queue

	set link [ns link $r1 $k1]
	set topClass [ns_create_class none none 0.97 1.0 -1.0 8 1 0]
        set audClass [ns_create_class1 $topClass none 0.3 0.25 auto 1 0 0 $Mbps]
	set dataClass [ns_create_class1 $topClass $topClass 0.3 auto auto 2 \
		0 0 $Mbps]

	$link insert $topClass
        $link insert $audClass
	$link insert $dataClass

	set qdisc [$audClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$dataClass qdisc]
	$qdisc set queue-limit $queue

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
	global s1 s2 s3 s4 r1 k1 ns_link
	set Mbps 1.5
	set stopTime 2.1
	set CBQalgorithm 2
	set ns_link(queue-limit) 1000
	set queue 1000
	create_graph $stopTime cbq $queue

	set link [ns link $r1 $k1]
	set topClass [ns_create_class none none 0.97 1.0 -1.0 8 1 0]
	set audClass [ns_create_class1 $topClass none 0.3 0.004 auto 1 0 0 $Mbps]
	set dataClass [ns_create_class1 $topClass $topClass 0.3 auto auto \
		2 0 0 $Mbps]

	$link insert $topClass
        $link insert $audClass
	$link insert $dataClass

	set qdisc [$audClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$dataClass qdisc]
	$qdisc set queue-limit $queue

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
	global s1 s2 s3 s4 r1 k1 ns_link
	set Mbps 1.5
	set stopTime 2.1
	set CBQalgorithm 2
	set ns_link(queue-limit) 1000
	set queue 1000
	create_graph $stopTime cbq $queue

	set link [ns link $r1 $k1]
	set topClass [ns_create_class none none 0.97 1.0 -1.0 8 1 0]
        set audClass [ns_create_class1 $topClass none 0.3 auto auto \
		1 0 0.024 $Mbps]
	set dataClass [ns_create_class1 $topClass $topClass 0.3 auto auto \
		2 0 0 $Mbps]

	$link insert $topClass
        $link insert $audClass
	$link insert $dataClass

	set qdisc [$audClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$dataClass qdisc]
	$qdisc set queue-limit $queue

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
	global s1 s2 s3 s4 r1 k1 ns_link
	set Mbps 1.5
	set stopTime 2.1
	set CBQalgorithm 2
	set ns_link(queue-limit) 1000
	set queue 1000
	create_graph $stopTime cbq $queue

	set link [ns link $r1 $k1]
	set topClass [ns_create_class none none 0.97 1.0 -1.0 8 1 0]
        set audClass [ns_create_class1 $topClass none 0.3 auto auto \
		1 0 0.12 $Mbps]
	set dataClass [ns_create_class1 $topClass $topClass 0.3 auto auto \
		2 0 0 $Mbps]

	$link insert $topClass
        $link insert $audClass
	$link insert $dataClass

	set qdisc [$audClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$dataClass qdisc]
	$qdisc set queue-limit $queue

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
	set queue 20
	set Mbps 1.5
	set stopTime 4.1
	set CBQalgorithm 2
	create_graph $stopTime cbq $queue
 	set link [ns link $r1 $k1]

	set topClass [ns_create_class1 none none 0.98 auto -1.0 8 1 0 $Mbps]
        set audioClass [ns_create_class1 $topClass none 0.03 auto auto \
		1 0 0 $Mbps]
	set vidClass [ns_create_class1 $topClass $topClass \
		0.32 auto auto 1 0 0 $Mbps] 
	set dataClass [ns_create_class1 $topClass $topClass \
		0.65 auto auto 2 0 0 $Mbps]

 	$link insert $topClass
	$link insert $vidClass
 	$link insert $audioClass
        $link insert $dataClass

	set qdisc [$audioClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$vidClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$dataClass qdisc]
	$qdisc set queue-limit $queue

	$link bind $vidClass 2
 	$link bind $audioClass 1
	$link bind $dataClass 3

	three_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	#[ns link $r1 $k1] trace [openTrace3 $stopTime test_Min1]
	openTrace2 $stopTime test_cbqMin1_MinIdle_set

	ns run
}

#
# deleted Min2, which was identical to Min1 except for
# a different value of minidle (which is no longer used)
#


#
# Min3 is like Min1, except extradelay is set to 0.2
#
proc test_cbqMin3 {} {
	global s1 s2 s3 s4 r1 k1 
	set Mbps 1.5
	set queue 20
	set stopTime 4.1
	set CBQalgorithm 2
	create_graph $stopTime cbq $queue
 	set link [ns link $r1 $k1]

	set topClass [ns_create_class1 none none 0.98 auto -1.0 8 1 0 $Mbps]
        set audioClass [ns_create_class1 $topClass none 0.03 auto auto \
		1 0 0.2 $Mbps]
	set vidClass [ns_create_class1 $topClass $topClass \
		0.32 auto auto 1 0 0 $Mbps] 
	set dataClass [ns_create_class1 $topClass $topClass \
		0.65 auto auto 2 0 0 $Mbps]

 	$link insert $topClass
	$link insert $vidClass
 	$link insert $audioClass
        $link insert $dataClass

	set qdisc [$audioClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$vidClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$dataClass qdisc]
	$qdisc set queue-limit $queue

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
	set Mbps 1.5
	set queue 20
	set stopTime 4.1
	set CBQalgorithm 0
	create_graph $stopTime cbq $queue
 	set link [ns link $r1 $k1]

	set topClass [ns_create_class1 none none 0.98 auto -1.0 8 1 0 $Mbps]
        set audioClass [ns_create_class1 $topClass none 0.03 auto auto \
		1 0 0 $Mbps]
	set vidClass [ns_create_class1 $topClass $topClass \
		0.32 auto auto 1 0 0 $Mbps] 
	set dataClass [ns_create_class1 $topClass $topClass \
		0.65 auto auto 2 0 0 $Mbps]

 	$link insert $topClass
	$link insert $vidClass
 	$link insert $audioClass
        $link insert $dataClass

	set qdisc [$audioClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$vidClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$dataClass qdisc]
	$qdisc set queue-limit $queue

	$link bind $vidClass 2
 	$link bind $audioClass 1
	$link bind $dataClass 3

	three_cbrs
	[ns link $r1 $k1] set algorithm $CBQalgorithm

	openTrace2 $stopTime test_cbqMin4_MinIdle_set

	ns run
}

# 
# deleted Min5, which was identical to Min4 except for
# a different value of minidle (which is no longer used)
# 

# 
# 
#
# Min6 is like Min4, except extradelay is set to 0.2
#
proc test_cbqMin6 {} {
	global s1 s2 s3 s4 r1 k1 
	set Mbps 1.5
	set queue 20
	set stopTime 4.1
	set CBQalgorithm 0
	create_graph $stopTime cbq $queue
 	set link [ns link $r1 $k1]

	set topClass [ns_create_class1 none none 0.98 auto -1.0 8 1 0 $Mbps]
        set audioClass [ns_create_class1 $topClass none 0.03 auto auto \
		1 0 0.2 $Mbps]
	set vidClass [ns_create_class1 $topClass $topClass \
		0.32 auto auto 1 0 0 $Mbps] 
	set dataClass [ns_create_class1 $topClass $topClass \
		0.65 auto auto 2 0 0 $Mbps]

 	$link insert $topClass
	$link insert $vidClass
 	$link insert $audioClass
        $link insert $dataClass

	set qdisc [$audioClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$vidClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$dataClass qdisc]
	$qdisc set queue-limit $queue

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
# With Ancestor-Only link-sharing, the top class cannot be
#   allocated 100% of the link bandwidth.
#
proc test_cbqTwoAO {} {
	global s1 s2 s3 s4 r1 k1 
	set queue 20
	set Mbps 1.5
	set stopTime 8.1
	set CBQalgorithm 0
	create_graph $stopTime cbq $queue
	set link [ns link $r1 $k1]

	set topClass [ns_create_class1 none none 0.98 auto -1.0 8 1 0 $Mbps]
        set audioClass [ns_create_class1 $topClass $topClass \
               0.01 auto auto 1 0 0 $Mbps]
	set dataClass [ns_create_class1 $topClass $topClass \
		0.99 auto auto 2 0 0 $Mbps]

 	$link insert $topClass
 	$link insert $audioClass
        $link insert $dataClass

	set qdisc [$audioClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$dataClass qdisc]
	$qdisc set queue-limit $queue

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

# This has a smaller value for maxidle for the root class. 
proc test_cbqTwoAO2 {} {
	global s1 s2 s3 s4 r1 k1 
	set Mbps 1.5
	set stopTime 8.1
	set queue 20
	set CBQalgorithm 0
	create_graph $stopTime cbq $queue
	set link [ns link $r1 $k1]

	set topClass [ns_create_class none none 0.98 0.005 -1.0 8 1 0]
        set audioClass [ns_create_class1 $topClass $topClass \
               0.01 auto auto 1 0 0 $Mbps]
	set dataClass [ns_create_class1 $topClass $topClass \
		0.99 auto auto 2 0 0 $Mbps]


 	$link insert $topClass
 	$link insert $audioClass
        $link insert $dataClass

	set qdisc [$audioClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$dataClass qdisc]
	$qdisc set queue-limit $queue

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

# A higher allocated bandwidth for the root class. 
proc test_cbqTwoAO3 {} {
	global s1 s2 s3 s4 r1 k1 
	set Mbps 1.5
	set stopTime 8.1
	set queue 20
	set CBQalgorithm 0
	create_graph $stopTime cbq $queue
	set link [ns link $r1 $k1]

	set topClass [ns_create_class1 none none 0.99 auto -1.0 8 1 0 $Mbps]
        set audioClass [ns_create_class1 $topClass $topClass \
               0.01 auto auto 1 0 0 $Mbps]
	set dataClass [ns_create_class1 $topClass $topClass \
		0.99 auto auto 2 0 0 $Mbps]

 	$link insert $topClass
 	$link insert $audioClass
        $link insert $dataClass

	set qdisc [$audioClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$dataClass qdisc]
	$qdisc set queue-limit $queue

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
	set Mbps 1.5
	set queue 20
	set stopTime 8.1
	set CBQalgorithm 1
	create_graph $stopTime cbq $queue
	set link [ns link $r1 $k1]

	set topClass [ns_create_class1 none none 1.0 auto -1.0 8 1 0 $Mbps]
        set audioClass [ns_create_class1 $topClass $topClass \
               0.01 auto auto 1 0 0 $Mbps]
	set dataClass [ns_create_class1 $topClass $topClass \
		0.99 auto auto 2 0 0 $Mbps]

 	$link insert $topClass
 	$link insert $audioClass
        $link insert $dataClass

	set qdisc [$audioClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$dataClass qdisc]
	$qdisc set queue-limit $queue

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
	set Mbps 1.5
	set queue 20
	set stopTime 8.1
	set CBQalgorithm 2
	create_graph $stopTime cbq $queue
	set link [ns link $r1 $k1]

	set topClass [ns_create_class1 none none 1.0 auto -1.0 8 1 0 $Mbps]
        set audioClass [ns_create_class1 $topClass $topClass \
               0.01 auto auto 1 0 0 $Mbps]
	set dataClass [ns_create_class1 $topClass $topClass \
		0.99 auto auto 2 0 0 $Mbps]

 	$link insert $topClass
 	$link insert $audioClass
        $link insert $dataClass

	set qdisc [$audioClass qdisc]
	$qdisc set queue-limit $queue
	set qdisc [$dataClass qdisc]
	$qdisc set queue-limit $queue

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

TestSuite runTest

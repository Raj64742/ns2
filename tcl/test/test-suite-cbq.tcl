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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-cbq.tcl,v 1.20 1997/11/11 03:15:55 sfloyd Exp $
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

set dir [pwd]
catch "cd tcl/test"
source misc.tcl
source topologies.tcl
catch "cd $dir"

TestSuite instproc make_queue { cl qlim } {
	$self instvar cbq_qtype_
	set q [new Queue/$cbq_qtype_]
	$q set limit_ $qlim
	$cl install-queue $q
}

TestSuite instproc make_fmon cbqlink {
	$self instvar ns_ fmon_
	set fmon_ [$ns_ makeflowmon Fid]
	$ns_ attach-fmon $cbqlink $fmon_
}

# ~/newr/rm/testB
# Create a flat link-sharing structure.
#
#	3 leaf classes:
#		vidClass	(32%), pri 1
#		audioClass	(03%), pri 1
#		dataClass	(65%), pri 2
#

TestSuite instproc create_flat { audborrowok audxdelay} {
	$self instvar topclass_ vidclass_ audioclass_ dataclass_
	$self instvar cbq_qtype_

	set qlim 20
	set cbq_qtype_ DropTail

	set topclass_ [new CBQClass]
	# (topclass_ doesn't have a queue)
	$topclass_ setparams none 0 0.98 auto 8 2 0

 	set vidclass_ [new CBQClass]
	$self make_queue $vidclass_ $qlim
	$vidclass_ setparams $topclass_ true 0.32 auto 1 1 0

	set audioclass_ [new CBQClass]
	$self make_queue $audioclass_ $qlim
	$audioclass_ setparams $topclass_ $audborrowok 0.03 auto 1 1 $audxdelay

	set dataclass_ [new CBQClass]
	$self make_queue $dataclass_ $qlim
	$dataclass_ setparams $topclass_ true 0.65 auto 2 1 0
}

TestSuite instproc insert_flat cbqlink {
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

TestSuite instproc create_flat2 { audmaxidle audxdelay } {
	$self instvar topclass_ audioclass_ dataclass_
	$self instvar cbq_qtype_

	set qlim 1000
	set cbq_qtype_ DropTail

	set topclass_ [new CBQClass]
	# (topclass_ doesn't have a queue)
	$topclass_ setparams none false 0.97 1.0 8 2 0

	set audioclass_ [new CBQClass]
	$self make_queue $audioclass_ $qlim
	$audioclass_ setparams $topclass_ false 0.30 $audmaxidle 1 1 $audxdelay

	set dataclass_ [new CBQClass]
	$self make_queue $dataclass_ $qlim
	$dataclass_ setparams $topclass_ true 0.30 auto 2 1 0
}

TestSuite instproc create_flat3 { rootbw rootmaxidle } {
	$self instvar topclass_ audioclass_ dataclass_
	$self instvar cbq_qtype_

	set qlim 20
	set cbq_qtype_ DropTail

	set topclass_ [new CBQClass]
	# (topclass_ doesn't have a queue)
	$topclass_ setparams none false $rootbw $rootmaxidle 8 2 0

	set audioclass_ [new CBQClass]
	$self make_queue $audioclass_ $qlim
	$audioclass_ setparams $topclass_ true 0.01 auto 1 1 0

	set dataclass_ [new CBQClass]
	$self make_queue $dataclass_ $qlim
	$dataclass_ setparams $topclass_ true 0.99 auto 2 1 0
}

TestSuite instproc insert_flat2 cbqlink {
	$self instvar topclass_ audioclass_ dataclass_

	#
	# note: auto settings for maxidle are resolved in insert
	# (see tcl/lib/ns-queue.tcl)
	#

 	$cbqlink insert $topclass_
 	$cbqlink insert $audioclass_
        $cbqlink insert $dataclass_

 	$cbqlink bind $audioclass_ 1;# fid 1
	$cbqlink bind $dataclass_ 2; # fid 2
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

	set qlim 20

	set topClass_ [new CBQClass]
	set topAClass_ [new CBQClass]
	set topBClass_ [new CBQClass]

	if { $cbqalgorithm_ == "ancestor-only" } { 
		# For Ancestor-Only link-sharing. 
		# Maxidle should be smaller for AO link-sharing.
		$topClass_ setparams none 0 0.97 auto 8 3 0
		$topAClass_ setparams $topClass_ 1 0.69 auto 8 2 0
		$topBClass_ setparams $topClass_ 1 0.29 auto 8 2 0
	} elseif { $cbqalgorithm_ == "top-level" } {
		# For Top-Level link-sharing?
		# borrowing from $topAClass_ is occuring before from $topClass
		# yellow borrows from $topBClass_
		# Goes green borrow from $topClass_ or from $topAClass?
		# When $topBClass_ is unsatisfied, there is no borrowing from
		#  $topClass_ until a packet is sent from the yellow class.
		$topClass_ setparams none 0 0.97 0.001 8 3 0
		$topAClass_ setparams $topClass_ 1 0.69 auto 8 2 0
		$topBClass_ setparams $topClass_ 1 0.29 auto 8 2 0
	} elseif { $cbqalgorithm_ == "formal" } {
		# For Formal link-sharing
		# The allocated bandwidth can be exact for parent classes.
		$topClass_ setparams none 0 1.0 1.0 8 3 0
		$topAClass_ setparams $topClass_ 1 0.7 1.0 8 2 0
		$topBClass_ setparams $topClass_ 1 0.3 1.0 8 2 0
	}

	$self instvar vidAClass_ vidBClass_ dataAClass_ dataBClass_

	set vidAClass_ [new CBQClass]
	$vidAClass_ setparams $topAClass_ 1 0.3 auto 1 1 0
	set dataAClass_ [new CBQClass]
 	$dataAClass_ setparams $topAClass_ 1 0.4 auto 2 1 0
 	set vidBClass_ [new CBQClass]
 	$vidBClass_ setparams $topBClass_ 1 0.1 auto 1 1 0
 	set dataBClass_ [new CBQClass]
 	$dataBClass_ setparams $topBClass_ 1 0.2 auto 2 1 0

	# (topclass_ doesn't have a queue)
	$self instvar cbq_qtype_
	set cbq_qtype_ DropTail
	$self make_queue $vidAClass_ $qlim
	$self make_queue $dataAClass_ $qlim
	$self make_queue $vidBClass_ $qlim
	$self make_queue $dataBClass_ $qlim
}

TestSuite instproc insert_twoagency cbqlink {

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
	global quiet
	$self instvar tmpschan_ tmpqchan_ topo_
	$topo_ instvar cbqlink_

	set graphfile temp.rands
	set bw [[$cbqlink_ set link_] set bandwidth_]
	set maxbytes [expr $bw / 8.0]

	set awkCode  {
		$2 == fid { print $1, $3/maxbytes }
	}
	set awkCodeAll {
		$2 == fid { print time, sum; sum = 0 }
		{
			sum += $3/maxbytes;
			if (NF==3)
				time = $1;
			else
				time = 0;
		}
	}

	if { [info exists tmpschan_] } {
		close $tmpschan_
	}
	if { [info exists tmpqchan_] } {
		close $tmpqchan_
	}

	set f [open $graphfile w]
	puts $f "TitleText: $testname"
	puts $f "Device: Postscript"
	
	exec rm -f temp.p
	exec touch temp.p
	foreach i { 1 2 3 4 } {
		exec echo "\n\"flow $i" >> temp.p
		exec awk $awkCode fid=$i maxbytes=$maxbytes temp.s > temp.$i
		exec cat temp.$i >> temp.p
		exec echo " " >> temp.p
	}

	exec awk $awkCodeAll fid=1 maxbytes=$maxbytes temp.s > temp.all
	exec echo "\n\"all " >> temp.p  
	exec cat temp.all >> temp.p 

	exec cat temp.p >@ $f
	close $f
	if {$quiet == "false"} {
		exec xgraph -bb -tk -x time -y bandwidth $graphfile &
	}
#	exec csh figure2.com $file
	
	exit 0
}

#
# Save the number of bytes sent by each class in each time interval.
# File: temp.s
#
TestSuite instproc cbrDump4 { linkno interval stopTime maxBytes } {
	set dumpfile temp.s
	$self instvar oldbytes_
	$self instvar ns_

	TestSuite instproc cdump { lnk interval file }  {
	  global quiet
	  $self instvar oldbytes_
	  $self instvar ns_ fmon_
	  set now [$ns_ now]
	  set fcl [$fmon_ classifier]
	  set fids { 1 2 3 4 }

	  if {$quiet == "false"} {
	  	puts "$now"
	  }
	  foreach i $fids {
		  set flow($i) [$fcl lookup auto 0 0 $i]
		  if { $flow($i) != "" } {
			  set bytes($i) [$flow($i) set bdepartures_]
			  puts $file "$now $i [expr $bytes($i) - $oldbytes_($i)]"
			  set oldbytes_($i) $bytes($i)
		  }
	  }
	  $ns_ at [expr $now + $interval] "$self cdump $lnk $interval $file"
	}

	set fids { 1 2 3 4 }
	foreach i $fids {
		set oldbytes_($i) 0
	}

	$self instvar tmpschan_
	set f [open $dumpfile w]
	set tmpschan_ $f
	puts $f "maxbytes $maxBytes"
	$ns_ at 0.0 "$self cdump $linkno $interval $f"

	TestSuite instproc cdumpdel file {
	  $self instvar ns_ fmon_
	  set now [$ns_ now]
	  set fcl [$fmon_ classifier]

	  set fids { 1 2 3 4 }
	  foreach i $fids {
		  set flow [$fcl lookup auto 0 0 $i]
		  if { $flow != "" } {
			  set dsamp [$flow get-delay-samples]
			  puts $file "$now $i [$dsamp mean]"
		  }
	  }
	}
	$self instvar tmpqchan_
	set f1 [open temp.q w]
	set tmpqchan_ $f1
	puts $f1 "delay"
	$ns_ at $stopTime "$self cdumpdel $f1"
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
	set cbr1 [$ns_ create-connection CBR $node_(s1) LossMonitor $node_(r2) 1]
	$cbr1 set packetSize_ 190
	$cbr1 set interval_ 0.001

	set cbr2 [$ns_ create-connection CBR $node_(s2) LossMonitor $node_(r2) 2]
	$cbr2 set packetSize_ 500
	$cbr2 set interval_ 0.002

	set cbr3 [$ns_ create-connection CBR $node_(s3) LossMonitor $node_(r2) 3]
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

#
# Create two CBR connections.
#
TestSuite instproc two_cbrs { ps1 ps2 int1 int2 dostop } {
	$self instvar ns_ node_
	set cbr1 [$ns_ create-connection CBR $node_(s1) LossMonitor $node_(r2) 1]
	$cbr1 set packetSize_ $ps1
	$cbr1 set interval_ $int1

	set cbr2 [$ns_ create-connection CBR $node_(s2) LossMonitor $node_(r2) 2]
	$cbr2 set packetSize_ $ps2
	$cbr2 set interval_ $int2

	$ns_ at 0.0 "$cbr1 start; $cbr2 start"
	if { $dostop } {
		$ns_ at 0.002 "$cbr1 stop"
		$ns_ at 1.0 "$cbr1 start"
		$ns_ at 1.08 "$cbr1 stop"
	}
}

TestSuite instproc four_cbrs {} {
	$self instvar ns_ node_
	set cbr1 [$ns_ create-connection CBR $node_(s1) LossMonitor $node_(r2) 1]
	$cbr1 set packetSize_ 190
	$cbr1 set interval_ 0.001

	set cbr2 [$ns_ create-connection CBR $node_(s2) LossMonitor $node_(r2) 2]
	$cbr2 set packetSize_ 1000
	$cbr2 set interval_ 0.005

	set cbr3 [$ns_ create-connection CBR $node_(s3) LossMonitor $node_(r2) 3]
	$cbr3 set packetSize_ 500
	$cbr3 set interval_ 0.002

	set cbr4 [$ns_ create-connection CBR $node_(s3) LossMonitor $node_(r2) 4]
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
	$ns_ at 36.0 "$cbr3 stop"
	$ns_ at 28.0 "$cbr4 stop"
	$ns_ at 32.0 "$cbr4 start"
}

#
# Figure 10 from the link-sharing paper. 
# ~/newr/rm/testB.com
# 

Class Test/WRR -superclass TestSuite
Test/WRR instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ cbq1-wrr
	set test_ CBQ_WRR
	$self next 0
}

Test/WRR instproc run {} {
	$self instvar cbqalgorithm_ ns_ net_ topo_
	set cbqalgorithm_ top-level
	set stopTime 28.1
	set maxbytes 187500

	$topo_ instvar cbqlink_
	$self create_flat true 0
	$self insert_flat $cbqlink_
	$self three_cbrs
	$self make_fmon $cbqlink_
	[$cbqlink_ queue] algorithm $cbqalgorithm_

	$self cbrDump4 $cbqlink_ 1.0 $stopTime $maxbytes
	$self openTrace $stopTime CBQ_WRR

	$ns_ run
}

Class Test/PRR -superclass TestSuite
Test/PRR instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ cbq1-prr
	set test_ CBQ_PRR
	$self next 0
}

#
# Figure 10, but packet-by-packet RR, and Formal.
# 
Test/PRR instproc run {} {
	$self instvar cbqalgorithm_ ns_ net_ topo_
	set cbqalgorithm_ formal
	set stopTime 28.1
	set maxbytes 187500

	$topo_ instvar cbqlink_
	$self create_flat true 0
	$self insert_flat $cbqlink_
	$self three_cbrs
	$self make_fmon $cbqlink_
	[$cbqlink_ queue] algorithm $cbqalgorithm_

	$self cbrDump4 $cbqlink_ 1.0 $stopTime $maxbytes
	$self openTrace $stopTime CBQ_PRR

	$ns_ run
}


# Figure 12 from the link-sharing paper.
# WRR, Ancestor-Only link-sharing.
# ~/newr/rm/testA.com
# 

Class Test/AO -superclass TestSuite
Test/AO instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ cbq1-wrr
	set test_ CBQ_AO
	$self next 0
}
Test/AO instproc run {} {
	$self instvar cbqalgorithm_ ns_ net_ topo_
	set stopTime 40.1
	set maxbytes 187500
	set cbqalgorithm_ ancestor-only

	$topo_ instvar cbqlink_
	$self create_twoagency
	$self insert_twoagency $cbqlink_
	$self four_cbrs
	$self make_fmon $cbqlink_
	[$cbqlink_ queue] algorithm $cbqalgorithm_

	$self cbrDump4 $cbqlink_ 1.0 $stopTime $maxbytes
	$self openTrace $stopTime CBQ_AO

	$ns_ run
}

#
# Figure 13 from the link-sharing paper.
# WRR, Top link-sharing.
# ~/newr/rm/testA.com
#
Class Test/TL -superclass TestSuite
Test/TL instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ cbq1-wrr
	set test_ CBQ_TL
	$self next 0
}
Test/TL instproc run {} {
	$self instvar cbqalgorithm_ ns_ net_ topo_
	set stopTime 40.1
	set maxbytes 187500
	set cbqalgorithm_ top-level

	$topo_ instvar cbqlink_
	$self create_twoagency
	$self insert_twoagency $cbqlink_
	$self four_cbrs
	$self make_fmon $cbqlink_
	[$cbqlink_ queue] algorithm $cbqalgorithm_

	$self cbrDump4 $cbqlink_ 1.0 $stopTime $maxbytes
	$self openTrace $stopTime CBQ_TL

	$ns_ run
}

### I AM HERE

#
# Figure 11 from the link-sharing paper.
# WRR, Formal (new) link-sharing.
# ~/newr/rm/testA.com
#

Class Test/FORMAL -superclass TestSuite
Test/FORMAL instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ cbq1-wrr
	set test_ CBQ_FORMAL
	$self next 0
}
Test/FORMAL instproc run {} {
	$self instvar cbqalgorithm_ ns_ net_ topo_
	set stopTime 40.1
	set maxbytes 187500
	set cbqalgorithm_ formal

	$topo_ instvar cbqlink_
	$self create_twoagency
	$self insert_twoagency $cbqlink_
	$self four_cbrs
	$self make_fmon $cbqlink_
	[$cbqlink_ queue] algorithm $cbqalgorithm_

	$self cbrDump4 $cbqlink_ 1.0 $stopTime $maxbytes
	$self openTrace $stopTime CBQ_FORMAL

	$ns_ run
}

TestSuite instproc finish_max tname {
	global quiet
        $self instvar ns_ tchan_ testName_
        exec ../../bin/getrc -s 2 -d 3 out.tr | \
          ../../bin/raw2xg -s 0.01 -m 90 -t $tname > temp.rands
        if {$quiet == "false"} {
                exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
        }
        ## now use default graphing tool to make a data file
        ## if so desired

        if { [info exists tchan_] && $quiet == "false" } {
                $self plotQueue $testName_
        }
        $ns_ halt
}


#
# Figure 11 from the link-sharing paper, but Formal (old) link-sharing.
# WRR. 
# ~/newr/rm/testA.com DELETED
#

# 
# To send five back-to-back packets for $audClass,
#   maxidle should be 0.004 seconds
# To send 50 back-to-back packets, maxidle should be 0.25 seconds

Class Test/MAX1 -superclass TestSuite
Test/MAX1 instproc init topo { 
        $self instvar net_ defNet_ test_
	Queue set limit_ 1000
        set net_ $topo
        set defNet_ cbq1-wrr
        set test_ CBQ_MAX1
        $self next 0
}
Test/MAX1 instproc run {} {
        $self instvar cbqalgorithm_ ns_ net_ topo_ node_
        set stopTime 2.1
        set maxbytes 187500
        set cbqalgorithm_ formal
 
        $topo_ instvar cbqlink_
        $self create_flat2 0.25 0
        $self insert_flat2 $cbqlink_
        $self two_cbrs 1000 1000 0.001 0.01 1
        [$cbqlink_ queue] algorithm $cbqalgorithm_

	TestSuite instproc finish tname { $self finish_max $tname }
	$self traceQueues $node_(r1) [$self openTrace $stopTime CBQ_MAX1]
        $ns_ run
}

Class Test/MAX2 -superclass TestSuite
Test/MAX2 instproc init topo { 
        $self instvar net_ defNet_ test_
	Queue set limit_ 1000
        set net_ $topo
        set defNet_ cbq1-wrr
        set test_ CBQ_MAX2
        $self next 0
}
Test/MAX2 instproc run {} {
        $self instvar cbqalgorithm_ ns_ net_ topo_ node_
        set stopTime 2.1
        set maxbytes 187500
        set cbqalgorithm_ formal
 
        $topo_ instvar cbqlink_
        $self create_flat2 0.004 0
        $self insert_flat2 $cbqlink_
        $self two_cbrs 1000 1000 0.001 0.01 1
        [$cbqlink_ queue] algorithm $cbqalgorithm_

	TestSuite instproc finish tname { $self finish_max $tname }
	$self traceQueues $node_(r1) [$self openTrace $stopTime CBQ_MAX2]
        $ns_ run
}

#
# Set "extradelay" to 0.024 seconds for a steady-state burst of 2 
#
Class Test/EXTRA1 -superclass TestSuite
Test/EXTRA1 instproc init topo { 
        $self instvar net_ defNet_ test_
        Queue set limit_ 1000
        set net_ $topo
        set defNet_ cbq1-prr
        set test_ CBQ_EXTRA1
        $self next 0
}
Test/EXTRA1 instproc run {} {
        $self instvar cbqalgorithm_ ns_ net_ topo_ node_
        set stopTime 2.1
        set maxbytes 187500
        set cbqalgorithm_ formal
        
        $topo_ instvar cbqlink_ 
        $self create_flat2 auto 0.024
        $self insert_flat2 $cbqlink_
        $self two_cbrs 1000 1000 0.015 0.01 0
        [$cbqlink_ queue] algorithm $cbqalgorithm_

        TestSuite instproc finish tname { $self finish_max $tname }
        $self traceQueues $node_(r1) [$self openTrace $stopTime CBQ_EXTRA1]
        $ns_ run
}


#
# Set "extradelay" to 0.12 seconds for a steady-state burst of 8 
#

Class Test/EXTRA2 -superclass TestSuite
Test/EXTRA2 instproc init topo { 
        $self instvar net_ defNet_ test_
        Queue set limit_ 1000
        set net_ $topo
        set defNet_ cbq1-prr
        set test_ CBQ_EXTRA2
        $self next 0
}
Test/EXTRA2 instproc run {} {
        $self instvar cbqalgorithm_ ns_ net_ topo_ node_
        set stopTime 2.1
        set maxbytes 187500
        set cbqalgorithm_ formal
        
        $topo_ instvar cbqlink_ 
        $self create_flat2 auto 0.12
        $self insert_flat2 $cbqlink_
        $self two_cbrs 1000 1000 0.015 0.01 0
        [$cbqlink_ queue] algorithm $cbqalgorithm_

        TestSuite instproc finish tname { $self finish_max $tname }
        $self traceQueues $node_(r1) [$self openTrace $stopTime CBQ_EXTRA2]
        $ns_ run
}

# With Packet-by-Packet Round-robin, it is necessary either to
# set a positive value for extradelay, or a negative value for minidle
#
Class Test/MIN1 -superclass TestSuite
Test/MIN1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ cbq1-prr
	set test_ CBQ_MIN1
	$self next 0
}

Test/MIN1 instproc run {} {
	$self instvar cbqalgorithm_ ns_ net_ topo_
	set cbqalgorithm_ formal
	set stopTime 4.1
	set maxbytes 187500

	$topo_ instvar cbqlink_
	$self create_flat false 0
	$self insert_flat $cbqlink_
	$self three_cbrs
	$self make_fmon $cbqlink_
	[$cbqlink_ queue] algorithm $cbqalgorithm_

	$self cbrDump4 $cbqlink_ 1.0 $stopTime $maxbytes
	$self openTrace $stopTime CBQ_MIN1

	$ns_ run
}

#
# Min2 is deprecated
#

#
# Min3 is like Min1, except extradelay is set to 0.2
#
Class Test/MIN3 -superclass TestSuite
Test/MIN3 instproc init topo {
        $self instvar net_ defNet_ test_
        set net_ $topo
        set defNet_ cbq1-prr
        set test_ CBQ_MIN3
        $self next 0
}   

Test/MIN3 instproc run {} {
        $self instvar cbqalgorithm_ ns_ net_ topo_
        set cbqalgorithm_ formal
        set stopTime 4.1
        set maxbytes 187500
    
        $topo_ instvar cbqlink_
        $self create_flat false 0.20
        $self insert_flat $cbqlink_
        $self three_cbrs
        $self make_fmon $cbqlink_
        [$cbqlink_ queue] algorithm $cbqalgorithm_
    
	$self cbrDump4 $cbqlink_ 1.0 $stopTime $maxbytes
        $self openTrace $stopTime CBQ_MIN3
    
        $ns_ run
}   

#
# Min1, but with Ancestor-Only link-sharing.
# 
Class Test/MIN4 -superclass TestSuite
Test/MIN4 instproc init topo {
        $self instvar net_ defNet_ test_
        set net_ $topo
        set defNet_ cbq1-prr  
        set test_ CBQ_MIN4
        $self next 0
}   
    
Test/MIN4 instproc run {} {
        $self instvar cbqalgorithm_ ns_ net_ topo_
        set cbqalgorithm_ ancestor-only
        set stopTime 4.1
        set maxbytes 187500
    
        $topo_ instvar cbqlink_
        $self create_flat false 0
        $self insert_flat $cbqlink_
        $self three_cbrs
        $self make_fmon $cbqlink_
        [$cbqlink_ queue] algorithm $cbqalgorithm_

	$self cbrDump4 $cbqlink_ 1.0 $stopTime $maxbytes
        $self openTrace $stopTime CBQ_MIN4
    
        $ns_ run
}   

#
# Min5 is deprecated
#

# 
#
# Min6 is like Min4, except extradelay is set to 0.2
#
Class Test/MIN6 -superclass TestSuite
Test/MIN6 instproc init topo {
        $self instvar net_ defNet_ test_
        set net_ $topo
        set defNet_ cbq1-prr
        set test_ CBQ_MIN6
        $self next 0
}   
    
Test/MIN6 instproc run {} {
        $self instvar cbqalgorithm_ ns_ net_ topo_
        set cbqalgorithm_ ancestor-only
        set stopTime 4.1
        set maxbytes 187500
    
        $topo_ instvar cbqlink_
        $self create_flat false 0.20
        $self insert_flat $cbqlink_
        $self three_cbrs
        $self make_fmon $cbqlink_
        [$cbqlink_ queue] algorithm $cbqalgorithm_
    
   	$self cbrDump4 $cbqlink_ 1.0 $stopTime $maxbytes
        $self openTrace $stopTime CBQ_MIN6
    
        $ns_ run
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
Class Test/TwoAO -superclass TestSuite
Test/TwoAO instproc init topo {
        $self instvar net_ defNet_ test_
        set net_ $topo
        set defNet_ cbq1-prr
        set test_ CBQ_TwoAO
        $self next 0
}   
Test/TwoAO instproc run {} {
        $self instvar cbqalgorithm_ ns_ net_ topo_ node_
        set stopTime 8.1
        set maxbytes 187500
        set cbqalgorithm_ ancestor-only
    
        $topo_ instvar cbqlink_
        $self create_flat3 0.98 1.0
        $self insert_flat2 $cbqlink_
        $self two_cbrs 190 500 0.001 0.002 0
	$self make_fmon $cbqlink_
        [$cbqlink_ queue] algorithm $cbqalgorithm_

	$self cbrDump4 $cbqlink_ 1.0 $stopTime $maxbytes
        $self openTrace $stopTime CBQ_TwoAO
    
        $ns_ run
}   

# This has a smaller value for maxidle for the root class. 
Class Test/TwoAO2 -superclass TestSuite
Test/TwoAO2 instproc init topo {
        $self instvar net_ defNet_ test_
        set net_ $topo
        set defNet_ cbq1-prr
        set test_ CBQ_TwoAO2
        $self next 0
}   
Test/TwoAO2 instproc run {} {
        $self instvar cbqalgorithm_ ns_ net_ topo_ node_
        set stopTime 8.1
        set maxbytes 187500
        set cbqalgorithm_ ancestor-only
    
        $topo_ instvar cbqlink_
        $self create_flat3 0.98 0.005
        $self insert_flat2 $cbqlink_
        $self two_cbrs 190 500 0.001 0.002 0 
	$self make_fmon $cbqlink_
        [$cbqlink_ queue] algorithm $cbqalgorithm_
    
	$self cbrDump4 $cbqlink_ 1.0 $stopTime $maxbytes
        $self openTrace $stopTime CBQ_TwoAO2
    
        $ns_ run
}   

Class Test/TwoAO3 -superclass TestSuite
Test/TwoAO3 instproc init topo {
        $self instvar net_ defNet_ test_
        set net_ $topo
        set defNet_ cbq1-prr
        set test_ CBQ_TwoAO3
        $self next 0
}   
Test/TwoAO3 instproc run {} {
        $self instvar cbqalgorithm_ ns_ net_ topo_ node_
        set stopTime 8.1
        set maxbytes 187500
        set cbqalgorithm_ ancestor-only
    
        $topo_ instvar cbqlink_
        $self create_flat3 0.99 auto
        $self insert_flat2 $cbqlink_
        $self two_cbrs 190 500 0.001 0.002 0 
	$self make_fmon $cbqlink_
        [$cbqlink_ queue] algorithm $cbqalgorithm_
    
	$self cbrDump4 $cbqlink_ 1.0 $stopTime $maxbytes
        $self openTrace $stopTime CBQ_TwoAO3
    
        $ns_ run
}   

#
# With Formal link-sharing, the dataClass gets most of the bandwidth.
# With Ancestor-Only link-sharing, the audioClass generally gets
#   to borrow from the not-overlimit root class.
# With Top-Level link-sharing, the audioClass is often blocked
#   from borrowing by an underlimit dataClass
#

Class Test/TwoTL -superclass TestSuite
Test/TwoTL instproc init topo {
        $self instvar net_ defNet_ test_
        set net_ $topo
        set defNet_ cbq1-prr
        set test_ CBQ_TwoTL
        $self next 0
}   
Test/TwoTL instproc run {} {
        $self instvar cbqalgorithm_ ns_ net_ topo_ node_
        set stopTime 8.1
        set maxbytes 187500
        set cbqalgorithm_ top-level
    
        $topo_ instvar cbqlink_
        $self create_flat3 1.0 auto
        $self insert_flat2 $cbqlink_
        $self two_cbrs 190 500 0.001 0.002 0 
	$self make_fmon $cbqlink_
        [$cbqlink_ queue] algorithm $cbqalgorithm_
    
	$self cbrDump4 $cbqlink_ 1.0 $stopTime $maxbytes
        $self openTrace $stopTime CBQ_TwoTL
    
        $ns_ run
}   

#
# With Formal link-sharing, the dataClass gets most of the bandwidth.
# With Ancestor-Only link-sharing, the audioClass generally gets
#   to borrow from the not-overlimit root class.
# With Top-Level link-sharing, the audioClass is often blocked
#   from borrowing by an underlimit dataClass
#
Class Test/TwoF -superclass TestSuite
Test/TwoF instproc init topo {
        $self instvar net_ defNet_ test_
        set net_ $topo
        set defNet_ cbq1-prr
        set test_ CBQ_TwoF
        $self next 0
}   
Test/TwoF instproc run {} {
        $self instvar cbqalgorithm_ ns_ net_ topo_ node_
        set stopTime 8.1
        set maxbytes 187500
        set cbqalgorithm_ formal
    
        $topo_ instvar cbqlink_
        $self create_flat3 1.0 auto
        $self insert_flat2 $cbqlink_
        $self two_cbrs 190 500 0.001 0.002 0 
	$self make_fmon $cbqlink_
        [$cbqlink_ queue] algorithm $cbqalgorithm_
    
	$self cbrDump4 $cbqlink_ 1.0 $stopTime $maxbytes
        $self openTrace $stopTime CBQ_TwoF
    
        $ns_ run
}   

TestSuite runTest

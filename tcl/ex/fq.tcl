#
# This file contains a preliminary cut at fair-queueing for ns
# as well as a number of stubs for Homework 3 in CS268.
#
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/ex/fq.tcl,v 1.7 1997/04/09 00:10:10 kannan Exp $
#

set ns [new Simulator]

# override built-in link allocator
$ns proc simplex-link { n1 n2 bw delay type } {

	$self instvar link_ queueMap_ nullAgent_
	set sid [$n1 id]
	set did [$n2 id]
	if [info exists queueMap_($type)] {
		set type $queueMap_($type)
	}
	if { $type == "FQ" } {
		set link_($sid:$did) [new FQLink $n1 $n2 $bw $delay $nullAgent_]
	} else {
		set q [new Queue/$type]
		$q drop-target $nullAgent_
		set link_($sid:$did) [new SimpleLink $n1 $n2 $bw $delay $q]
	}

	$n1 add-neighbor $n2

	#XXX yuck
	if { $type == "RED" } {
	 	set bw [[$link_($sid:$did) set link_] set bandwidth_]
		$q set ptc_ [expr $bw / (8. * [$q set mean_pktize_])]
	}
}

Class Classifier/Flow/FQ -superclass Classifier/Flow

Classifier/Flow/FQ instproc no-slot flowID {
	$self instvar fq_
	$fq_ new-flow $flowID
}

Class FQLink -superclass Link

FQLink instproc init { src dst bw delay nullAgent } {
	$self next $src $dst
	$self instvar link_ queue_ head_ toNode_ ttl_ classifier_ \
		nactive_ drpT_
	set drpT_ $nullAgent
	set nactive_ 0
	set queue_ [new Queue/FQ]
	set link_ [new DelayLink]
	$link_ set bandwidth_ $bw
	$link_ set delay_ $delay

	set classifier_ [new Classifier/Flow/FQ]
	$classifier_ set fq_ $self

	$queue_ target $link_
	$queue_ drop-target $nullAgent
	$link_ target [$toNode_ entry]

	set head_ $classifier_

	# XXX
	# put the ttl checker after the delay
	# so we don't have to worry about accounting
	# for ttl-drops within the trace and/or monitor
	# fabric
	#
	set ttl_ [new TTLChecker]
	$ttl_ target [$link_ target]
	$link_ target $ttl_

	$queue_ set secsPerByte_ [expr 8.0 / [$link_ set bandwidth_]]
}

Queue set limit_ 10

FQLink set queueManagement_ RED
FQLink set queueManagement_ DropTail

FQLink instproc new-flow flowID {
	$self instvar classifier_ nactive_ queue_ link_ drpT_
	incr nactive_

	set type [$class set queueManagement_]
	set q [new Queue/$type]

	#XXX yuck
	if { $type == "RED" } {
	 	set bw [$link_ set bandwidth_]
		$q set ptc_ [expr $bw / (8. * [$q set mean_pktsize_])]
	}
	$q drop-target $drpT_

	$classifier_ install $flowID $q
	$q target $queue_
	$queue_ install $flowID $q
}

#XXX ask Kannan why this isn't in otcl base class.
FQLink instproc up? { } {
	return up
}

#
# Build trace objects for this link and
# update the object linkage
#
FQLink instproc trace { ns f } {
	$self instvar enqT_ deqT_ drpT_ queue_ link_ head_ fromNode_ toNode_
	set enqT_ [$ns create-trace Enque $f $fromNode_ $toNode_]
	set deqT_ [$ns create-trace Deque $f $fromNode_ $toNode_]
	set drpT_ [$ns create-trace Drop $f $fromNode_ $toNode_]
	$drpT_ target [$queue_ drop-target]
	$queue_ drop-target $drpT_

	$deqT_ target [$queue_ target]
	$queue_ target $deqT_

	$enqT_ target $head_
	set head_ $enqT_
}

#
# Insert objects that allow us to monitor the queue size
# of this link.  Return the name of the object that
# can be queried to determine the average queue size.
#
FQLink instproc init-monitor ns {
	puts stderr "FQLink::init-monitor not implemented"
}

#Queue/RED set thresh_ 3
#Queue/RED set maxthresh_ 8

proc build_topology { ns which } {
	foreach i "0 1 2 3" {
		global n$i
		set n$i [$ns node]
	}
	$ns duplex-link $n0 $n2 5Mb 2ms DropTail
	$ns duplex-link $n1 $n2 5Mb 10ms DropTail
	if { $which == "FIFO" } {
		$ns duplex-link $n2 $n3 1.5Mb 10ms DropTail
	} elseif { $which == "RED" } {
		$ns duplex-link $n2 $n3 1.5Mb 10ms RED
	} else {
		$ns duplex-link $n2 $n3 1.5Mb 10ms FQ
	}
}

proc build_tcp { from to startTime } {
	global ns
	set tcp [new Agent/TCP]
	set sink [new Agent/TCPSink]
	$ns attach-agent $from $tcp
	$ns attach-agent $to $sink
	$ns connect $tcp $sink
	set ftp [new Source/FTP]
	$ftp set agent_ $tcp
	$ns at $startTime "$ftp start"
	return $tcp
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
			    ($5 == "tcp"))\
					print $2, $8 + ($11 % 90) * 0.01
		}
	} out.tr > temp.p
	exec awk {
		{
			if ($1 == "d")
				print $2, $8 + ($11 % 90) * 0.01
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

	# dump the highest seqno sent of each tcp agent
	# this gives an idea of throughput
	set k 1
	while 1 {
		global tcp$k
		if [info exists tcp$k] {
			set tcp [set tcp$k]
			puts "tcp$k seqno [$tcp set t_seqno_]"
		} else {
			break
		}
		incr k
	}
	exit 0
}

set f [open out.tr w]
$ns trace-all $f

build_topology $ns FQ

set tcp1 [build_tcp $n0 $n3 0.1]
$tcp1 set class_ 1
set tcp2 [build_tcp $n1 $n3 0.1]
$tcp2 set class_ 2

$ns at 40.0 "finish Output"
#$ns at 8.0 "xfinish"

proc xfinish {} {
	global ns f
	$ns flush-trace
	close $f

	puts "converting output to nam format..."
	exec awk -f ../nam-demo/nstonam.awk out.tr > fq-nam.tr 
	exec rm -f out
	#XXX
	puts "running nam..."
	exec nam fq-nam &
	exit 0
}

$ns run



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
		$q set ptc_ [expr $bw / (8. * [$q set meanPacketSize_])]
	}
}

Class Classifier/Flow/FQ -superclass Classifier/Flow

Classifier/Flow/FQ instproc no-slot flowID {
	$self instvar fq_
	puts "no-slot $flowID (fq $fq_)"
	$fq_ new-flow $flowID
	#XXX
#	exit 1
}

Class FQLink -superclass Link

FQLink instproc init { src dst bw delay nullAgent } {
	$self next $src $dst
	$self instvar link_ queue_ head_ toNode_ ttl_ classifier_ \
		nactive_ drpT_
	set drpT_ $nullAgent
	set nactive_ 0
	set queue_ [new Queue/FQ]
	set link_ [new Delay/Link]
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

	$self update-cost
}

FQLink instproc update-cost {} {
	$self instvar nactive_ link_ queue_
	if { $nactive_ > 0 } {
		set bw [$link_ set bandwidth_]
		$queue_ set costPerByte_ [expr 8.0 / $bw * $nactive_]
	}
}

FQLink instproc new-flow flowID {
	$self instvar classifier_ nactive_ queue_ link_ drpT_
	incr nactive_
	$self update-cost
	set q [new Queue/RED]
	#XXX yuck
 	set bw [$link_ set bandwidth_]
	$q set ptc_ [expr $bw / (8. * [$q set meanPacketSize_])]
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

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out.tr w]
$ns trace-all $f

$ns duplex-link $n0 $n2 5Mb 2ms DropTail
$ns duplex-link $n1 $n2 5Mb 2ms DropTail
$ns duplex-link $n2 $n3 1.5Mb 10ms FQ

set cbr0 [new Agent/CBR]
$ns attach-agent $n0 $cbr0
$cbr0 set packetSize_ 250
$cbr0 set interval_ 3ms

set cbr1 [new Agent/CBR]
$ns attach-agent $n1 $cbr1
$cbr1 set class_ 1
$cbr1 set packetSize_ 550
$cbr1 set interval_ 3ms

set null0 [new Agent/Null]
$ns attach-agent $n3 $null0

set null1 [new Agent/Null]
$ns attach-agent $n3 $null1

$ns connect $cbr0 $null0
$ns connect $cbr1 $null1

$ns at 1.0 "$cbr0 start"
$ns at 1.1 "$cbr1 start"

#set tcp [new Agent/TCP]
#$tcp set class_ 2
#set sink [new Agent/TCPSink]
#$ns attach-agent $n0 $tcp
#$ns attach-agent $n3 $sink
#$ns connect $tcp $sink
#set ftp [new Source/FTP]
#$ftp set agent_ $tcp
#$ns at 1.2 "$ftp start"

#$ns at 1.35 "$ns detach-agent $n0 $tcp ; $ns detach-agent $n3 $sink"

puts [$cbr0 set packetSize_]
puts [$cbr0 set interval_]

$ns at 3.0 "finish"

proc finish {} {
	global ns f
	$ns flush-trace
	close $f

	puts "converting output to nam format..."
	exec awk -f ../nam-demo/nstonam.awk out.tr > simple-nam.tr 
	exec rm -f out
	#XXX
	puts "running nam..."
	exec nam simple-nam &
	exit 0
}

$ns run


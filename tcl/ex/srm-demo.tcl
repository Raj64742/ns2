#
# This file contains contrived scenarios and protocol agents
# to illustrate the basic srm suppression algorithms.
# It is not an srm implementation.
#
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/ex/srm-demo.tcl,v 1.1 1997/03/18 20:14:05 mccanne Exp $
#
source timer.tcl

set ns [new MultiSim]

# cause ACKs to get dropped
Queue set limit_ 6

foreach k "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14" {
	 set node($k) [$ns node]
}

set f [open out.tr w]
$ns trace-all $f

proc makelinks { bw delay pairs } {
	global ns node
	foreach p $pairs {
		set src $node([lindex $p 0])
		set dst $node([lindex $p 1])
		$ns duplex-link $src $dst $bw $delay DropTail
	}
}

makelinks 1.5Mb 10ms {
	{ 9 0 }
	{ 9 1 }
	{ 9 2 }
	{ 10 3 }
	{ 10 4 }
	{ 10 5 }
	{ 11 6 }
	{ 11 7 }
	{ 11 8 }
}

makelinks 1.5Mb 40ms {
	{ 12 9 }
	{ 12 10 }
	{ 12 11 }
}

makelinks 1.5Mb 10ms {
	{ 13 12 } 
}

makelinks 1.5Mb 50ms {
	{ 14 12 }
}

Class Agent/Message/MC_Acker -superclass Agent/Message
Agent/Message/MC_Acker set packetSize_ 800

Agent/Message/MC_Acker instproc recv msg {
	set from [lindex $msg 0]
	set seqno [lindex $msg 1]
	$self set dst_ $from
	$self send "ack/$from/$seqno"
}

Class Agent/Message/MC_Sender -superclass Agent/Message

Agent/Message/MC_Sender instproc recv msg {
}

Agent/Message/MC_Sender instproc init {} {
	$self next
	$self set seqno_ 1
}

Agent/Message/MC_Sender instproc send-pkt {} {
	$self instvar seqno_ addr_
	$self send "$addr_ $seqno_"
	incr seqno_
}

set sndr [new Agent/Message/MC_Sender]
$sndr set packetSize_ 1400
$sndr set dst_ 0x8002
$sndr set class_ 1

foreach k "0 1 2 3 4 5 6 7 8" {
	set rcvr($k) [new Agent/Message/MC_Acker]
	$ns attach-agent $node($k) $rcvr($k)
	$rcvr($k) set class_ 2
	$ns at 1.0 "$node($k) join-group $rcvr($k) 0x8002"
}

$ns attach-agent $node(14) $sndr

$ns at 1.05 "$sndr send-pkt"
$ns at 1.08 "$sndr send-pkt"
$ns at 1.11 "$sndr send-pkt"
$ns at 1.14 "$sndr send-pkt"

$ns at 3.0 "finish"

proc finish {} {
	global ns f
	$ns flush-trace
	close $f

	puts "converting output to nam format..."
	exec awk -f ../nam-demo/nstonam.awk out.tr > srm-demo-nam.tr 
	exec rm -f out
	#XXX
	puts "running nam..."
	exec ../../../nam/nam srm-demo-nam &
	exit 0
}

$ns run


#
# Copyright (c) Xerox Corporation 1997. All rights reserved.
#
# License is granted to copy, to use, and to make and to use derivative
# works for research and evaluation purposes, provided that Xerox is
# acknowledged in all documentation pertaining to any such copy or
# derivative work. Xerox grants no other licenses expressed or
# implied. The Xerox trade name should not be used in any advertising
# without its written permission. 
#
# XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
# MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
# FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
# express or implied warranty of any kind.
#
# These notices must be retained in any copies of any part of this
# software. 
#
#@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/ex/test-rcvr.tcl,v 1.1 1998/04/25 00:57:49 bajaj Exp $

#This script demonstrates support for two different delay-adaptive receiver types,
#namely the vat-receiver adaptation algorithm and a conservative adapatation 
#algotrithm (that tries to achieve low jitter at the expense of high playback delay)

#IT creates a simple 2 node topology with a link capacity of 2.0Mb.
#The background traffic on the link comprises of an aggregrate of 40 expo on/off 
#sources with peak rate of 80K and on/off time of 500ms
#The experimental traffic constitutes a single expo on/off source with a peak rate 
#of 80k and on/offtime of 500ms

set sources 40
set rcvrType VatRcvr
set bw 2.0Mb

#propagation delay
set delay 0.01ms
#simulation time
set runtime 100   
#pick up a random seed
set seed 1973272912
set bgsrcType expo
set out [open out.tr w]
set rate 80k
set btime 500ms 
set itime 500ms


#set up a queue limit so that no packets are dropped
Queue set limit_ 1000000

set rcvrType [lindex $argv 0]

if { ($rcvrType != "VatRcvr") && ($rcvrType != "ConsRcvr") } {
	puts "Usage : ns test.rcvr.tcl <rcvrtype>"
	puts "<rcvrtype> = \"VatRcvr\" | \"ConsRcvr\" "
	exit 1
} 

#puts stderr $out

ns-random $seed
$defaultRNG seed $seed

Agent instproc print-delay-stats {sndtime now plytime } {
	global out
	puts $out "$sndtime $now $plytime"
	flush $out
}


Simulator instproc create-receiver {node rcvrType} {
    set v [new Agent/$rcvrType]
    $self attach-agent $node $v
    return $v
}

Simulator instproc create-sender {node rate rcvr fid sndType } {
	global runtime btime itime
	set s [new Agent/CBR/UDP]
	$self attach-agent $node $s
	#Create exp on/off source
	if { $sndType == "expo" } {
		set tr [new Traffic/Expoo]
		$tr set packet-size 200
		$tr set burst-time $btime
		$tr set idle-time $itime
		$tr set rate $rate
	} else {    
		#create victrace source
		set tfile [new Tracefile]
		$tfile filename "vic.32.200"
		set tr [new Traffic/Trace]
		$tr attach-tracefile $tfile

	}
	
	$s attach-traffic $tr
	$s set fid_ $fid
	
	$self connect $s $rcvr
	$self at [expr double([ns-random] % 10000000) / 1e7]  "$s start"
	$self at $runtime "$s stop"
}

proc finish {} {
	global rcvrType
	set f [open temp.rands w]
	puts $f "TitleText: $rcvrType"
	puts $f "Device: Postscript"

	exec rm -f temp.p1 temp.p2

	exec awk {
		{
		print $2,$2-$1
		}
	} out.tr > temp.p1
	exec awk {
		{
		print $2,$3-$1
		}
	} out.tr > temp.p2
	puts $f [format "\n\"Network Delay"]
	flush $f
	exec cat temp.p1 >@ $f
	flush $f
	puts $f [format "\n\"Network+Playback Delay"]
	flush $f
	exec cat temp.p2 >@ $f
	flush $f
	close $f
	
	exec rm -f temp.p1 temp.p2
	exec xgraph -bb -tk -nl -m -x simtime -y delay temp.rands &

	exec rm -f out.tr
	exit 0
} 


set ns [new Simulator]

set n1 [$ns node]
set n2 [$ns node]


$ns duplex-link $n1 $n2 $bw $delay DropTail

set v0 [$ns create-receiver $n2 $rcvrType]

set v2 [new Agent/Null]
$ns attach-agent $n2 $v2

set i 0
while { $i < $sources } {
    $ns create-sender $n1 $rate $v2 0 $bgsrcType
    incr i
}

$ns create-sender $n1 $rate $v0 0 expo

$ns at [expr $runtime+1] "finish"
$ns run


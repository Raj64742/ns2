#
# simulator for router mechanisms
#

source flow-link.tcl

set goodslot 0
set badslot 1

#
# create:
#
# S1		 S3
#   \		 /
#    \		/
#    R1========R2
#    /		\
#   /		 \
# S2		 S4
#
# - 10Mb/s, 3ms, drop-tail
# = 1.5Mb/s, 20ms, CBQ
#
proc create_topology tf {
	global ns
	global s1 s2 s3 s4
	global r1 r2

	set s1 [$ns node]
	set s2 [$ns node]
	set s3 [$ns node]
	set s4 [$ns node]

	set r1 [$ns node]
	set r2 [$ns node]

	$ns duplex-link $s1 $r1 10Mb 2ms DropTail
	$ns duplex-link $s2 $r1 10Mb 3ms DropTail
	$ns simplex-link $r1 $r2 1.5Mb 20ms CBQ
	set cbqlink [$ns link $r1 $r2]
	$ns simplex-link $r2 $r1 1.5Mb 20ms RED
	[[$ns link $r2 $r1] queue] set limit_ 25
	$ns duplex-link $s3 $r2 10Mb 4ms DropTail
	$ns duplex-link $s4 $r2 10Mb 5ms DropTail

#	$ns trace-queue $n2 $n3 $tf
	return $cbqlink
}

#
# set up the CBQ classifier for 2 classes (good and bad)
# set default to the good class
# also, put snoop queues between the classifier and
# the cbq class
#
proc class_bindings { cbqlink garrsnoop barrsnoop } {
	global goodcl badcl
	global goodslot badslot

	set classifier [$cbqlink classifier]
	$classifier install $goodslot $goodcl
	$classifier set default_ $goodslot

	if { $garrsnoop != "" && $garrsnoop != "none" } {
		$classifier install $goodslot $garrsnoop
		$garrsnoop target $goodcl
	}
	if { $barrsnoop != "" && $barrsnoop != "none" } {
		$classifier install $badslot $barrsnoop
		$barrsnoop target $badcl
	}
}

proc set_red_params { redq psize qlim bytes wait } {
	$redq set mean_pktsize_ $psize
	$redq set limit_ $qlim
	$redq set bytes_ $bytes
	$redq set wait_ $wait
}

#
# create the CBQ classes and their queues
# 	install queues into classes
# 	install classes into CBQ
#	set cbq class params
#
proc create_cbqclasses { cbqlink fm qsz } {
	global ns goodcl badcl

	set cbq [$cbqlink queue]
	set rootcl [new CBQClass]
	set badcl [new CBQClass]
	set goodcl [new CBQClass]

	set badq [new Queue/RED]
	set goodq [new Queue/RED]
	$badq link [$cbqlink link]
	$goodq link [$cbqlink link]
	set_red_params $badq 1000 $qsz true false
	set_red_params $goodq 1000 $qsz true false

	$badcl install-queue $badq
	$goodcl install-queue $goodq

	$badcl setparams $rootcl true 0.0 0.004 1 1 0
	$goodcl setparams $rootcl true 0.98 0.004 1 1 0
	$rootcl setparams none true 0.98 0.004 1 1 0

	$cbqlink insert $rootcl
	$cbqlink insert $badcl
	$cbqlink insert $goodcl

	#
	# code to splice in a set of snoopdrop things
	#
	set btarget [$badq drop-target]
	set gtarget [$goodq drop-target]

	set dsnoop [new SnoopQueue/Drop]
	$dsnoop set-monitor $fm
	$badq drop-target $dsnoop
	$dsnoop target $btarget

	set dsnoop [new SnoopQueue/Drop]
	$dsnoop set-monitor $fm
	$goodq drop-target $dsnoop
	$dsnoop target $gtarget

	set edsnoop [new SnoopQueue/EDrop]
	$edsnoop set-monitor $fm
	$badq early-drop-target $edsnoop
	$edsnoop target $btarget

	set edsnoop [new SnoopQueue/EDrop]
	$edsnoop set-monitor $fm
	$goodq early-drop-target $edsnoop
	$edsnoop target $gtarget
}

proc create_source { src sink fid } {
	global ns

	set cbr1s [new Agent/CBR]
	$cbr1s set fid_ $fid
	$ns attach-agent $src $cbr1s

	set cbr1r [new Agent/LossMonitor]
	$ns attach-agent $sink $cbr1r
	$ns connect $cbr1s $cbr1r
	return $cbr1s
}

proc create_tcp_source { src sink fid win pktsize } {
	global ns

	set srctype TCP/Reno
	set sinktype TCPSink
	set tcp [$ns create-connection $srctype $src $sinktype $sink $fid]
	if {$pktsize > 0} {
		$tcp set packetSize_ $pktsize
	}
	if {$win > 0} {
		$tcp set window_ $win
	}
	return [$tcp attach-source FTP]
}
proc printflow f {
	puts "flow $f: epdrops: [$f set epdrops_]; ebdrops: [$f set ebdrops_]; pdrops: [$f set pdrops_]; bdrops: [$f set bdrops_]"
}

proc dumpflows { fm interval } {
	global ns
	proc dump { f interval } {
		global ns
		$ns at [expr [$ns now] + $interval] "dump $f $interval"
		$f dump
		$f reset
		#set kf [$f flows]
		#puts "flows known: $kf"
		#foreach ff $kf {
		#	printflow $ff
		#}
	}
	$ns at 0.0 "dump $fm $interval"
}


proc create_flowmon fch {
	global ns

	set flowmon [new QueueMonitor/ED/Flowmon]
	set cl [new Classifier/Hash/SrcDestFid 33]
	$cl proc unknown-flow { src dst fid hashbucket } {
		global ns
		set fdesc [new QueueMonitor/ED/Flow]
		set slot [$self installNext $fdesc]
puts "[$ns now]: (self:$self) installing flow $fdesc (s:$src,d:$dst,f:$fid) in buck: $hashbucket, slot >$slot<"
		$self set-hash $hashbucket $src $dst $fid $slot
	}

	$flowmon classifier $cl
	$flowmon attach $fch
	return $flowmon
}

#
# awk code used to produce:
#       x axis: # arrivals for this flow+category / # total arrivals [bytes]
#       y axis: # drops for this flow+category / # drops this category [pkts]
proc unforcedmakeawk { } {
        set awkCode {
            BEGIN { print "\"flow 0" }
            {
                if ($2 != prev) {
                        print " "; print "\"flow " $2; print 100.0 * $9/$13, 100.0 * $10 / $14; prev = $2
                } else
                        print 100.0 * $9 / $13, 100.0 * $10 / $14
            }
        }
        return $awkCode
}
#
# awk code used to produce:
#       x axis: # arrivals for this flow+category / # total arrivals [bytes]
#       y axis: # drops for this flow+category / # drops this category [bytes]
proc forcedmakeawk { } {
        set awkCode {
            BEGIN { print "\"flow 0" }
            {
                if ($2 != prev) {
                        print " "; print "\"flow " $2; print 100.0 * $9/$13, 100.0 * $11 / $15; prev = $2
                } else
                        print 100.0 * $9 / $13, 100.0 * $11 / $15
            }
        }
        return $awkCode
}


proc finish { tf ff } {
	close $tf
	close $ff
	puts "simulation complete"
	exit 0
}

proc sim1 {} {
	global ns

	global s1 s2 s3 s4
	global r1 r2

	puts "starting sim1"

	set start1 1.0
	set start2 1.2
	set start3 1.4
	set endsim 300.0

	set ns [new Simulator]

	set tracef [open out.tr w]
	set cbqlink [create_topology $tracef]

	set flowf [open flow.tr w]
	set flowmon [create_flowmon $flowf]
	dumpflows $flowmon 1.0

	create_cbqclasses $cbqlink $flowmon 100

	set arrivalsnooper [new SnoopQueue/In]
	class_bindings $cbqlink $arrivalsnooper none
	$arrivalsnooper set-monitor $flowmon

	set src1 [create_tcp_source $s1 $s3 1 100 1000]
	set src2 [create_tcp_source $s2 $s4 2 100 50]
	set src3 [create_source $s1 $s4 3]
		$src3 set packetSize_ 190
		$src3 set interval_ 0.003

	$ns at $start1 "$src1 start"
	$ns at $start2 "$src2 start"
	$ns at $start3 "$src3 start"
	$ns at $endsim "finish $tracef $flowf"
	$ns run
}
sim1

## Simple Bi-directional Shared Tree multicast test
# on a binary tree

set ns [new Simulator -multicast on]

set f [open out-mc8.tr w]
$ns trace-all $f
set nf [open out-mc8.nam w]
$ns namtrace-all $nf

set degree 2 ;#binary
set depth  4 ;
set n(0) [$ns node]
set nidx 1
for {set l 1} {$l<$depth} {incr l} {
	set nodes_at_level [expr pow($degree, $l)]
	for {set k 1} {$k <= $nodes_at_level} {incr k} {
		#create new node
		eval set n($nidx) [$ns node]
		#link it to the parent
		set p [expr ($nidx - 1)/$degree]
		$ns duplex-link $n($p) $n($nidx) 1.5Mb [expr $depth*10/pow(2,$l)]ms DropTail
		#orient the link
		# parent p has children [$p*$degree+1..($p+1)*$degree]
		# so middle point is $p*$degree+($degree+1)/2
		set mp [expr $p*$degree + ($degree+1)/2.0]
		if {$nidx < $mp} {
			set orientation "left-down"
		} elseif {$nidx > $mp } {
			set orientation "right-down"
		} else {
			set orientation "down"
		}
		$ns duplex-link-op $n($p) $n($nidx) orient $orientation
#		$ns duplex-link-op $n($p) $n($nidx) queuePos [expr 1/pow($degree, $l)]

		#attach senders
		set cbr($nidx) [new Agent/CBR]
		$cbr($nidx) set dst_ 0x8003
		$cbr($nidx) set class_ [expr 100 + $nidx]
		$cbr($nidx) set interval_ 20ms
		$ns attach-agent $n($nidx) $cbr($nidx)

		#attach receivers
		set rcvr($nidx) [new Agent/Null]
		$ns attach-agent $n($nidx) $rcvr($nidx)

		incr nidx
	}
}

### Start multicast configuration: 
source ../../mcast/BST.tcl

BST set RP_([expr 0x8003]) $n(0)


set mproto BST
set mrthandle [$ns mrtproto $mproto {}]
### End of multicast configuration

$ns color 103 Navy      ;#cbrs
$ns color 105 BlueViolet

$ns color 30 purple   ;#grafts
$ns color 31 green    ;#prunes

$n(0) color blue        ;#RP

$n(0) color Navy
$n(3) color BlueViolet

$ns at 0    "$cbr(3) start"
$ns at 0.05 "$cbr(5) start"
$ns at 0.2  "$n(4) join-group   $rcvr(4) 0x8003"
$ns at 0.3  "$n(6) join-group   $rcvr(6) 0x8003"
$ns at 0.4  "$n(4) leave-group  $rcvr(4) 0x8003"
$ns at 0.5  "$n(6) leave-group  $rcvr(6) 0x8003"
$ns at 0.55 "finish"

proc finish {} {
        global ns
        $ns flush-trace

        puts "running nam..."
        exec nam out-mc8 &
        exit 0
}

$ns run



# Run this script as
# ../../ns test-drr.tcl 
# or
# ../../ns test-drr.tcl setmask 
# After running this script cat "out" to view the flow stats

#default Mask parameter
set MASK 0

if { [lindex $argv 0] == "setmask" } {
    set MASK 1
    puts stderr "Ignoring port nos of sources"
}

set ns [new Simulator]

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out.tr w]

#$ns trace-all $f

$ns duplex-link $n0 $n1 1.0Mb 0.01ms DRR 
$ns duplex-link $n2 $n0 10.0Mb 0.01ms DRR 
$ns duplex-link $n3 $n0 10.0Mb 0.01ms DRR 

# trace the bottleneck queue 
#
$ns trace-queue $n0 $n1 $f

Simulator instproc get-link { node1 node2 } {
    $self instvar link_
    set id1 [$node1 id]
    set id2 [$node2 id]
    return $link_($id1:$id2)
}

#Alternate way for setting parameters for the DRR queue
set l [$ns get-link $n0 $n1]
set q [$l queue]

$q mask $MASK
$q blimit 25000
$q quantum 500
$q buckets 7

# create 6 cbr sources :
#source 0 :
set cbr0 [new Agent/CBR]
$ns attach-agent $n2 $cbr0
$cbr0 set packetSize_ 250
$cbr0 set interval_ 20ms
$cbr0 set random_ 1

#source 1 :
set cbr1 [new Agent/CBR]
$ns attach-agent $n2 $cbr1
$cbr1 set packetSize_ 250
$cbr1 set interval_ 4ms
$cbr1 set random_ 1

#source 2:
set cbr2 [new Agent/CBR]
$ns attach-agent $n2 $cbr2
$cbr2 set packetSize_ 250
$cbr2 set interval_ 2.5ms
$cbr2 set random_ 1


#source 3 :
set cbr3 [new Agent/CBR]
$ns attach-agent $n3 $cbr3
$cbr3 set packetSize_ 250
$cbr3 set interval_ 2.5ms
$cbr3 set random_ 1

#source 4
set cbr4 [new Agent/CBR]
$ns attach-agent $n3 $cbr4
$cbr4 set packetSize_ 250
$cbr4 set interval_ 4ms
$cbr4 set random_ 1

#source 5
set cbr5 [new Agent/CBR]
$ns attach-agent $n3 $cbr5
$cbr5 set packetSize_ 250
$cbr5 set interval_ 20ms
$cbr5 set random_ 1

# receiver 0 :
set lm0 [new Agent/Null]
$ns attach-agent $n1 $lm0

$ns connect $cbr0 $lm0
$ns connect $cbr1 $lm0
$ns connect $cbr2 $lm0
$ns connect $cbr3 $lm0
$ns connect $cbr4 $lm0
$ns connect $cbr5 $lm0

$ns at 0.0 "$cbr0 start"
$ns at 0.0  "$cbr1 start"
$ns at 0.0  "$cbr2 start"
$ns at 0.0  "$cbr3 start"
$ns at 0.0  "$cbr4 start"
$ns at 0.0  "$cbr5 start"


$ns at 2.0 "$cbr0 stop"
$ns at 2.0 "$cbr1 stop"
$ns at 2.0  "$cbr2 stop"
$ns at 2.0  "$cbr3 stop"
$ns at 2.0  "$cbr4 stop"
$ns at 2.0  "$cbr5 stop"


$ns at 3.0 "close $f;finish"


proc finish {} {
    puts "processing output ..."
    exec awk -f drr-awkfile out.tr >out; exec cat out & 
    exec sleep 1
    exit 0
}

$ns run


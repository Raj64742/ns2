##
## simple example to demonstrate use of traffic generation modules
##
## topology consists of 2 nodes connected by a point-to-point link
## 4 agents source traffic from node n0 destined for null agents
## at node n1.  agent s1 is an exponential on/off source, s2 is
## a pareto on/off source, agents s3 and s4 generate traffic
## based on a trace file.
##

source tg-finish.tcl
set stoptime 100.0
set plottime 101.0

set ns [new Simulator]

set n0 [$ns node]
set n1 [$ns node]

$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail

set f [open out.tr w]
$ns trace-queue $n0 $n1 $f


## set up the exponential on/off source, parameterized by packet size,
## ave on time, ave off time and peak rate

set s1 [new Agent/CBR/UDP]
$ns attach-agent $n0 $s1

set null1 [new Agent/Null]
$ns attach-agent $n1 $null1

$ns connect $s1 $null1

set exp1 [new Traffic/Expoo]
$exp1 set packet-size 210
$exp1 set burst-time 500ms
$exp1 set idle-time 500ms
$exp1 set rate 64k

$s1 attach-traffic $exp1

## set up the pareto on/off source, parameterized by packet size,
## ave on time, ave  off time, peak rate and pareto shape parameter

set s2 [new Agent/CBR/UDP]
$ns attach-agent $n0 $s2

set null2 [new Agent/Null]
$ns attach-agent $n1 $null2

$ns connect $s2 $null2

set pareto2 [new Traffic/Pareto]
$pareto2 set packet-size 210
$pareto2 set burst-time 500ms
$pareto2 set idle-time 500ms
$pareto2 set rate 64k
$pareto2 set shape 1.5

$s2 attach-traffic $pareto2


## initialize a trace file
set tfile [new Tracefile]
$tfile filename example-trace

## set up a source that uses the trace file

set s3 [new Agent/CBR/UDP]
$ns attach-agent $n0 $s3

set null3 [new Agent/Null]
$ns attach-agent $n1 $null3

$ns connect $s3 $null3

set tfile [new Tracefile]
#$tfile filename /tmp/example-trace
$tfile filename example-trace

set trace3 [new Traffic/Trace]
$trace3 attach-tracefile $tfile

$s3 attach-traffic $trace3


## attach a 2nd source to the same trace file

set s4 [new Agent/CBR/UDP]
$ns attach-agent $n0 $s4

set null4 [new Agent/Null]
$ns attach-agent $n1 $null4

$ns connect $s4 $null4

set trace4 [new Traffic/Trace]
$trace4 attach-tracefile $tfile

$s4 attach-traffic $trace4

$ns at 1.0 "$s1 start"
$ns at 1.0 "$s2 start"
$ns at 1.0 "$s3 start"
$ns at 1.0 "$s4 start"

$ns at $stoptime "$s1 stop"
$ns at $stoptime "$s2 stop"
$ns at $stoptime "$s3 stop"
$ns at $stoptime "$s4 stop"

$ns at $plottime "close $f"
$ns at $plottime "finish tg"

$ns run

## run nam?
## exec awk -f ../nam-demo/nstonam.awk out.tr > tg-nam.tr
## exec rm -f out
## puts "running nam..."
## exec nam tg-nam &
## exit 0

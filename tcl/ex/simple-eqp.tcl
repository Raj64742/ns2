#
# Simple example of an equal cost multi-path routing through
# two equal cost routes.
#
#
#		$n0       $n3
#		   \     /   \
#		    \   /     \
#		     $n2       $n4
#		    /   \     /
#		   /     \   /
#		$n1       $n5
#
# The default in ns is to not use equal cost routes.  Therefore, we must first
# ensure that each Node can use multiple Paths when they are available.
#
# The routing protocol that must be run is specified as arguments to the 
# run command, rather than being specified separately.
#

set ns [new Simulator]

Node set multiPath_ 1

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
set n4 [$ns node]
set n5 [$ns node]

set f [open out.tr w]
$ns trace-all $f

$ns duplex-link $n0 $n2 10Mb 2ms DropTail
$ns duplex-link $n1 $n2 10Mb 2ms DropTail

$ns duplex-link $n2 $n3 1.5Mb 10ms DropTail
$ns duplex-link $n3 $n4 1.5Mb 10ms DropTail
$ns queue-limit $n2 $n3 5

$ns duplex-link $n2 $n5 1.5Mb 10ms DropTail
$ns duplex-link $n5 $n4 1.5Mb 10ms DropTail
$ns queue-limit $n2 $n5 5

proc build-tcp { n0 n1 startTime } {
    global ns
    set tcp [new Agent/TCP]
    $ns attach-agent $n0 $tcp

    set snk [new Agent/TCPSink]
    $ns attach-agent $n1 $snk

    $ns connect $tcp $snk

    set ftp [new Source/FTP]
    $ftp set agent_ $tcp
    $ns at $startTime "$ftp start"
    return $tcp
}

[build-tcp $n0 $n4 0.7] set class_ 0
[build-tcp $n1 $n4 0.9] set class_ 1

proc finish {} {
    global argv0
    global ns f
    close $f
    $ns flush-trace

    if [string match {*.tcl} $argv0] {
	set prog [string range $argv0 0 [expr [string length $argv0] - 5]]
    } else {
	set prog $argv0
    }

    puts "converting output to nam format..."
    exec awk -f ../nam-demo/nstonam.awk out.tr > $prog-nam.tr 
    exec rm -f out
    #XXX
    puts "running nam..."
    exec nam $prog-nam &
    exit 0
}

$ns at 8.0 "finish"
$ns run Dynamic DV

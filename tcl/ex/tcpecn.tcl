#
# A simple example for tcp ecn simulation/animation with namgraph support
# This script is adopted from ns-2/tcl/test/test-suite-ecn.tcl
#
 
set ns [new Simulator]
 
#
#
# Create a simple six node topology:
#
#        s1                 s3
#         \                 /
# 10Mb,2ms \  1.5Mb,20ms   / 10Mb,4ms
#           r1 --------- r2
# 10Mb,3ms /               \ 10Mb,5ms
#         /                 \
#        s2                 s4
#

proc build_topology { ns } {

    global node_

    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(r2) [$ns node]
    set node_(s3) [$ns node]
    set node_(s4) [$ns node]

    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 25
    $ns queue-limit $node_(r2) $node_(r1) 25
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
 
    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(s2) $node_(r1) orient right-up
    $ns duplex-link-op $node_(r1) $node_(r2) orient right
    $ns duplex-link-op $node_(r1) $node_(r2) queuePos 0
    $ns duplex-link-op $node_(r2) $node_(r1) queuePos 0
    $ns duplex-link-op $node_(s3) $node_(r2) orient left-down
    $ns duplex-link-op $node_(s4) $node_(r2) orient left-up
 
}

set f [open out.tr w]
$ns trace-all $f
set nf [open out.nam w]
$ns namtrace-all $nf

build_topology $ns

set redq [[$ns link $node_(r1) $node_(r2)] queue]
$redq set setbit_ true
 
set tcp1 [$ns create-connection TCP/Reno $node_(s1) TCPSink $node_(s3) 0]
$tcp1 set window_ 15
$tcp1 set ecn_ 1
 
set tcp2 [$ns create-connection TCP/Reno $node_(s2) TCPSink $node_(s3) 1]
$tcp2 set window_ 15
$tcp2 set ecn_ 1
 
set ftp1 [$tcp1 attach-source FTP]    
set ftp2 [$tcp2 attach-source FTP]
 
$ns at 0.0 "$ftp1 start"
$ns at 0.0 "$ftp2 start"

$ns at 2.0 "finish"

proc finish {} {
        global ns f nf
        $ns flush-trace
        close $f
        close $nf
 
        #XXX
        puts "Filtering ..."
	exec tclsh ../bin/namfilter.tcl out.nam

        puts "running nam..."
        exec nam out.nam &
        exit 0
}
 
$ns run



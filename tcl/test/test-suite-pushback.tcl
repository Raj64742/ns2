#
# Copyright (c) 2000  International Computer Science Institute
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#      This product includes software developed by ACIRI, the AT&T
#      Center for Internet Research at ICSI (the International Computer
#      Science Institute).
# 4. Neither the name of ACIRI nor of ICSI may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

set dir [pwd]
catch "cd tcl/test"
source misc_simple.tcl
catch "cd $dir"
Queue/RED set gentle_ true
Agent/Pushback set verbose_ false
Queue/RED/Pushback set rate_limiting_ 0
Agent/Pushback set enable_pushback_ 0

set flowfile fairflow.tr; # file where flow data is written
set flowgraphfile fairflow.xgr; # file given to graph tool 

TestSuite instproc finish file {
	global quiet PERL
	$self instvar ns_ tchan_ testName_
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
          $PERL ../../bin/raw2xg -a -s 0.01 -m 90 -t $file > temp.rands
	if {$quiet == "false"} {
        	exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
	}
        ## now use default graphing tool to make a data file
        ## if so desired

	if { [info exists tchan_] && $quiet == "false" } {
		$self plotQueue $testName_
	}
	$ns_ halt
}

TestSuite instproc enable_tracequeue ns {
	$self instvar tchan_ node_
	set redq [[$ns link $node_(r0) $node_(r1)] queue]
	set tchan_ [open all.q w]
	$redq trace curq_
	$redq trace ave_
	$redq attach $tchan_
}

Class Topology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}

Class Topology/net2 -superclass Topology
Topology/net2 instproc init ns {
    $self instvar node_ 
    #the destinations; declared first 
    for {set i 0} {$i < 2} {incr i} {
        set node_(d$i) [$ns node]
    }

    #the routers
    for {set i 0} {$i < 4} {incr i} {
        set node_(r$i) [$ns node]
        #$node_(r$i) add-pushback-agent
    }
    $node_(r0) add-pushback-agent

    #the sources
    for {set i 0} {$i < 4} {incr i} {
        set node_(s$i) [$ns node]
    }

    $self next 

    $ns duplex-link $node_(s0) $node_(r0) 10Mb 2ms DropTail
    $ns duplex-link $node_(s1) $node_(r0) 10Mb 3ms DropTail
    $ns pushback-simplex-link $node_(r0) $node_(r1) 0.5Mb 10ms 
    $ns simplex-link $node_(r1) $node_(r0) 0.5Mb 10ms DropTail
    $ns duplex-link $node_(d0) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(d1) $node_(r1) 10Mb 2ms DropTail

    $ns queue-limit $node_(r0) $node_(r1) 100
    $ns queue-limit $node_(r1) $node_(r0) 100
}   

TestSuite instproc plotQueue file {
	global quiet
	$self instvar tchan_
	#
	# Plot the queue size and average queue size, for RED gateways.
	#
	set awkCode {
		{
			if ($1 == "Q" && NF>2) {
				print $2, $3 >> "temp.q";
				set end $2
			}
			else if ($1 == "a" && NF>2)
				print $2, $3 >> "temp.a";
		}
	}
	set f [open temp.queue w]
	puts $f "TitleText: $file"
	puts $f "Device: Postscript"

	if { [info exists tchan_] } {
		close $tchan_
	}
	exec rm -f temp.q temp.a 
	exec touch temp.a temp.q

	exec awk $awkCode all.q

	puts $f \"queue
	exec cat temp.q >@ $f  
	puts $f \n\"ave_queue
	exec cat temp.a >@ $f
	###puts $f \n"thresh
	###puts $f 0 [[ns link $r1 $r2] get thresh]
	###puts $f $end [[ns link $r1 $r2] get thresh]
	close $f
	if {$quiet == "false"} {
		exec xgraph -bb -tk -x time -y queue temp.queue &
	}
}

TestSuite instproc tcpDumpAll { tcpSrc interval label } {
    global quiet
    $self instvar dump_inst_ ns_
    if ![info exists dump_inst_($tcpSrc)] {
	set dump_inst_($tcpSrc) 1
	set report $label/window=[$tcpSrc set window_]/packetSize=[$tcpSrc set packetSize_]
	if {$quiet == "false"} {
		puts $report
	}
	$ns_ at 0.0 "$self tcpDumpAll $tcpSrc $interval $label"
	return
    }
    $ns_ at [expr [$ns_ now] + $interval] "$self tcpDumpAll $tcpSrc $interval $label"
    set report time=[$ns_ now]/class=$label/ack=[$tcpSrc set ack_]/packets_resent=[$tcpSrc set nrexmitpack_]
    if {$quiet == "false"} {
    	puts $report
    }
}       

TestSuite instproc setTopo {} {
    $self instvar node_ net_ ns_ topo_

    set topo_ [new Topology/$net_ $ns_]
    set node_(s0) [$topo_ node? s0]
    set node_(s1) [$topo_ node? s1]
    set node_(r0) [$topo_ node? r0]
    set node_(r1) [$topo_ node? r1]
    set node_(d0) [$topo_ node? d0]
    set node_(d1) [$topo_ node? d1]
    [$ns_ link $node_(r0) $node_(r1)] trace-dynamics $ns_ stdout
}

TestSuite instproc printall { fmon time packetsize} {
	set packets [$fmon set pdepartures_]
	set linkBps [ expr 500000/8 ]
	set utilization [expr ($packets*$packetsize)/($time*$linkBps)]
	puts " "
	puts "link utilization [format %.3f $utilization]"
	set fcl [$fmon classifier];
	for {set i 1} {$i < 4} {incr i} {
	    set flow [$fcl lookup auto 0 0 $i]
	    set flowpkts [$flow set pdepartures_]
	    set flowutil [expr ($flowpkts*$packetsize)/($time*$linkBps)]
	    puts "fid: $i utilization: [format %.3f $flowutil]"
	}
}

#
# one complete test with CBR flows only, no pushback and no red-pd.
#
Class Test/cbrs -superclass TestSuite
Test/cbrs instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ cbrs
    $self next
}
Test/cbrs instproc run {} {
    $self instvar ns_ node_ testName_ net_ topo_
    $self setTopo
    set packetsize_ 200
    Application/Traffic/CBR set random_ 0
    Application/Traffic/CBR set packetSize_ $packetsize_

    set slink [$ns_ link $node_(r0) $node_(r1)]; # link to collect stats on
    set fmon [$ns_ makeflowmon Fid]
    $ns_ attach-fmon $slink $fmon

    set stoptime 30.0
    #set stoptime 5.0
    # good traffic
    set udp1 [$ns_ create-connection UDP $node_(s0) Null $node_(d0) 1]
    set cbr1 [$udp1 attach-app Traffic/CBR]
    $cbr1 set rate_ 0.1Mb
    $cbr1 set random_ 0.005

    # poor traffic
    set udp2 [$ns_ create-connection UDP $node_(s1) Null $node_(d1) 2]
    set cbr2 [$udp2 attach-app Traffic/CBR]
    $cbr2 set rate_ 0.1Mb
    $cbr2 set random_ 0.005

    $self enable_tracequeue $ns_
    $ns_ at 0.2 "$cbr1 start"
    $ns_ at 0.1 "$cbr2 start"

    # bad traffic
    set udp [$ns_ create-connection UDP $node_(s0) Null $node_(d1) 3]
    set cbr [$udp attach-app Traffic/CBR]
    $cbr set rate_ 0.5Mb
    $cbr set random_ 0.001
    $ns_ at 0.0 "$cbr start"

    $self timeDump 5.0
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self printall $fmon $stoptime $packetsize_"
    $ns_ at $stoptime "$self cleanupAll $testName_"
    $ns_ run
}

#
# one complete test with CBR flows only, with ACC.
#
Class Test/cbrs-acc -superclass TestSuite
Test/cbrs-acc instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ cbrs-acc
    Queue/RED/Pushback set rate_limiting_ 1
    Test/cbrs-acc instproc run {} [Test/cbrs info instbody run]
    $self next
}

TestSuite runTest

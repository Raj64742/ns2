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
	$self instvar ns_ tchan_ testName_ tmpschan_
	# was: -s 2 -d 3 
        #exec $PERL ../../bin/getrc -s 12 -d 13 all.tr | \
        #  $PERL ../../bin/raw2xg -a -s 0.01 -m 90 -t $file > temp2.rands
	#if {$quiet == "false"} {
        #	exec xgraph -bb -tk -nl -m -x time -y packets temp2.rands &
	#}
        ## now use default graphing tool to make a data file
        ## if so desired

	if { [info exists tchan_] && $quiet == "false" } {
		#$self plotQueue $testName_
	}
	if { [info exists tmpschan_]} {
		$self finishflows $testName_
		close $tmpschan_
	}
	$ns_ halt
}

# display graph of results
#
# temp.s:
# time: 4.000 LinkUtilThisTime  1.002 totalLinkUtil: 1.000 totalOQPkts: 1250
# fid: 1 Util: 0.124 OQdroprate: 0.320 OQpkts: 155 OQdrops: 73
#
TestSuite instproc finishflows testname {
        global quiet
        $self instvar tmpschan_ tmpqchan_ topo_ node_ ns_
	$self instvar packetsize_
        $topo_ instvar cbqlink_ bandwidth1_

        set graphfile temp.rands
        set maxbytes [expr $bandwidth1_ / 8.0]
	set maxpkts [expr 1.0 * $maxbytes / $packetsize_]

        set awkCode  {
		{
		if ($1 == 0) {time=1; oldpkts=0;}
		if ($1 == "time:" && $3 == "LinkUtilThisTime") {
			time = $2
		}
		if ($1 == "fid:" && $2==flow) {
		  newpkts = $8;
		  pkts = newpkts - oldpkts;
		  print time, pkts/maxpkts;
		  oldpkts = newpkts
		}}
        }
        set awkCodeAll {
		{
		if ($1==0) {oldpkts=0}
		if ($1 == "time:" && $7 == "totalOQPkts:") {
			time = $2;
		  	newpkts = $8;
		  	pkts = newpkts - oldpkts;
		  	print time, pkts/maxpkts;
		  	oldpkts = newpkts
		}}
        }

        if { [info exists tmpschan_] } {
                close $tmpschan_
        }

        set f [open $graphfile w]
        puts $f "TitleText: $testname"
        puts $f "Device: Postscript"

        exec rm -f temp.p
        exec touch temp.p
        foreach i { 1 2 3 4 5 } {
                exec echo "\n\"flow $i" >> temp.p
                exec awk $awkCode flow=$i maxpkts=$maxpkts temp.s > temp.$i
                exec cat temp.$i >> temp.p
                exec echo " " >> temp.p
        }

        exec awk $awkCodeAll maxpkts=$maxpkts temp.s > temp.all
        exec echo "\n\"all " >> temp.p
        exec cat temp.all >> temp.p

        exec cat temp.p >@ $f
        close $f
	if {$quiet == "false"} {
               	exec xgraph -bb -tk -ly 0,1 -x time -y bandwidth $graphfile &
	}
#       exec csh figure2.com $file

        exit 0
}

Class Topology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}

Class Topology/net2 -superclass Topology
Topology/net2 instproc init ns {
    $self instvar node_ bandwidth_ bandwidth1_
    set bandwidth_ 0.5Mb
    set bandwidth1_ 500000
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
    $ns pushback-simplex-link $node_(r0) $node_(r1) $bandwidth_ 10ms 
    $ns simplex-link $node_(r1) $node_(r0) $bandwidth_ 10ms DropTail
    $ns duplex-link $node_(d0) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(d1) $node_(r1) 10Mb 2ms DropTail

    $ns queue-limit $node_(r0) $node_(r1) 100
    $ns queue-limit $node_(r1) $node_(r0) 100
}   

Class Topology/net3 -superclass Topology
Topology/net3 instproc init ns {
    $self instvar node_ bandwidth_ bandwidth1_
    set bandwidth_ 0.5Mb
    set bandwidth1_ 500000
    #the destinations; declared first 
    for {set i 0} {$i < 2} {incr i} {
        set node_(d$i) [$ns node]
    }

    #the routers
    for {set i 0} {$i < 4} {incr i} {
        set node_(r$i) [$ns node]
        $node_(r$i) add-pushback-agent
    }

    #the sources
    for {set i 0} {$i < 4} {incr i} {
        set node_(s$i) [$ns node]
    }

    $self next 

    $ns duplex-link $node_(s0) $node_(r2) 10Mb 2ms DropTail
    $ns duplex-link $node_(s1) $node_(r3) 10Mb 3ms DropTail
    $ns pushback-duplex-link $node_(r0) $node_(r1) $bandwidth_ 10ms 
    $ns pushback-duplex-link $node_(r2) $node_(r0) $bandwidth_ 10ms 
    $ns pushback-duplex-link $node_(r3) $node_(r0) $bandwidth_ 10ms 
    $ns duplex-link $node_(d0) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(d1) $node_(r1) 10Mb 2ms DropTail

    $ns queue-limit $node_(r0) $node_(r1) 100
    $ns queue-limit $node_(r1) $node_(r0) 100
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

#    
# Arrange for time to be printed every
# $interval seconds of simulation time
#    
TestSuite instproc statsDump { interval fmon packetsize oldpkts } {
        global quiet 
        $self instvar dump_inst_ ns_ tmpschan_ f
	set dumpfile temp.s
        if ![info exists dump_inst_] {
		$self instvar tmpschan_ f
                set dump_inst_ 1
		set f [open $dumpfile w]
		set tmpschan_ $f
                $ns_ at 0.0 "$self statsDump $interval $fmon $packetsize $oldpkts"
                return
        }
        set time [$ns_ now]
        puts $f "$time"
        set newtime [expr [$ns_ now] + $interval]
	## $quiet == "false"
        if { $time > 0} {
            set totalPkts [$fmon set pdepartures_]
            set packets [expr $totalPkts - $oldpkts]
            set oldpkts $totalPkts
    	    set linkBps [ expr 500000/8 ]
    	    set recentUtil [expr (1.0*$packets*$packetsize)/($interval*$linkBps)]
    	    set totalLinkUtil [expr (1.0*$totalPkts*$packetsize)/($time*$linkBps)]
            set now [$ns_ now]
    	    puts $f "time: [format %.3f $now] LinkUtilThisTime  [format %.3f $recentUtil] totalLinkUtil: [format %.3f $totalLinkUtil] totalOQPkts: $totalPkts" 
    	    set fcl [$fmon classifier];
	    ## this 
    	    for {set i 1} {$i < 6} {incr i} {
    	        set flow [$fcl lookup auto 0 0 $i]
		if {$flow != "" } {
		  set flowpkts($flow) [$flow set pdepartures_]
    	          set flowutil [expr (1.0*$flowpkts($flow)*$packetsize)/($time*$linkBps)]
		  set flowdrops($flow) [$flow set pdrops_]
    	          set flowdroprate [expr (1.0*$flowdrops($flow)/($flowpkts($flow) + $flowdrops($flow)))] 
		  puts $f "fid: $i Util: [format %.3f $flowutil] OQdroprate: [format %.3f $flowdroprate] OQpkts: [format %d $flowpkts($flow)] OQdrops: [format %d $flowdrops($flow)]"
		}
	    }
        }
        $ns_ at $newtime "$self statsDump $interval $fmon $packetsize $oldpkts"
}

TestSuite instproc setup {} {
    $self instvar ns_ node_ testName_ net_ topo_ cbr_ cbr2_ packetsize_
    $self setTopo

    set stoptime 100.0
    #set stoptime 5.0
    #set dumptime 5.0
    set dumptime 1.0
    set stoptime1 [expr $stoptime + 1.0]
    set packetsize_ 200
    Application/Traffic/CBR set random_ 0
    Application/Traffic/CBR set packetSize_ $packetsize_

    set slink [$ns_ link $node_(r0) $node_(r1)]; # link to collect stats on
    set fmon [$ns_ makeflowmon Fid]
    $ns_ attach-fmon $slink $fmon

    set udp1 [$ns_ create-connection UDP $node_(s0) Null $node_(d0) 1]
    set cbr1 [$udp1 attach-app Traffic/CBR]
    $cbr1 set rate_ 0.1Mb
    $cbr1 set random_ 0.005

    set udp2 [$ns_ create-connection UDP $node_(s1) Null $node_(d1) 2]
    set cbr2_ [$udp2 attach-app Traffic/CBR]
    $cbr2_ set rate_ 0.1Mb
    $cbr2_ set random_ 0.005

    # bad traffic
    set udp [$ns_ create-connection UDP $node_(s0) Null $node_(d1) 3]
    set cbr_ [$udp attach-app Traffic/CBR]
    $cbr_ set rate_ 0.5Mb
    $cbr_ set random_ 0.001
    $ns_ at 0.0 "$cbr_ start"

    set udp4 [$ns_ create-connection UDP $node_(s1) Null $node_(d0) 4]
    set cbr4 [$udp4 attach-app Traffic/CBR]
    $cbr4 set rate_ 0.1Mb
    $cbr4 set random_ 0.005

    set udp5 [$ns_ create-connection UDP $node_(s0) Null $node_(d0) 5]
    set cbr5 [$udp5 attach-app Traffic/CBR]
    $cbr5 set rate_ 0.1Mb
    $cbr5 set random_ 0.005

    $ns_ at 0.2 "$cbr1 start"
    $ns_ at 0.1 "$cbr2_ start"
    $ns_ at 0.3 "$cbr4 start"
    $ns_ at 0.4 "$cbr5 start"

    $self statsDump $dumptime $fmon $packetsize_ 0
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime1 "$self cleanupAll $testName_"
}

#
# one complete test with CBR flows only, no pushback and no red-pd.
#
Class Test/cbrs -superclass TestSuite
Test/cbrs instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ cbrs
    $self next 0
}
Test/cbrs instproc run {} {
    $self instvar ns_ node_ testName_ net_ topo_
    $self setTopo
    $self setup
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
    $self next 0
}

#
# one complete test with CBR flows only, with ACC
# CBR flows, ACC, flows starting and stopping 
#
Class Test/cbrs1 -superclass TestSuite
Test/cbrs1 instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ cbrs1
    $self next 0
}
Test/cbrs1 instproc run {} {
    $self instvar ns_ node_ testName_ net_ topo_ cbr_ cbr2_
    $self setTopo
    $self setup
    $ns_ at 10.0 "$cbr_ set rate_ 0.1Mb"
    $ns_ at 15.0 "$cbr2_ set rate_ 0.5Mb"
    $ns_ run
}

#
# one complete test with CBR flows only, with ACC
# CBR flows, ACC, flows starting and stopping 
#
Class Test/cbrs-acc1 -superclass TestSuite
Test/cbrs-acc1 instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ cbrs-acc1
    Queue/RED/Pushback set rate_limiting_ 1
    Test/cbrs-acc1 instproc run {} [Test/cbrs1 info instbody run]
    $self next 0
}

TestSuite instproc setup1 {} {
    $self instvar ns_ node_ testName_ net_ topo_ cbr_ cbr2_ packetsize_
    $self setTopo

    set stoptime 100.0
    #set dumptime 5.0
    set dumptime 1.0
    #set stoptime 5.0
    set stoptime1 [expr $stoptime + 1.0]
    set packetsize_ 200
    Application/Traffic/CBR set random_ 0
    Application/Traffic/CBR set packetSize_ $packetsize_

    set slink [$ns_ link $node_(r0) $node_(r1)]; # link to collect stats on
    set fmon [$ns_ makeflowmon Fid]
    $ns_ attach-fmon $slink $fmon

    set udp1 [$ns_ create-connection UDP $node_(s0) Null $node_(d0) 1]
    set cbr1_ [$udp1 attach-app Traffic/CBR]
    $cbr1_ set rate_ 0.1Mb
    $cbr1_ set random_ 0.005

    set udp2 [$ns_ create-connection UDP $node_(s1) Null $node_(d1) 2]
    set cbr2_ [$udp2 attach-app Traffic/CBR]
    $cbr2_ set rate_ 0.1Mb
    $cbr2_ set random_ 0.005

    set udp3 [$ns_ create-connection UDP $node_(s1) Null $node_(d1) 3]
    set cbr3_ [$udp3 attach-app Traffic/CBR]
    $cbr3_ set rate_ 0.1Mb
    $cbr3_ set random_ 0.005

    set udp4 [$ns_ create-connection UDP $node_(s1) Null $node_(d1) 4]
    set cbr4_ [$udp4 attach-app Traffic/CBR]
    $cbr4_ set rate_ 0.1Mb
    $cbr4_ set random_ 0.005

    $ns_ at 0.2 "$cbr1_ start"
    $ns_ at 0.1 "$cbr2_ start"
    $ns_ at 0.3 "$cbr3_ start"
    $ns_ at 0.4 "$cbr4_ start"

    # bad traffic
    set udp [$ns_ create-connection UDP $node_(s0) Null $node_(d1) 5]
    set cbr_ [$udp attach-app Traffic/CBR]
    $cbr_ set rate_ 0.1Mb
    $cbr_ set random_ 0.001
    $ns_ at 0.0 "$cbr_ start"
    $ns_ at 1.0 "$cbr_ set rate_ 0.15Mb"
    $ns_ at 2.0 "$cbr_ set rate_ 0.2Mb"
    $ns_ at 3.0 "$cbr_ set rate_ 0.25Mb"
    $ns_ at 4.0 "$cbr_ set rate_ 0.3Mb"
    $ns_ at 5.0 "$cbr_ set rate_ 0.35Mb"
    $ns_ at 6.0 "$cbr_ set rate_ 0.4Mb"
    $ns_ at 7.0 "$cbr_ set rate_ 0.45Mb"
    $ns_ at 8.0 "$cbr_ set rate_ 0.5Mb"
    $ns_ at 9.0 "$cbr_ set rate_ 0.55Mb"
    $ns_ at 10.0 "$cbr_ set rate_ 0.6Mb"
    $ns_ at 11.0 "$cbr_ set rate_ 0.65Mb"
    $ns_ at 12.0 "$cbr_ set rate_ 0.7Mb"
    $ns_ at 13.0 "$cbr_ set rate_ 0.75Mb"
    $ns_ at 14.0 "$cbr_ set rate_ 0.8Mb"
    $ns_ at 15.0 "$cbr_ set rate_ 0.855Mb"
    $ns_ at 17.0 "$cbr_ set rate_ 0.8Mb"
    $ns_ at 17.0 "$cbr_ set rate_ 0.75Mb"
    $ns_ at 18.0 "$cbr_ set rate_ 0.7Mb"
    $ns_ at 19.0 "$cbr_ set rate_ 0.65Mb"
    $ns_ at 20.0 "$cbr_ set rate_ 0.6Mb"
    $ns_ at 21.0 "$cbr_ set rate_ 0.55Mb"
    $ns_ at 22.0 "$cbr_ set rate_ 0.5Mb"
    $ns_ at 23.0 "$cbr_ set rate_ 0.45Mb"
    $ns_ at 24.0 "$cbr_ set rate_ 0.4Mb"
    $ns_ at 25.0 "$cbr_ set rate_ 0.35Mb"
    $ns_ at 26.0 "$cbr_ set rate_ 0.3Mb"
    $ns_ at 27.0 "$cbr_ set rate_ 0.25Mb"
    $ns_ at 28.0 "$cbr_ set rate_ 0.2Mb"
    $ns_ at 29.0 "$cbr_ set rate_ 0.15Mb"
    $ns_ at 30.0 "$cbr_ set rate_ 0.1Mb"

    $self statsDump $dumptime $fmon $packetsize_ 0
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime1 "$self cleanupAll $testName_"
}

#
# one complete test with CBR flows only, no pushback and no red-pd.
# Slowly-growing bad flow.
#
Class Test/slowgrow -superclass TestSuite
Test/slowgrow instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ slowgrow
    $self next 0
}
Test/slowgrow instproc run {} {
    $self instvar ns_ node_ testName_ net_ topo_
    $self setTopo
    $self setup1
    $ns_ run
}

#
# one complete test with CBR flows only, no pushback and no red-pd.
# Slowly-growing bad flow, but with local ACC.
#
Class Test/slowgrow-acc -superclass TestSuite
Test/slowgrow-acc instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ slowgrow-acc
    Queue/RED/Pushback set rate_limiting_ 1
    Test/slowgrow-acc instproc run {} [Test/slowgrow info instbody run]
    $self next 0
}

######################################################33

TestSuite instproc setup2 {} {
    $self instvar ns_ node_ testName_ net_ topo_ cbr_ cbr2_ packetsize_
    $self setTopo

    set stoptime 100.0
    set dumptime 1.0
    set stoptime1 [expr $stoptime + 1.0]
    set packetsize_ 200
    Application/Traffic/CBR set random_ 0
    Application/Traffic/CBR set packetSize_ $packetsize_

    set slink [$ns_ link $node_(r0) $node_(r1)]; # link to collect stats on
    set fmon [$ns_ makeflowmon Fid]
    $ns_ attach-fmon $slink $fmon

    set udp1 [$ns_ create-connection UDP $node_(s0) Null $node_(d0) 1]
    set cbr1 [$udp1 attach-app Traffic/CBR]
    $cbr1 set rate_ 0.1Mb
    $cbr1 set random_ 0.005

    set udp2 [$ns_ create-connection UDP $node_(s1) Null $node_(d1) 2]
    set cbr2_ [$udp2 attach-app Traffic/CBR]
    $cbr2_ set rate_ 0.1Mb
    $cbr2_ set random_ 0.005

    # bad traffic
    set udp [$ns_ create-connection UDP $node_(s0) Null $node_(d1) 3]
    set cbr_ [$udp attach-app Traffic/CBR]
    $cbr_ set rate_ 0.5Mb
    $cbr_ set random_ 0.001
    $ns_ at 0.0 "$cbr_ start"

    set udp4 [$ns_ create-connection UDP $node_(s1) Null $node_(d0) 4]
    set cbr4 [$udp4 attach-app Traffic/CBR]
    $cbr4 set rate_ 0.1Mb
    $cbr4 set random_ 0.005

    set udp5 [$ns_ create-connection UDP $node_(s0) Null $node_(d0) 5]
    set cbr5 [$udp5 attach-app Traffic/CBR]
    $cbr5 set rate_ 0.1Mb
    $cbr5 set random_ 0.005

    $ns_ at 0.2 "$cbr1 start"
    $ns_ at 0.1 "$cbr2_ start"
    $ns_ at 0.3 "$cbr4 start"
    $ns_ at 0.4 "$cbr5 start"

    $self statsDump $dumptime $fmon $packetsize_ 0
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime1 "$self cleanupAll $testName_"
}


#
# one complete test with CBR flows only, no pushback and no local ACC
#
Class Test/A_noACC -superclass TestSuite
Test/A_noACC instproc init {} {
    $self instvar net_ test_
    set net_ net3 
    set test_ A_noACC
    $self next 0
}
Test/A_noACC instproc run {} {
    $self instvar ns_ node_ testName_ net_ topo_
    $self setTopo
    $self setup2
    $ns_ run
}

# With ACC only.
Class Test/A_ACC -superclass TestSuite
Test/A_ACC instproc init {} {
    $self instvar net_ test_
    set net_ net3 
    set test_ A_ACC
    Queue/RED/Pushback set rate_limiting_ 1
    Agent/Pushback set enable_pushback_ 0
    Test/A_ACC instproc run {} [Test/A_noACC info instbody run]
    $self next 0
}

# With Pushback.
Class Test/A_Push -superclass TestSuite
Test/A_Push instproc init {} {
    $self instvar net_ test_
    set net_ net3 
    set test_ A_Push
    Queue/RED/Pushback set rate_limiting_ 1
    Agent/Pushback set enable_pushback_ 1
    Test/A_Push instproc run {} [Test/A_noACC info instbody run]
    $self next 0
}

TestSuite runTest

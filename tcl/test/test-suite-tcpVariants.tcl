#
# Copyright (c) 1995 The Regents of the University of California.
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
#	This product includes software developed by the Computer Systems
#	Engineering Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-tcpVariants.tcl,v 1.1 1998/05/13 22:57:05 sfloyd Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-tcp.tcl
#

source misc.tcl
source topologies.tcl

TestSuite instproc finish file {
	global quiet
        exec ../../bin/set_flow_id -s all.tr | \
          ../../bin/getrc -s 2 -d 3 | \
          ../../bin/raw2xg -s 0.01 -m 90 -t $file > temp.rands
	if {$quiet == "false"} {
		exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
	}
        ## now use default graphing tool to make a data file
	## if so desired
        exit 0
}

TestSuite instproc printtimers { tcp time} {
	global quiet
	if {$quiet == "false"} {
        	puts "time: $time sRTT(in ticks): [$tcp set srtt_]/8 RTTvar(in ticks): [$tcp set rttvar_]/4 backoff: [$tcp set backoff_]"
	}
}

TestSuite instproc printtimersAll { tcp time interval } {
        $self instvar dump_inst_ ns_
        if ![info exists dump_inst_($tcp)] {
                set dump_inst_($tcp) 1
                $ns_ at $time "$self printtimersAll $tcp $time $interval"
                return
        }
	set newTime [expr [$ns_ now] + $interval]
	$ns_ at $time "$self printtimers $tcp $time"
        $ns_ at $newTime "$self printtimersAll $tcp $newTime $interval"
}

#
# Links1 uses 8Mb, 5ms feeders, and a 800Kb 10ms bottleneck.
# Queue-limit on bottleneck is 2 packets.
#
Class Topology/net4 -superclass NodeTopology/4nodes
Topology/net4 instproc init ns {
    $self next $ns
    $self instvar node_
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 0ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 0ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 100ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 8
    $ns queue-limit $node_(k1) $node_(r1) 8
    if {[$class info instprocs config] != ""} {
	$self config $ns
    }

    $self instvar lossylink_
    set lossylink_ [$ns link $node_(r1) $node_(k1)]
    set em [new ErrorModule Fid] 
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $lossylink_ errormodule $em
}


TestSuite instproc emod {} {
        $self instvar topo_
        $topo_ instvar lossylink_
        set errmodule [$lossylink_ errormodule]
        return $errmodule
} 

TestSuite instproc drop_pkts pkts {
    $self instvar ns_
    set emod [$self emod]
    set errmodel1 [new ErrorModel/List]
    $errmodel1 droplist $pkts
    $emod insert $errmodel1
    $emod bind $errmodel1 1
}

TestSuite instproc setup {tcptype list} {
        $self instvar ns_ node_ testName_

        Agent/TCP set bugFix_ false
	set fid 1
        # Set up TCP connection
    	if {$tcptype == "Tahoe"} {
      		set tcp1 [$ns_ create-connection TCP $node_(s1) \
          	TCPSink $node_(k1) $fid]
    	} elseif {$tcptype == "Sack1"} {
      		set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) \
          	TCPSink/Sack1  $node_(k1) $fid]
    	} else {
      		set tcp1 [$ns_ create-connection TCP/$tcptype $node_(s1) \
          	TCPSink $node_(k1) $fid]
    	}
        $tcp1 set window_ 28
        set ftp1 [$tcp1 attach-source FTP]
        $ns_ at 1.0 "$ftp1 start"

        $self tcpDump $tcp1 5.0
        $self drop_pkts $list

        $self traceQueues $node_(r1) [$self openTrace 6.0 $testName_]
        $ns_ run
}

# Definition of test-suite tests

###################################################
## One drop
###################################################

Class Test/onedrop_tahoe -superclass TestSuite
Test/onedrop_tahoe instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	onedrop_tahoe
	$self next
}
Test/onedrop_tahoe instproc run {} {
        $self setup Tahoe {14}
}

Class Test/onedrop_reno -superclass TestSuite
Test/onedrop_reno instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	onedrop_reno
	$self next
}
Test/onedrop_reno instproc run {} {
        $self setup Reno {14}
}

Class Test/onedrop_newreno -superclass TestSuite
Test/onedrop_newreno instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	onedrop_newreno
	$self next
}
Test/onedrop_newreno instproc run {} {
        $self setup Newreno {14}
}

Class Test/onedrop_sack -superclass TestSuite
Test/onedrop_sack instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	onedrop_sack
	$self next
}
Test/onedrop_sack instproc run {} {
        $self setup Sack1 {14}
}

###################################################
## Two drops
###################################################

Class Test/twodrops_tahoe -superclass TestSuite
Test/twodrops_tahoe instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	twodrops_tahoe
	$self next
}
Test/twodrops_tahoe instproc run {} {
        $self setup Tahoe {14 28}
}

Class Test/twodrops_reno -superclass TestSuite
Test/twodrops_reno instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	twodrops_reno
	$self next
}
Test/twodrops_reno instproc run {} {
        $self setup Reno {14 28}
}

Class Test/twodrops_newreno -superclass TestSuite
Test/twodrops_newreno instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	twodrops_newreno
	$self next
}
Test/twodrops_newreno instproc run {} {
        $self setup Newreno {14 28}
}

Class Test/twodrops_sack -superclass TestSuite
Test/twodrops_sack instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	twodrops_sack
	$self next
}
Test/twodrops_sack instproc run {} {
        $self setup Sack1 {14 28}
}

###################################################
## Three drops
###################################################

Class Test/threedrops_tahoe -superclass TestSuite
Test/threedrops_tahoe instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	threedrops_tahoe
	$self next
}
Test/threedrops_tahoe instproc run {} {
        $self setup Tahoe {14 26 28}
}

Class Test/threedrops_reno -superclass TestSuite
Test/threedrops_reno instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	threedrops_reno
	$self next
}
Test/threedrops_reno instproc run {} {
        $self setup Reno {14 26 28}
}

Class Test/threedrops_newreno -superclass TestSuite
Test/threedrops_newreno instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	threedrops_newreno
	$self next
}
Test/threedrops_newreno instproc run {} {
        $self setup Newreno {14 26 28}
}

Class Test/threedrops_sack -superclass TestSuite
Test/threedrops_sack instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	threedrops_sack
	$self next
}
Test/threedrops_sack instproc run {} {
        $self setup Sack1 {14 26 28}
}

###################################################
## Four drops
###################################################

Class Test/fourdrops_tahoe -superclass TestSuite
Test/fourdrops_tahoe instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	fourdrops_tahoe
	$self next
}
Test/fourdrops_tahoe instproc run {} {
        $self setup Tahoe {14 24 26 28}
}

Class Test/fourdrops_reno -superclass TestSuite
Test/fourdrops_reno instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	fourdrops_reno
	$self next
}
Test/fourdrops_reno instproc run {} {
        $self setup Reno {14 24 26 28}
}

Class Test/fourdrops_newreno -superclass TestSuite
Test/fourdrops_newreno instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	fourdrops_newreno
	$self next
}
Test/fourdrops_newreno instproc run {} {
        $self setup Newreno {14 24 26 28}
}

Class Test/fourdrops_sack -superclass TestSuite
Test/fourdrops_sack instproc init topo {
	$self instvar net_ defNet_ test_
	set net_	$topo
	set defNet_	net4
	set test_	fourdrops_sack
	$self next
}
Test/fourdrops_sack instproc run {} {
        $self setup Sack1 {14 24 26 28}
}


TestSuite runTest

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:

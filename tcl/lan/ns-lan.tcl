#
# Copyright (c) 1997 Regents of the University of California.
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
# 	This product includes software developed by the Daedalus Research
#       Group at the University of California, Berkeley.
# 4. Neither the name of the University nor of the research group
#    may be used to endorse or promote products derived from this software 
#    without specific prior written permission.
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
# Contributed by the Daedalus Research Group, http://daedalus.cs.berkeley.edu
#

Class NsObject

NsObject instproc init {args} {
	# initialize default vars first
	$self init-vars [$self info class]
	eval $self next $args
}

NsObject instproc init-vars {parents} {
	foreach cl $parents {
		if {$cl == "Object"} continue
		# depth first: set vars of ancestors first
		$self init-vars "[$cl info superclass]"
		foreach var [$cl info vars] {
			if [catch "$self set $var"] {
				$self set $var [$cl set $var]
			}
		}
	}
}

# Defaults for link-layer
LL set bandwidth_ 2Mb
LL set delay_ 0.5ms
LL set macDA_ 0

if [TclObject is-class LL/Snoop] {
LL/Snoop set snoopTick_ 0.05
LL/Snoop set bandwidth_ 1Mb
LL/Snoop set delay_ 0ms
LL/Snoop set snoopDisable_ 0
LL/Snoop set srtt_ 100ms
LL/Snoop set rttvar_ 0
LL/Snoop set g_ 0.25
}

# TraceIp trace IP packet headers for LAN components
TraceIp set src_ -1
TraceIp set dst_ -1
TraceIp set callback_ 0
TraceIp set show_tcphdr_ 0
TraceIp set mask_ 0xffffffff
TraceIp set shift_ 8

TraceIp instproc init {type args} {
	$self next $type
	$self instvar type_ src_
	set type_ $type
	if {$args != ""} {
		set src_ $args
	}
}

Class TraceIp/Enque -superclass TraceIp
TraceIp/Enque instproc init {args} {
	eval $self next "+" $args
}

Class TraceIp/Deque -superclass TraceIp
TraceIp/Deque instproc init {args} {
	eval $self next "-" $args
}

Class TraceIp/Drop -superclass TraceIp
TraceIp/Drop instproc init {args} {
	eval $self next "d" $args
}

Class TraceIp/BackoffDrop -superclass TraceIp
TraceIp/BackoffDrop instproc init {args} {
	eval $self next "b" $args
}

Class TraceIp/Corrupt -superclass TraceIp
TraceIp/Corrupt instproc init {args} {
	eval $self next "c" $args
}

Class TraceIp/Mac/Hop -superclass TraceIp/Mac
TraceIp/Mac/Hop instproc init {args} {
	eval $self next "h" $args
}

# Trace/Recv trace the receive events of a packet
Class Trace/Recv -superclass Trace
Trace/Recv instproc init {} {
	$self next "r"
}

# Trace/Loss trace the packets that are loss due to error model
Class Trace/Loss -superclass Trace
Trace/Loss instproc init {} {
	$self next "l"
}


#
# newLan:  create a LAN from a sete of nodes
#
Simulator instproc newLan {nodelist bw delay args} {
	set lan [eval new LanLink $self $args]
	foreach node $nodelist {
		$lan addNode $node $bw $delay
	}
	return $lan
}

# XXX Depricated:  use newLan instead of make-lan
Simulator instproc make-lan {nodelist bw delay {llType LL} {ifqType Queue/DropTail} {macType Mac} {chanType Channel}} {
	return [$self newLan $nodelist $bw $delay -llType $llType \
		-ifqType $ifqType -macType $macType -chanType $chanType]
}


#
# Link/LanDuplex:  a peer-to-peer duplex link on a LAN
# 
Class Link/LanDuplex -superclass SimpleLink
Link/LanDuplex instproc init {src dst bw delay q args} {
	eval $self next $src $dst $bw $delay $q $args
}

Link/LanDuplex instproc trace {ns f} {
	$self instvar link_ fromNode_ toNode_
	$self instvar enqT_ deqT_ drpT_ rcvT_ dynT_

	set enqT_ [$ns create-trace Enque $f $fromNode_ $toNode_]
	$enqT_ target [$link_ sendtarget]
	$link_ sendtarget $enqT_
	set recvT_ [$ns create-trace Recv $f $toNode_ $fromNode_]
	$recvT_ target [$link_ recvtarget]
	$link_ recvtarget $recvT_

	set drpT_ [$ns create-trace Loss $f $fromNode_ $toNode_]
	set namtrfd [$ns get-nam-traceall]
	if {$namtrfd != ""} {
		$drpT_ attach-nam $namtraceAllFile_
	}
	$link_ drop-target $drpT_
}


Class NetIface -superclass Connector

NetIface set ifqType_ Queue/DropTail
NetIface set macType_ Mac/Csma/Cd

NetIface instproc ifqType {val} { $self set ifqType_ $val }
NetIface instproc macType {val} { $self set macType_ $val }

NetIface instproc init {node bw args} {
	eval $self next $args
	$self instvar ifqType_ macType_
	$self instvar node_ lcl_ ifq_ mac_ drpT_ deqT_

	set node_ $node
	set lcl_ [new Classifier]
	set ifq_ [new $ifqType_]
	set mac_ [new $macType_]
	$lcl_ set offset_ [PktHdr_offset PacketHeader/Mac macSA_]
	$mac_ set bandwidth_ $bw

	$mac_ target $lcl_
	$ifq_ target $mac_
	$self target $ifq_
}

NetIface instproc trace {ns f} {
	$self instvar node_ lcl_ ifq_ mac_ drpT_ deqT_

	set id [$node_ id]
	[set drpT_ [new TraceIp/Drop $id]] attach $f
	[set deqT_ [new TraceIp/Deque $id]] attach $f
	$deqT_ target [$ifq_ target]
	$ifq_ target $deqT_
	$ifq_ drop-target $drpT_
	$mac_ drop-target $drpT_
}


#
# LanLink:  a LAN abstract
#
Class LanLink -superclass NsObject
LanLink set llType_ LL
LanLink set ifqType_ Queue/DropTail
LanLink set macType_ Mac/Csma/Cd
LanLink set chanType_ Channel

LanLink instproc llType {val} { $self set llType_ $val }
LanLink instproc ifqType {val} { $self set ifqType_ $val }
LanLink instproc macType {val} { $self set macType_ $val }
LanLink instproc chanType {val} { $self set chanType_ $val }

LanLink instproc init {ns args} {
	$self instvar llType_ ifqType_ macType_ chanType_
	$self instvar ns_ nodelist_ id_ channel_ mcl_ netIface_
	eval $self next $args

	set ns_ $ns
	set nodelist_ ""
	set id_ 0
	set channel_ [new $chanType_]
	set mcl_ [new Classifier/Mac]
	$mcl_ set offset_ [PktHdr_offset PacketHeader/Mac macDA_]
	$channel_ target $mcl_
}

LanLink instproc trace {ns f} {
	$self instvar ns_ nodelist_ id_ channel_ mcl_ netIface_
	[set drpT_ [new TraceIp/Corrupt]] attach $f
	$channel_ drop-target $drpT_
	foreach node $nodelist_ {
		$netIface_($node) trace $ns $f
	}
}

LanLink instproc netIface {node} {
	return [$self set netIface_($node)]
}

# addNode:  add a new node to the LAN by creating LL, IFQ, MAC...
LanLink instproc addNode {node bw delay {nif ""} {sllType ""} {dllType ""}} {
	$self instvar llType_ ifqType_ macType_ chanType_
	$self instvar ns_ nodelist_ id_ channel_ mcl_ netIface_
	$ns_ instvar link_

	if {$sllType == ""} { set sllType $llType_ }
	if {$dllType == ""} { set dllType $llType_ }
	if {$nif == ""} {
		set nif [new NetIface $node $bw \
				-ifqType $ifqType_ -macType $macType_]
	} 
	set netIface_($node) $nif
	set mac [$nif set mac_]
	$mac set addr_ [incr id_]
	$mac channel $channel_
	$mac classifier $mcl_
	$mcl_ install $id_ $mac
	$node addmac $mac

	set src $node
	set sid [$src id]
	foreach dst $nodelist_ {
		set did [$dst id]
		set sl [set link_($sid:$did) [new Link/LanDuplex $src $dst \
				$bw $delay [new Queue/DropTail] $sllType]]
		set dl [set link_($did:$sid) [new Link/LanDuplex $dst $src \
				$bw $delay [new Queue/DropTail] $dllType]]

		set macDA [[$netIface_($dst) set mac_] set addr_]
		$self setupLL $src $dst [$sl link] $macDA
		$self setupLL $dst $src [$dl link] $id_
	}
	lappend nodelist_ $node
}

LanLink instproc setupLL {src dst ll macDA} {
	$self instvar ns_ netIface_
	set nif $netIface_($src)
	$ll ifq [$nif set ifq_]
	$ll sendtarget $nif
	$ll recvtarget [$src entry]
	$ll set macDA_ $macDA
	[$nif set lcl_] install $macDA $ll

	# setup tracing after setting up linkage
	$ns_ trace-queue $src $dst
	$ns_ namtrace-queue $src $dst
}

LanLink instproc install-error {em {src ""} {dst ""}} {
	$self instvar ns_ channel_
	if {$src == ""} {
		$em target [$channel_ target]
		$channel_ target $em
	} else {
		set ll [[$ns_ link $src $dst] link]
		$em target [$ll target]
		$ll target $em
	}
}


LanLink instproc create-error { src dstlist emname rate unit {trans ""}} {
	if { $trans == "" } {
		set trans [list 0.5 0.5]
	}

	# default is exponential errmodel
	if { $emname == "uniform" } {
		set e1 [new ErrorModel/Uniform $rate $unit] 
		set e2 [new ErrorModel/Uniform $rate $unit] 
	} elseif { $emname == "2state" } {
		set e1 [new ErrorModel/MultiState/TwoStateMarkov $rate $trans \
				$unit]
		set e2 [new ErrorModel/MultiState/TwoStateMarkov $rate $trans \
				$unit]
	} else {
		set e1 [new ErrorModel/Expo $rate $unit]
		set e2 [new ErrorModel/Expo $rate $unit]
	}       
	
	foreach dst $dstlist {
		$self install-error $src $dst $e1
		$self install-error $dst $src $e2
	}
}

LanLink instproc init-monitor {ns qfile sampleInt src {dst ""}} {
	$self instvar ns_ ifq_
	$self instvar qfile_ sampleInterval_ qm_ si_ so_ sd_
	$ns instvar link_

	set ifq ifq_($src)
	set qfile_($src) $qfile
	set sampleInterval_($src) $sampleInt
	set qm [set qm_($src) [new QueueMonitor]]
	set si [set si_($src) [new SnoopQueue/In]]
	set so [so_($src) [new SnoopQueue/Out]]
	set sd [sd_($src) [new SnoopQueue/Drop]]

	$si set-monitor $qm
	$so set-monitor $qm
	$sd set-monitor $qm

	$si target $ifq
	foreach dst $nodelist_ {
		if {$src == $dst} continue
		[$link_([$src id]:[$dst id]) link] target $si
	}

	$so target [$ifq target]
	$ifq target $so_($src)

	if [info exists drpT_] {
		$sd_($src) target [$drpT_($src) target]
		$drpT_($src) target $sd_($src)
	} else {
		$sd_($src) target [ns set nullAgent_]
		ifq_($src) drop-target $sd_($src)
	}

	set bytesInt_ [new BytesIntegrator]
	$qMonitor_ set-bytes-integrator $bytesInt_
	set pktsInt_ [new BytesIntegrator]
	$qMonitor_ set-pkts-integrator $pktsInt_
	return $qMonitor_
}

LanLink instproc queue-sample-timeout {} {
	$self instvar ns_ qfile_ sampleInterval_
	$self instvar qm_

	set nodelist [array names qm_]
	set numnodes [llength $nodelist]
	for {set i 0} {$i < $numnodes} {incr i} {
		set src [lindex $nodelist $i]
		set qavg [$self sample-queue-size $src]
		puts $qfile_($src) "[$ns_ now] n[$src id]:n* $qavg"
	}
	$ns_ at [expr [$ns_ now] + $sampleInterval_($src)] \
			"$self queue-sample-timeout"
}

LanLink instproc sample-queue-size {src} {
	$self instvar ns_ qfile_ sampleInterval_
	$self instvar qm_

	set now [$ns_ now]
	set qBytesMonitor_ [$qm_($src) get-bytes-integrator]
	set qPktsMonitor_ [$qm_($src) get-pkts-integrator]
	
	$qBytesMonitor_ newpoint $now [$qBytesMonitor_ set lasty_]
	set bsum [$qBytesMonitor_ set sum_]

	$qPktsMonitor_ newpoint $now [$qPktsMonitor_ set lasty_]
	set psum [$qPktsMonitor_ set sum_]

	if ![info exists lastSample_] {
		set lastSample_ 0
	}
	set dur [expr $now - $lastSample_]
	if { $dur != 0 } {
		set meanBytesQ [expr $bsum / $dur]
		set meanPktsQ [expr $psum / $dur]
	} else {
		set meanBytesQ 0
		set meanPktsQ 0
	}
	$qBytesMonitor_ set sum_ 0.0
	$qPktsMonitor_ set sum_ 0.0
	set lastSample_ $now

	return "$meanBytesQ $meanPktsQ"
}

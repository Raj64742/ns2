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

# Defaults for link-layer are in ns-ll.tcl

# TraceIp trace IP packet headers for LAN components
TraceIp set src_ -1
TraceIp set dst_ -1
TraceIp set callback_ 0
TraceIp set show_tcphdr_ 0
TraceIp set mask_ 0xffffffff
#TraceIp set shift_ 8
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
	$lan addNode $nodelist $bw $delay

        # added for nam trace purpose
	$self instvar LanLinks_
	set LanLinks_([$lan nid]) $lan

	return $lan
}

# For convenience, use make-lan.  For more fine-grained control,
# use newLan instead of make-lan.
Simulator instproc make-lan {nodelist bw delay {llType LL} \
		{ifqType Queue/DropTail} {macType Mac} {chanType Channel}} {
	set lan [new LanLink $self -llType $llType -ifqType $ifqType \
			-macType $macType -chanType $chanType]
	$lan addNode $nodelist $bw $delay $llType $ifqType $macType

	# added for nam trace purpose
	$self instvar LanLinks_
	set LanLinks_([$lan nid]) $lan

	return $lan
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

	set drpT_ [$ns create-trace Loss $f $toNode_ $fromNode_]
	set namtrfd [$ns get-nam-traceall]
	if {$namtrfd != ""} {
		$drpT_ namattach $namtrfd
	}
	$link_ drop-target $drpT_
}


Class NetIface -superclass Connector
NetIface set ifqType_ Queue/DropTail
NetIface set macType_ Mac/Csma/Cd
NetIface instproc ifqType {val} { $self set ifqType_ $val }
NetIface instproc macType {val} { $self set macType_ $val }

NetIface instproc init {node bw args} {
	set args [eval $self init-vars $args]
	eval $self next $args
	$self instvar ifqType_ macType_
	$self instvar node_ lcl_ ifq_ mac_ drpT_ deqT_

	set node_ $node
	set lcl_ [new Classifier]
	$lcl_ set offset_ [PktHdr_offset PacketHeader/Mac macSA_]
	set ifq_ [new $ifqType_]
	set mac_ [new $macType_]
	$mac_ set bandwidth_ $bw

	$mac_ target $lcl_
	$ifq_ target $mac_
	$self target $ifq_
}

NetIface instproc trace {ns f} {
	$self instvar node_ lcl_ ifq_ mac_ drpT_ deqT_
	if [info exists drpT_] return

	set id [$node_ id]
	[set drpT_ [new TraceIp/Drop $id]] attach $f
	[set deqT_ [new TraceIp/Deque $id]] attach $f
	$deqT_ target [$ifq_ target]
	$ifq_ target $deqT_
	$ifq_ drop-target $drpT_
	$mac_ drop-target $drpT_
}

NetIface instproc install-error {em macSA} {
	$self instvar lcl_
	$em target [$lcl_ slot $macSA]
	$lcl_ install $macSA $em
}


#
# LanLink:  a LAN abstraction
#
Class LanLink -superclass InitObject
LanLink set llType_ LL
LanLink set ifqType_ Queue/DropTail
LanLink set macType_ Mac/Csma/Cd
LanLink set chanType_ Channel

LanLink instproc llType {val} { $self set llType_ $val }
LanLink instproc ifqType {val} { $self set ifqType_ $val }
LanLink instproc macType {val} { $self set macType_ $val }
LanLink instproc chanType {val} { $self set chanType_ $val }

LanLink instproc init {ns args} {
	set args [eval $self init-vars $args]
	$self instvar llType_ ifqType_ macType_ chanType_
	$self instvar ns_ nodelist_
	$self instvar id_ channel_ mcl_ netIface_
	$self instvar nid_

	set ns_ $ns
	set nodelist_ ""
	set id_ 0
	set nid_ [Node getid]
	set channel_ [new $chanType_]
	set mcl_ [new Classifier/Mac]
	$mcl_ set offset_ [PktHdr_offset PacketHeader/Mac macDA_]
	$channel_ target $mcl_
}

LanLink instproc nid {} {
	$self instvar nid_
	return $nid_
}

LanLink instproc trace {{ns ""} {f ""}} {
	$self instvar ns_ nodelist_
	$self instvar id_ channel_ mcl_ netIface_ drpT_

	if [info exists drpT_] {
		puts "LanLink $self already setup tracing"
		return
	}
	if {$ns == ""} { set ns $ns_ }
	if {$f == ""} { set f [$ns_ get-ns-traceall] }
	set drpT_ [new TraceIp/Corrupt]
	$drpT_ attach $f
	$channel_ drop-target $drpT_
	foreach src $nodelist_ {
		$netIface_($src) trace $ns $f
	}
}

LanLink instproc netIface {node} {
	return [$self set netIface_($node)]
}

# addNode:  add a new node to the LAN by creating LL, IFQ, MAC...
LanLink instproc addNode {nodes bw delay {sllType ""} \
		{ifqType ""} {macType ""} {dllType ""}} {
	$self instvar llType_ ifqType_ macType_ chanType_
	$self instvar id_ channel_ mcl_ netIface_
	$self instvar ns_ nodelist_
	$ns_ instvar link_
	$self instvar bw_ delay_

	set bw_ $bw
	set delay_ $delay

	if {$ifqType == ""} { set ifqType $ifqType_ }
	if {$macType == ""} { set macType $macType_ }
	if {$sllType == ""} { set sllType $llType_ }
	if {$dllType == ""} { set dllType $llType_ }


	foreach src $nodes {
		set nif [new NetIface $src $bw \
				-ifqType $ifqType -macType $macType]
		set mac [$nif set mac_]
		$mac set addr_ [incr id_]
		$mac channel $channel_
		$mac classifier $mcl_
		$mcl_ install $id_ $mac
		$src addmac $mac
		set netIface_($src) $nif

		set sid [$src id]
		foreach dst $nodelist_ {
			set did [$dst id]
			set link_($sid:$did) [new Link/LanDuplex $src $dst \
				$bw $delay [new Queue/DropTail] $sllType]
			set link_($did:$sid) [new Link/LanDuplex $dst $src \
				$bw $delay [new Queue/DropTail] $dllType]

			$self attachLL $src $dst
			$self attachLL $dst $src
		}
		lappend nodelist_ $src
	}
}

LanLink instproc attachLL {src dst} {
	$self instvar ns_ netIface_

	set nif $netIface_($src)
	set macDA [[$netIface_($dst) set mac_] set addr_]
	set ll [[$ns_ link $src $dst] link]
	$ll ifq [$nif set ifq_]
	$ll sendtarget $nif
	$ll recvtarget [$src entry]
	$ll set macDA_ $macDA
	[$nif set lcl_] install $macDA $ll

	# setup tracing after setting up linkage
	$nif trace $ns_ [$ns_ get-ns-traceall]
	$ns_ trace-queue $src $dst
	$ns_ namtrace-queue $src $dst
}


LanLink instproc install-error {em {src ""} {dst ""}} {
	$self instvar ns_ channel_ netIface_
	if {$dst == ""} {
		$em target [$channel_ target]
		$channel_ target $em
	} else {
		set macSA [[$netIface_($src) set mac_] set addr_]
		$netIface_($dst) install-error $em $macSA
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

# XXX, defined the same as ns-namsupp.tcl,  

LanLink instproc dump-namconfig {} {
        $self instvar nid_ bw_ delay_ nodelist_ ns_
        
        $ns_ puts-nam-config \
        "X -t * -n $nid_ -r $bw_ -D $delay_ -o left"
        
        set cnt 0
        set LanOrient(0) up
        set LanOrient(1) down
            
        foreach dst $nodelist_ {
            $ns_ puts-nam-config \
            "L -t * -s $nid_ -d [$dst id] -o $LanOrient($cnt)"
            incr cnt
            set cnt [expr $cnt % 2]
        }

}


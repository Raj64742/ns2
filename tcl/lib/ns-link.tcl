#
# Copyright (c) 1996 Regents of the University of California.
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
# 	This product includes software developed by the MASH Research
# 	Group at the University of California Berkeley.
# 4. Neither the name of the University nor of the Research Group may be
#    used to endorse or promote products derived from this software without
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-link.tcl,v 1.14 1997/05/13 22:27:57 polly Exp $
#
Class Link
Link instproc init { src dst } {
	$self next

        #modified for interface code
	$self instvar trace_ fromNode_ toNode_ source_ dest_
	set fromNode_ [$src getNode]
	set toNode_ [$dst getNode]
        set source_ $src
        set dest_ $dst

	set trace_ ""
}

Link instproc head {} {
	$self instvar head_
	return $head_
}

Link instproc queue {} {
	$self instvar queue_
	return $queue_
}

Link instproc link {} {
	$self instvar link_
	return $link_
}

Class SimpleLink -superclass Link

SimpleLink instproc init { src dst bw delay q {lltype "DelayLink"} } {
	$self next $src $dst
	$self instvar link_ queue_ head_ toNode_ ttl_

        #added for interface code
        $self instvar ifacein_ ifaceout_
        set ifacein_ 0
        set ifaceout_ 0

	set queue_ $q
	set link_ [new $lltype]
	$link_ set bandwidth_ $bw
	$link_ set delay_ $delay

	$queue_ target $link_
	$link_ target [$dst entry]

	#set head_ $queue_ -> replace by the following
        if { [$src info class] == "DuplexNetInterface" } {
                set head_ [$src exitpoint]
                set ifacein_ $head_
                $head_ target $queue_
        } else {
                set head_ $queue_
        }

	# XXX
	# put the ttl checker after the delay
	# so we don't have to worry about accounting
	# for ttl-drops within the trace and/or monitor
	# fabric
	#
	set ttl_ [new TTLChecker]
	$ttl_ target [$link_ target]
	$link_ target $ttl_
}

#
# Build trace objects for this link and
# update the object linkage
#
SimpleLink instproc trace { ns f } {
	$self instvar enqT_ deqT_ drpT_ queue_ link_ head_ fromNode_ toNode_
	set enqT_ [$ns create-trace Enque $f $fromNode_ $toNode_]
	set deqT_ [$ns create-trace Deque $f $fromNode_ $toNode_]
	set drpT_ [$ns create-trace Drop $f $fromNode_ $toNode_]

	$drpT_ target [$queue_ drop-target]
	$queue_ drop-target $drpT_

	$deqT_ target [$queue_ target]
	$queue_ target $deqT_

	#$enqT_ target $head_
	#set head_ $enqT_       -> replaced by the following
        if { [$head_ info class] == "networkinterface" } {
	    $enqT_ target [$head_ target]
	    $head_ target $enqT_
	    # puts "head is i/f"
        } else {
	    $enqT_ target $head_
	    set head_ $enqT_
	    # puts "head is not i/f"
	}

	$self instvar dynamics_
	if [info exists dynamics_] {
		$self trace-dynamics $ns $f
	}
}
#
# like init-monitor, but allows for specification of more of the items
# attach-monitors $insnoop $inqm $outsnoop $outqm $dropsnoop $dropqm
#
SimpleLink instproc attach-monitors { insnoop outsnoop dropsnoop qmon } {
	$self instvar drpT_ queue_ head_ snoopIn_ snoopOut_ snoopDrop_
	$self instvar qMonitor_

	set snoopIn_ $insnoop
	set snoopOut_ $outsnoop
	set snoopDrop_ $dropsnoop

	$snoopIn_ target $head_
	set head_ $snoopIn_

	$snoopOut_ target [$queue_ target]
	$queue_ target $snoopOut_

	if [info exists drpT_] {
		$snoopDrop_ target [$drpT_ target]
		$drpT_ target $snoopDrop_
		$queue_ drop-target $drpT_
	} else {
		$snoopDrop_ target [ns set nullAgent_]
		$queue_ drop-target $snoopDrop_
	}

	$snoopIn_ set-monitor $qmon
	$snoopOut_ set-monitor $qmon
	$snoopDrop_ set-monitor $qmon
	set qMonitor_ $qmon
}

#
# Insert objects that allow us to monitor the queue size
# of this link.  Return the name of the object that
# can be queried to determine the average queue size.
#
SimpleLink instproc init-monitor ns {
	$self instvar qMonitor_

	set qMonitor_ [new QueueMonitor]

	$self attach-monitors [new SnoopQueue/In] \
		[new SnoopQueue/Out] [new SnoopQueue/Drop] $qMonitor_

	set bytesInt_ [new Integrator]
	$qMonitor_ set-bytes-integrator $bytesInt_
	set pktsInt_ [new Integrator]
	$qMonitor_ set-pkts-integrator $pktsInt_
	return $qMonitor_
}

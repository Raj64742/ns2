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

# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-lib.tcl,v 1.107 1998/06/24 23:42:24 kfall Exp $

#

#
# Word of warning to developers:
# this code (and all it sources) is compiled into the
# ns executable.  You need to rebuild ns or explicitly
# source this code to see changes take effect.
#


proc warn msg {
	global warned_
	if ![info exists warned_($msg)] {
		puts stderr $msg
		set warned($msg) 1
	}
}

if {[info commands debug] == ""} {
	proc debug args {
		warn {Script debugging disabled.  Reconfigure with --with-tcldebug, and recompile.}
	}
}

proc find-max list {
	set max 0
	foreach val $list {
		if {$val > $max} {
			set max $val
		}
	}
	return $max
}


#
# Create the core OTcl class called "Simulator".
# This is the principal interface to the simulation engine.
#
Class Simulator

source ns-address.tcl
source ns-node.tcl
source ns-hiernode.tcl
source ns-link.tcl
source ns-source.tcl
source ns-compat.tcl
source ns-nam.tcl
source ns-packet.tcl
source ns-queue.tcl
source ns-trace.tcl
source ns-agent.tcl
source ns-random.tcl
source ns-route.tcl
source ns-errmodel.tcl
source ns-intserv.tcl
source ../rtp/session-rtp.tcl
source ../interface/ns-iface.tcl
source ../lan/ns-mlink.tcl
source ../lan/ns-mac.tcl
source ../lan/ns-lan.tcl
source ../lan/ns-ll.tcl
source ../mcast/timer.tcl
source ../mcast/ns-mcast.tcl
source ../mcast/McastProto.tcl
source ../mcast/DM.tcl
source ../mcast/dynamicDM.tcl
source ../mcast/pimDM.tcl
source ../mcast/detailedDM.tcl
source ../ctr-mcast/CtrMcast.tcl
source ../ctr-mcast/CtrMcastComp.tcl
source ../ctr-mcast/CtrRPComp.tcl
source ../pim/pim-init.tcl
source ../pim/pim-messagers.tcl
source ../pim/pim-mfc.tcl
source ../pim/pim-mrt.tcl
source ../pim/pim-recvr.tcl
source ../pim/pim-sender.tcl
source ../pim/pim-vifs.tcl
source ../mcast/srm.tcl
source ../mcast/srm-ssm.tcl
source ../mcast/McastMonitor.tcl
source ../rlm/rlm.tcl
source ../rlm/rlm-ns.tcl
source ../session/session.tcl
source ns-namsupp.tcl

source ns-default.tcl
source ../emulate/ns-emulate.tcl

Simulator instproc init args {
	$self create_packetformat
	$self use-scheduler Calendar
	$self set nullAgent_ [new Agent/Null]
	$self set-address-format def
	eval $self next $args
}

Simulator instproc nullagent {} {
	$self instvar nullAgent_
	return $nullAgent_
}

Simulator instproc use-scheduler type {
	$self instvar scheduler_
	if [info exists scheduler_] {
		if { [$scheduler_ info class] == "Scheduler/$type" } {
			return
		} else {
			delete $scheduler_
		}
	}
	set scheduler_ [new Scheduler/$type]
	$scheduler_ now
}

#
# A simple method to wrap any object around
# a trace object that dumps to stdout
#
Simulator instproc dumper obj {
	set t [$self alloc-trace hop stdout]
	$t target $obj
	return $t
}

# Default behavior is changed: consider nam as not initialized if 
# no shape OR color parameter is given
Simulator instproc node args {
	$self instvar Node_
	set node [new [Simulator set node_factory_]]
	set Node_([$node id]) $node
	if [Simulator set EnableMcast_] {
		$node enable-mcast $self
	}
	$self check-node-num
	return $node
}

Simulator instproc hier-node haddr {
 	error "now create hier-nodes with just [$ns_ node $haddr]"
}

Simulator instproc now {} {
	$self instvar scheduler_
	return [$scheduler_ now]
}

Simulator instproc at args {
	$self instvar scheduler_
	return [eval $scheduler_ at $args]
}

Simulator instproc at-now args {
	$self instvar scheduler_
	return [eval $scheduler_ at-now $args]
}

Simulator instproc cancel args {
	$self instvar scheduler_
	return [eval $scheduler_ cancel $args]
}

Simulator instproc dump-namagents {} {
	$self instvar tracedAgents_ monitoredAgents_
	
	if ![$self is-started] {
		return
	}
	if [info exists tracedAgents_] {
		foreach id [array names tracedAgents_] {
			$tracedAgents_($id) add-agent-trace $id
		}
		unset tracedAgents_
	}
	if [info exists monitoredAgents_] {
		foreach a $monitoredAgents_ {
			$a show-monitor
		}
		unset monitoredAgents_
	}
}

Simulator instproc dump-namversion { v } {
	$self puts-nam-config "V -t * -v $v -a 0"
}

Simulator instproc dump-namcolors {} {
	$self instvar color_
	if ![$self is-started] {
		return 
	}
	foreach id [array names color_] {
		$self puts-nam-config "c -t * -i $id -n $color_($id)"
	}
}

Simulator instproc dump-namlans {} {
	$self instvar lanConfigList_
	if ![$self is-started] {
		return
	}
	if [info exists lanConfigList_] {
		foreach lan $lanConfigList_ {
			$lan dump-namconfig
		}
		unset lanConfigList_
	}
}

Simulator instproc dump-namlinks {} {
	$self instvar linkConfigList_
	if ![$self is-started] {
		return
	}
	if [info exists linkConfigList_] {
		foreach lnk $linkConfigList_ {
			$lnk dump-namconfig
		}
		unset linkConfigList_
	}
}

Simulator instproc dump-namnodes {} {
	$self instvar Node_
	if ![$self is-started] {
		return
	}
	foreach nn [array names Node_] {
		$Node_($nn) dump-namconfig
	}
}

Simulator instproc dump-namqueues {} {
	$self instvar link_
	if ![$self is-started] {
		return
	}
	foreach qn [array names link_] {
		$link_($qn) dump-nam-queueconfig
	}
}

# Write hierarchical masks/shifts into trace file
Simulator instproc dump-namaddress {} {
	AddrParams instvar hlevel_ NodeShift_ NodeMask_ PortShift_ PortMask_ \
	    McastShift_ McastMask_
	
	# First write number of hierarchies
	$self puts-nam-config \
	    "A -t * -n $hlevel_ -p $PortShift_ -o $PortMask_ -c $McastShift_ -a $McastMask_"
	
	for {set i 1} {$i <= $hlevel_} {incr i} {
		$self puts-nam-config \
		    "A -t * -h $i -m $NodeMask_($i) -s $NodeShift_($i)"
	}
}

#
# check if total num of nodes exceed 2 to the power n 
# where <n=node field size in address>
#
Simulator instproc check-node-num {} {
	AddrParams instvar nodebits_ 
	if {[Node set nn_] > [expr pow(2, $nodebits_)]} {
		error "Number of nodes exceeds node-field-size of $nodebits_ bits"
	}
	if [Simulator set EnableHierRt_] {
		$self chk-hier-field-lengths
	}
}

#
# Check if number of items at each hier level (num of nodes, or clusters or domains)
# exceed size of that hier level field size (in bits). should be modified to support 
# n-level of hierarchies
#
Simulator instproc chk-hier-field-lengths {} {
	AddrParams instvar domain_num_ cluster_num_ nodes_num_ NodeMask_
	
	if [info exists domain_num_] {
		if {[expr $domain_num_ - 1]> $NodeMask_(1)} {
			error "\# of domains exceed dom-field-size "
		}
	} 
	if [info exists cluster_num_] {
		set maxval [expr [find-max $cluster_num_] - 1] 
		if {$maxval > [expr pow(2, $NodeMask_(2))]} {
			error "\# of clusters exceed clus-field-size "
		}
	}
	if [info exists nodes_num_] {
		set maxval [expr [find-max $nodes_num_] -1]
		if {$maxval > [expr pow(2, $NodeMask_(3))]} {
			error "\# of nodess exceed node-field-size"
		}
	}
}


Simulator instproc run {} {
	#$self compute-routes

	$self check-node-num
	$self rtmodel-configure			;# in case there are any
	[$self get-routelogic] configure
	$self instvar scheduler_ Node_ link_ started_ 
	
	set started_ 1
	
	#
	# Reset every node, which resets every agent.
	#
	foreach nn [array names Node_] {
		$Node_($nn) reset
	}
	#
	# Also reset every queue
	#
	foreach qn [array names link_] {
		set q [$link_($qn) queue]
		$q reset
	}

	# Setting nam trace file version first
	$self dump-namversion 1.0b6
	
	# Addressing scheme
	$self dump-namaddress
	
	# Color configuration for nam
	$self dump-namcolors
	
	# Node configuration for nam
	$self dump-namnodes
	
	# Lan and Link configurations for nam
	$self dump-namlinks 
	$self dump-namlans
	
	# nam queue configurations
	$self dump-namqueues
	
	# Traced agents for nam
	$self dump-namagents

    return [$scheduler_ run]
}

Simulator instproc halt {} {
	$self instvar scheduler_
	$scheduler_ halt
}

Simulator instproc dumpq {} {
	$self instvar scheduler_
	$scheduler_ dumpq
}

Simulator instproc is-started {} {
	$self instvar started_
	return [info exists started_]
}

Simulator instproc clearMemTrace {} {
	$self instvar scheduler_
	$scheduler_ clearMemTrace
}

Simulator instproc simplex-link { n1 n2 bw delay arg } {
	$self instvar link_ queueMap_ nullAgent_
	set sid [$n1 id]
	set did [$n2 id]
	
	# XXX the following is an absolutely disgusting hack,
	# but difficult to avoid for the moment (kf)
	# idea: if arg (formerly type) is "QTYPE stuff", split
	# the string so type is QTYPE and "stuff" is passed along
	#
	set argsz [llength $arg]
	if { $argsz == 1 } {
		set type $arg
	} else {
		if { [ lindex $arg 0] == "intserv" } {
			set type [lindex $arg 1]
		} else {
			set type [lindex $arg 0]
			set larg [lindex $arg 1]
		}
	}
	if [info exists queueMap_($type)] {
		set type $queueMap_($type)
	}
	if [$class set NumberInterfaces_] {
		$self instvar interfaces_
		if ![info exists interfaces_($n1:$n2)] {
			set interfaces_($n1:$n2) [new DuplexNetInterface]
			set interfaces_($n2:$n1) [new DuplexNetInterface]
			$n1 addInterface $interfaces_($n1:$n2)
			$n2 addInterface $interfaces_($n2:$n1)
		}
		set nd1 $interfaces_($n1:$n2)
		set nd2 $interfaces_($n2:$n1)
	} else {
		set nd1 $n1
		set nd2 $n2
	}

	
	# XXX more disgusting hack
	if { [string first "ErrorModule" $type] != 0 } {
		set q [new Queue/$type]
	} else {
		if { $argsz > 1 } {
			set q [eval new $type $larg]
		} else {
			set q [new $type Fid]
		}
	}

	if { $argsz != 1 } {
		# assume we have a string of form "linktype linkarg"
		if { $type == "RTM" || $type == "CBQ" || $type == "CBQ/WRR" } {
			set link_($sid:$did) [new CBQLink $nd1 $nd2 $bw $delay $q $larg]
		}
		#XX need to clean this up
		if { [lindex $arg 0] == "intserv" } {
			set link_($sid:$did) [new IntServLink $nd1 $nd2 $bw $delay $q $arg]
		}
	} else {
		if { $type == "CBQ" || $type == "CBQ/WRR" } {
			# default classifier for cbq is just Fid type
			set classifier [new Classifier/Hash/Fid 33]
			set link_($sid:$did) [new CBQLink $nd1 $nd2 $bw $delay $q $classifier]
		} else {
			set link_($sid:$did) [new SimpleLink $nd1 $nd2 $bw $delay $q]
		}
	}
	$n1 add-neighbor $n2
	
	#XXX yuck
	if {[string first "RED" $type] != -1} {
		$q link [$link_($sid:$did) set link_]
	}
	
	set trace [$self get-ns-traceall]
	if {$trace != ""} {
		$self trace-queue $n1 $n2 $trace
	}
	set trace [$self get-nam-traceall]
	if {$trace != ""} {
		$self namtrace-queue $n1 $n2 $trace
	}
}

# 
# Assumes every lan is a MultiLink. We keep pointers to all lan created 
# during startup and call MultiLink::dump-namconfig in Simulator::run
#
Simulator instproc register-nam-lanconfig mlink {
	$self instvar lanConfigList_
	
	if [info exists lanConfigList_] {
		if {[lsearch $lanConfigList_ $mlink] >= 0} {
			# This lan is already there
			return
		}
	}
	lappend lanConfigList_ $mlink
}

#
# This is used by Link::orient to register/update the order in which links 
# should created in nam. This is important because different creation order
# may result in different layout.
#
# A poor hack. :( Any better ideas?
#
Simulator instproc register-nam-linkconfig link {
    $self instvar linkConfigList_ link_
    if [info exists linkConfigList_] {
	# Check whether the reverse simplex link is registered,
	# if so, don't register this link again.
	# We should have a separate object for duplex link.
	set i1 [[$link fromNode] id]
	set i2 [[$link toNode] id]
	if [info exists link_($i2:$i1)] {
	    set pos [lsearch $linkConfigList_ $link_($i2:$i1)]
	    if {$pos >= 0} {
		set a1 [$link_($i2:$i1) get-attribute "ORIENTATION"]
		set a2 [$link get-attribute "ORIENTATION"]
		if {$a1 == "" && $a2 != ""} {
		    # If this duplex link has not been 
		    # assigned an orientation, do it.
		    set linkConfigList_ \
			[lreplace $linkConfigList_ $pos $pos]
		} else {
		    return
		}
	    }
	}

	# Remove $link from list if it's already there
	set pos [lsearch $linkConfigList_ $link]
	if {$pos >= 0} {
	    set linkConfigList_ \
		[lreplace $linkConfigList_ $pos $pos]
	}
    }
    lappend linkConfigList_ $link
}

#
# GT-ITM may occasionally generate duplicate links, so we need this check
# to ensure duplicated links do not appear in nam trace files.
#
Simulator instproc remove-nam-linkconfig {i1 i2} {
	$self instvar linkConfigList_ link_
	if ![info exists linkConfigList_] {
		return
	}
	set pos [lsearch $linkConfigList_ $link_($i1:$i2)]
	if {$pos >= 0} {
		set linkConfigList_ [lreplace $linkConfigList_ $pos $pos]
		return
	}
	set pos [lsearch $linkConfigList_ $link_($i2:$i1)]
	if {$pos >= 0} {
		set linkConfigList_ [lreplace $linkConfigList_ $pos $pos]
	}
}

Simulator instproc duplex-link { n1 n2 bw delay type } {
	$self instvar link_
	set i1 [$n1 id]
	set i2 [$n2 id]
	if [info exists link_($i1:$i2)] {
		$self remove-nam-linkconfig $i1 $i2
	}

	$self simplex-link $n1 $n2 $bw $delay $type
	$self simplex-link $n2 $n1 $bw $delay $type
	
	# nam only has duplex link, so we do a registration here.
	$self register-nam-linkconfig $link_($i1:$i2)
}

Simulator instproc duplex-intserv-link { n1 n2 bw pd sched signal adc args } {
	$self  duplex-link $n1 $n2 $bw $pd "intserv $sched $signal $adc $args"
}

Simulator instproc duplex-link-op { n1 n2 op args } {
	$self instvar link_
	eval $link_([$n1 id]:[$n2 id]) $op $args
	eval $link_([$n2 id]:[$n1 id]) $op $args
}

Simulator instproc flush-trace {} {
	$self instvar alltrace_
	if [info exists alltrace_] {
		foreach trace $alltrace_ {
			$trace flush
		}
	}
}

Simulator instproc namtrace-all file {
	$self instvar namtraceAllFile_
	set namtraceAllFile_ $file
}

Simulator instproc namtrace-some file {
	$self instvar namtraceSomeFile_
	set namtraceSomeFile_ $file
}

Simulator instproc trace-all file {
	$self instvar traceAllFile_
	set traceAllFile_ $file
}

Simulator instproc get-nam-traceall {} {
	$self instvar namtraceAllFile_
	if [info exists namtraceAllFile_] {
		return $namtraceAllFile_
	} else {
		return ""
	}
}

Simulator instproc get-ns-traceall {} {
	$self instvar traceAllFile_
	if [info exists traceAllFile_] {
		return $traceAllFile_
	} else {
		return ""
	}
}

# If exists a traceAllFile_, print $str to $traceAllFile_
Simulator instproc puts-ns-traceall { str } {
	$self instvar traceAllFile_
	if [info exists traceAllFile_] {
		puts $traceAllFile_ $str
	}
}

# If exists a traceAllFile_, print $str to $traceAllFile_
Simulator instproc puts-nam-traceall { str } {
	$self instvar namtraceAllFile_
	if [info exists namtraceAllFile_] {
		puts $namtraceAllFile_ $str
	} elseif [info exists namtraceSomeFile_] {
		puts $namtraceSomeFile_ $str
	}
}

# namConfigFile is used for writing color/link/node/queue/annotations. 
# XXX It cannot co-exist with namtraceAll.
Simulator instproc namtrace-config { f } {
	$self instvar namConfigFile_
	set namConfigFile_ $f
}

Simulator instproc get-nam-config {} {
	$self instvar namConfigFile_
	if [info exists namConfigFile_] {
		return $namConfigFile_
	} else {
		return ""
	}
}

# Used only for writing nam configurations to trace file(s). This is different
# from puts-nam-traceall because we may want to separate configuration 
# informations and actual tracing information
Simulator instproc puts-nam-config { str } {
	$self instvar namtraceAllFile_ namConfigFile_
	
	if [info exists namConfigFile_] {
		puts $namConfigFile_ $str
	} elseif [info exists namtraceAllFile_] {
		puts $namtraceAllFile_ $str
	} elseif [info exists namtraceSomeFile_] {
		puts $namtraceSomeFile_ $str
	}
}

Simulator instproc color { id name } {
	$self instvar color_
	set color_($id) $name
}

Simulator instproc get-color { id } {
	$self instvar color_
	return $color_($id)
}

# you can pass in {} as a null file
Simulator instproc create-trace { type file src dst {op ""} } {
	$self instvar alltrace_
	set p [new Trace/$type]
	if [catch {$p set src_ [$src id]}] {
		$p set src_ $src
	}
	if [catch {$p set dst_ [$dst id]}] {
		$p set dst_ $dst
	}
	lappend alltrace_ $p
	if {$file != ""} {
		$p ${op}attach $file		
	}
	return $p
}

Simulator instproc namtrace-queue { n1 n2 {file ""} } {
	$self instvar link_ namtraceAllFile_
	if {$file == ""} {
		if ![info exists namtraceAllFile_] return
		set file $namtraceAllFile_
	}
	$link_([$n1 id]:[$n2 id]) nam-trace $self $file
}

Simulator instproc trace-queue { n1 n2 {file ""} } {
	$self instvar link_ traceAllFile_
	if {$file == ""} {
		if ![info exists traceAllFile_] return
		set file $traceAllFile_
	}
	$link_([$n1 id]:[$n2 id]) trace $self $file
}

#
# arrange for queue length of link between nodes n1 and n2
# to be tracked and return object that can be queried
# to learn average q size etc.  XXX this API still rough
#
Simulator instproc monitor-queue { n1 n2 qtrace { sampleInterval 0.1 } } {
	$self instvar link_
	return [$link_([$n1 id]:[$n2 id]) init-monitor $self $qtrace $sampleInterval]
}

Simulator instproc queue-limit { n1 n2 limit } {
	$self instvar link_
	[$link_([$n1 id]:[$n2 id]) queue] set limit_ $limit
}

Simulator instproc drop-trace { n1 n2 trace } {
	$self instvar link_
	[$link_([$n1 id]:[$n2 id]) queue] drop-target $trace
}

Simulator instproc cost {n1 n2 c} {
	$self instvar link_
	$link_([$n1 id]:[$n2 id]) cost $c
}

Simulator instproc attach-agent { node agent } {
	$node attach $agent
}

Simulator instproc attach-tbf-agent { node agent tbf } {
	$node attach $agent
	$agent attach-tbf $tbf
}


Simulator instproc detach-agent { node agent } {
	$self instvar nullAgent_
	$node detach $agent $nullAgent_
}

#
#   Helper proc for setting delay on an existing link
#
Simulator instproc delay { n1 n2 delay } {
	$self instvar link_
	set sid [$n1 id]
	set did [$n2 id]
	if [info exists link_($sid:$did)] {
		set d [$link_($sid:$did) link]
		$d set delay_ $delay
	}
}

#XXX need to check that agents are attached to nodes already
Simulator instproc connect {src dst} {
	$self simplex-connect $src $dst
	$self simplex-connect $dst $src
	return $src
}

Simulator instproc simplex-connect { src dst } {
	$src set dst_ [$dst set addr_] 
	return $src
}

#
# Here are a bunch of helper methods.
#


Simulator proc instance {} {
	set ns [Simulator info instances]
	if { $ns != "" } {
		return $ns
	}
	foreach sim [Simulator info subclass] {
		set ns [$sim info instances]
		if { $ns != "" } {
			return $ns
		}
	}
	error "Cannot find instance of simulator"
}

Simulator instproc get-node-by-id id {
	$self instvar Node_
	set Node_($id)
}

Simulator instproc all-nodes-list {} {
	$self instvar Node_
	set nodes ""
	foreach n [array names Node_] {
		lappend nodes $Node_($n)
	}
	set nodes
}

Simulator instproc link { n1 n2 } {
	$self instvar link_
	set link_([$n1 id]:[$n2 id])
}

# Creates connection. First creates a source agent of type s_type and binds
# it to source.  Next creates a destination agent of type d_type and binds
# it to dest.  Finally creates bindings for the source and destination agents,
# connects them, and  returns the source agent.
Simulator instproc create-connection {s_type source d_type dest pktClass} {
	set s_agent [new Agent/$s_type]
	set d_agent [new Agent/$d_type]
	$s_agent set fid_ $pktClass
	$d_agent set fid_ $pktClass
	$self attach-agent $source $s_agent
	$self attach-agent $dest $d_agent
	$self connect $s_agent $d_agent
	
	return $s_agent
}

Simulator instproc create-tcp-connection {s_type source d_type dest pktClass} {
	set s_agent [new Agent/$s_type]
	set d_agent [new Agent/$d_type]
	$s_agent set fid_ $pktClass
	$d_agent set fid_ $pktClass
	$self attach-agent $source $s_agent
	$self attach-agent $dest $d_agent
#	$self connect $s_agent $d_agent
	
	return "$s_agent $d_agent"
}

Classifier instproc no-slot slot {
	#XXX should say something better for routing problem
	puts stderr "$self: no target for slot $slot"
	exit 1
}


#
# Other classifier methods overload the instproc-likes to track 
# and return the installed objects.
#
Classifier instproc install {slot val} {
	$self set slots_($slot) $val
	$self cmd install $slot $val
}

Classifier instproc installNext val {
	set slot [$self cmd installNext $val]
	$self set slots_($slot) $val
	set slot
}

Classifier instproc adjacents {} {
	$self array get slots_
}

Classifier instproc in-slot? slot {
	$self instvar slots_
	set ret ""
	if {[array size slots_] < $slot} {
		set ret slots_($slot)
	}
	set ret
}

# dump is for debugging purposes
Classifier instproc dump {} {
	$self instvar slots_ offset_ shift_ mask_
	puts "classifier $self"
	puts "\t$offset_ offset"
	puts "\t$shift_ shift"
	puts "\t$mask_ mask"
	puts "\t[array size slots_] slots"
	foreach i [lsort -integer [array names slots_]] {
		set iv $slots_($i)
		puts "\t\tslot $i: $iv"
	}
}

Classifier/Hash instproc dump args {
	eval $self next $args
	$self instvar default_
	puts "\t$default_ default"
}

Classifier/Hash instproc init nbuck {
	# we need to make sure that port shift/mask values are there
	# so we set them after they get their default values
	$self next $nbuck
	$self instvar shift_ mask_
	set shift_ [AddrParams set NodeShift_(1)]
	set mask_ [AddrParams set NodeMask_(1)]
}

#
# To create: multi-access lan, and 
#            links with interface labels
#
Simulator instproc simplex-link-of-interfaces { f1 f2 bw delay type } {
	$self instvar link_ nullAgent_
	set n1 [$f1 getNode]
	set n2 [$f2 getNode]
	set sid [$n1 id]
	set did [$n2 id]
	set q [new Queue/$type]
	$q drop-target $nullAgent_
	set link_($sid:$did) [new SimpleLink $f1 $f2 $bw $delay $q]
	$n1 add-neighbor $n2
}

Simulator instproc duplex-link-of-interfaces { n1 n2 bw delay type {ori ""} {q_clrid 0} } {
	set f1 [new DuplexNetInterface]
	$n1 addInterface $f1
	set f2 [new DuplexNetInterface]
	$n2 addInterface $f2
	$self simplex-link-of-interfaces $f1 $f2 $bw $delay $type
	$self simplex-link-of-interfaces $f2 $f1 $bw $delay $type
	
	set trace [$self get-ns-traceall]
	if {$trace != ""} {
		$self trace-queue $n1 $n2 $trace
		$self trace-queue $n2 $n1 $trace
	}
	set trace [$self get-nam-traceall]
	if {$trace != ""} {
		$self namtrace-queue $n1 $n2 $trace
		$self namtrace-queue $n2 $n1 $trace
	}

	# nam only has duplex link. We do a registration here because 
	# automatic layout doesn't require calling Link::orient.
	$self instvar link_
	$self register-nam-linkconfig $link_([$n1 id]:[$n2 id])
}

Simulator instproc getlink { id1 id2 } {
	$self instvar link_
	if [info exists link_($id1:$id2)] {
		return $link_($id1:$id2)
	}
	return -1
}

Simulator instproc multi-link { nodes bw delay type } {
	$self instvar link_ 

	# set multiLink [new PhysicalMultiLink $nodes $bw $delay $type]
	set multiLink [new NonReflectingMultiLink $nodes $bw $delay $type]
	
	# set up dummy links for unicast routing
	foreach n $nodes {
	set q [$multiLink getQueue $n]
		set l [$multiLink getDelay $n]
		set did [$n id]
		foreach n2 $nodes {
	    if { [$n2 id] != $did } {
		    set sid [$n2 id]
		    set dumlink [new DummyLink $n2 $n $q $l]
		    set link_($sid:$did) $dumlink
		    $dumlink setContainingObject $multiLink
		    # set lan trace
		    set trace [$self get-ns-traceall]
		    if {$trace != "" } {
			    $self trace-queue $n2 $n $trace
		    }
		    set trace [$self get-nam-traceall]
		    if {$trace != ""} {
			    $self namtrace-queue $n2 $n $trace
		    }
		    $self register-nam-linkconfig $dumlink
	    }
		}
	}
	
	return $multiLink
}

#Simulator instproc multi-link-of-interfaces { nodes bw delay type } {
#        $self instvar link_ 
#        
#        # create the interfaces
#        set ifs ""
#        foreach n $nodes {
#                set f [new DuplexNetInterface]
#                $n addInterface $f
#                lappend ifs $f
#        }
#
#	# create lan
#        set multiLink [new NonReflectingMultiLink $ifs $bw $delay $type]
#
#        # set up dummy links for multicast routing
#        foreach f $ifs {
#                set n [$f getNode]
#                set q [$multiLink getQueue $n]
#                set l [$multiLink getDelay $n]  
#                set did [$n id]
#                foreach f2 $ifs {
#                        set n2 [$f2 getNode]
#                        if { [$n2 id] != $did } {
#				set sid [$n2 id]   
#				set dumlink [new DummyLink $f2 $f $q $l $multiLink]
#				set link_($sid:$did) $dumlink
#				# set lan trace
#				set trace [$self get-ns-traceall]
#				if {$trace != "" } {
#					$self trace-queue $n2 $n $trace
#				}
#				set trace [$self get-nam-traceall]
#				if {$trace != ""} {
#					$self namtrace-queue $n2 $n $trace
#				}
#				$self register-nam-linkconfig $dumlink
#                        }
#                }
#        }
#
#        return $multiLink
#}

# XXX Hack to use nam lan traces. Does not work with trace-all
Simulator instproc multi-link-of-interfaces { nodes bw delay type } {
	$self instvar link_ 
	
	# create the interfaces
	set ifs ""
	foreach n $nodes {
		set f [new DuplexNetInterface]
		$n addInterface $f
		lappend ifs $f
	}
	
	# create lan
	set multiLink [new NonReflectingMultiLink $ifs $bw $delay $type]
	
	# set up dummy links for multicast routing
	foreach f $ifs {
		set n [$f getNode]
		set q [$multiLink getQueue $n]
		set l [$multiLink getDelay $n]  
		set did [$n id]
		foreach f2 $ifs {
			set n2 [$f2 getNode]
			if { [$n2 id] != $did } {
				set sid [$n2 id]   
				set dumlink [new DummyLink $f2 $f $q $l $multiLink]
				set link_($sid:$did) $dumlink
			}
		}
	}
	
	# set lan trace
	set trace [$self get-ns-traceall]
	if {$trace != "" } {
		$multiLink trace $self $trace
	}
	set trace [$self get-nam-traceall]
	if {$trace != ""} {
		$multiLink nam-trace $self $trace
	}
	$self register-nam-lanconfig $multiLink
	
	return $multiLink
}

Simulator instproc makeflowmon { cltype { clslots 29 } } {
	
	set flowmon [new QueueMonitor/ED/Flowmon]
	set cl [new Classifier/Hash/$cltype $clslots]
	
	$cl proc unknown-flow { src dst fid hashbucket }  {
		set fdesc [new QueueMonitor/ED/Flow]
		set dsamp [new Samples]
		$fdesc set-delay-samples $dsamp
		set slot [$self installNext $fdesc] 
		$self set-hash $hashbucket $src $dst $fid $slot
	}
	
	$cl proc no-slot slotnum {
		#
		# note: we can wind up here when a packet passes
		# through either an Out or a Drop Snoop Queue for
		# a queue that the flow doesn't belong to anymore.
		# Since there is no longer hash state in the
		# hash classifier, we get a -1 return value for the
		# hash classifier's classify() function, and there
		# is no node at slot_[-1].  What to do about this?
		# Well, we are talking about flows that have already
		# been moved and so should rightly have their stats
		# zero'd anyhow, so for now just ignore this case..
		# puts "classifier $self, no-slot for slotnum $slotnum"
	}
	$flowmon classifier $cl
	return $flowmon
}

# attach a flow monitor to a link
# 3rd argument dictates whether early drop support is to be used

Simulator instproc attach-fmon {lnk fm { edrop 0 } } {
	set isnoop [new SnoopQueue/In]
	set osnoop [new SnoopQueue/Out]
	set dsnoop [new SnoopQueue/Drop]
	$lnk attach-monitors $isnoop $osnoop $dsnoop $fm
	if { $edrop != 0 } {
		set edsnoop [new SnoopQueue/EDrop]
		$edsnoop set-monitor $fm
		[$lnk queue] early-drop-target $edsnoop
		$edsnoop target [$self set nullAgent_]
	}
	[$lnk queue] drop-target $dsnoop
}

# Imported from session.tcl. It is deleted there.
### to insert loss module to regular links in detailed Simulator
Simulator instproc lossmodel {lossobj from to} {
	set link [$self link $from $to]
	set head [$link head]
	# puts "[[$head target] info class]"
	$lossobj target [$head target]
	$head target $lossobj
	# puts "[[$head target] info class]"
}

Simulator instproc bw_parse { bspec } {
        if { [scan $bspec "%f%s" b unit] == 1 } {
                set unit b
        }
	# xxx: all units should support X"ps" --johnh
        switch $unit {
        b  { return $b }
        bps  { return $b }
        kb { return [expr $b*1000] }
        Mb { return [expr $b*1000000] }
        Gb { return [expr $b*1000000000] }
        default { 
                  puts "error: bw_parse: unknown unit `$unit'" 
                  exit 1
                }
        }
}

Simulator instproc delay_parse { dspec } {
        if { [scan $dspec "%f%s" b unit] == 1 } {
                set unit s
        }
        switch $unit {
        s  { return $b }
        ms { return [expr $b*0.001] }
        ns { return [expr $b*0.000001] }
        default { 
                  puts "error: bw_parse: unknown unit `$unit'" 
                  exit 1
                }
        }
}


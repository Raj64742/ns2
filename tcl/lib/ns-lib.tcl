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

# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-lib.tcl,v 1.155 1999/06/29 00:01:57 salehi Exp $

#

#
# Word of warning to developers:
# this code (and all it sources) is compiled into the
# ns executable.  You need to rebuild ns or explicitly
# source this code to see changes take effect.
#


proc warn {msg} {
	global warned_
	if {![info exists warned_($msg)]} {
		puts stderr "warning: $msg"
		set warned_($msg) 1
	}
}

if {[info commands debug] == ""} {
	proc debug args {
		warn {Script debugging disabled.  Reconfigure with --with-tcldebug, and recompile.}
	}
}

proc assert args {
        if [catch "expr $args" ret] {
                set ret [eval $args]
        }
        if {! $ret} {
                error "assertion failed: $args"
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

source ns-autoconf.tcl
source ns-address.tcl
source ns-node.tcl
source ns-hiernode.tcl
source ns-mobilenode.tcl
source ns-bsnode.tcl
source ns-link.tcl
source ns-source.tcl
source ns-compat.tcl
source ns-nam.tcl
source ns-packet.tcl
source ns-queue.tcl
source ns-trace.tcl
source ns-random.tcl
source ns-agent.tcl
source ns-route.tcl
source ns-errmodel.tcl
source ns-intserv.tcl
source ns-cmutrace.tcl
source ns-mip.tcl
source ns-sat.tcl
#source ns-wireless-mip.tcl
source ../rtp/session-rtp.tcl
source ../interface/ns-iface.tcl
source ../lan/ns-mac.tcl
source ../lan/ns-ll.tcl
source ../lan/vlan.tcl
source ../mcast/timer.tcl
source ../mcast/ns-mcast.tcl
source ../mcast/McastProto.tcl
source ../mcast/DM.tcl
source ../ctr-mcast/CtrMcast.tcl
source ../ctr-mcast/CtrMcastComp.tcl
source ../ctr-mcast/CtrRPComp.tcl
source ../mcast/BST.tcl
source ../mcast/srm.tcl
source ../mcast/srm-ssm.tcl
source ../mcast/mftp_snd.tcl
source ../mcast/mftp_rcv.tcl
source ../mcast/mftp_rcv_stat.tcl
source ../mcast/McastMonitor.tcl
source ../rlm/rlm.tcl
source ../rlm/rlm-ns.tcl
source ../session/session.tcl
source ../webcache/http-server.tcl
source ../webcache/http-cache.tcl
source ../webcache/http-agent.tcl
source ../webcache/http-mcache.tcl
source ns-namsupp.tcl
source ../mobility/dsdv.tcl
source ../mobility/dsr.tcl
source ../mobility/com.tcl

source ns-default.tcl
source ../emulate/ns-emulate.tcl

Simulator instproc init args {
	$self create_packetformat

	#the calendar scheduler doesn't work on big mobile network runs
	#it dies around 240 secs...
	#$self use-scheduler List

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
        if { [Simulator info vars EnableMcast_] != "" } {
                warn "Flag variable Simulator::EnableMcast_ discontinued.\n\t\
                      Use multicast methods as:\n\t\t\
                        % set ns \[new Simulator -multicast on]\n\t\t\
                        % \$ns multicast"
                $self multicast
                Simulator unset EnableMcast_
        }
        if { [Simulator info vars NumberInterfaces_] != "" } {
                warn "Flag variable Simulator::NumberInterfaces_ discontinued.\n\t\
                      Setting (or not) this variable will not affect the simulations."
                Simulator unset NumberInterfaces_
        }
	set node [new [Simulator set node_factory_] $args]
	set Node_([$node id]) $node
	$node set ns_ $self
	if [$self multicast?] {
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

Simulator instproc after {ival args} {
        eval $self at [expr [$self now] + $ival] $args
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

	# Do all nam-related initialization here
	$self init-nam

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

Simulator instproc simplex-link { n1 n2 bw delay qtype args } {
	$self instvar link_ queueMap_ nullAgent_
	set sid [$n1 id]
	set did [$n2 id]
	
	if [info exists queueMap_($qtype)] {
		set qtype $queueMap_($qtype)
	}
	# construct the queue
	set qtypeOrig $qtype
	switch -exact $qtype {
		ErrorModule {
			if { [llength $args] > 0 } {
				set q [eval new $qtype $args]
			} else {
				set q [new $qtype Fid]
			}
		}
		intserv {
			set qtype [lindex $args 0]
			set q [new Queue/$qtype]
		}
		default {
			set q [new Queue/$qtype]
		}
	}

	# Now create the link
	switch -exact $qtypeOrig {
		RTM {
                        set c [lindex $args 1]
                        set link_($sid:$did) [new CBQLink       \
                                        $n1 $n2 $bw $delay $q $c]
                }
                CBQ -
                CBQ/WRR {
                        # assume we have a string of form "linktype linkarg"
                        if {[llength $args] == 0} {
                                # default classifier for cbq is just Fid type
                                set c [new Classifier/Hash/Fid 33]
                        } else {
                                set c [lindex $args 1]
                        }
                        set link_($sid:$did) [new CBQLink       \
                                        $n1 $n2 $bw $delay $q $c]
                }
		FQ      {
			set link_($sid:$did) [new FQLink $n1 $n2 $bw $delay $q]
		}
                intserv {
                        #XX need to clean this up
                        set link_($sid:$did) [new IntServLink   \
                                        $n1 $n2 $bw $delay $q	\
						[concat $qtypeOrig $args]]
                }
                default {
                        set link_($sid:$did) [new SimpleLink    \
                                        $n1 $n2 $bw $delay $q]
                }
        }
	$n1 add-neighbor $n2
	
	#XXX yuck
	if {[string first "RED" $qtype] != -1} {
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
	
	# Register this simplex link in nam link list. Treat it as 
	# a duplex link in nam
	$self register-nam-linkconfig $link_($sid:$did)
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
	set i1 [[$link src] id]
	set i2 [[$link dst] id]
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

Simulator instproc duplex-link { n1 n2 bw delay type args } {
	$self instvar link_
	set i1 [$n1 id]
	set i2 [$n2 id]
	if [info exists link_($i1:$i2)] {
		$self remove-nam-linkconfig $i1 $i2
	}

	eval $self simplex-link $n1 $n2 $bw $delay $type $args
	eval $self simplex-link $n2 $n1 $bw $delay $type $args
}

Simulator instproc duplex-intserv-link { n1 n2 bw pd sched signal adc args } {
	eval $self duplex-link $n1 $n2 $bw $pd intserv $sched $signal $adc $args
}

Simulator instproc simplex-link-op { n1 n2 op args } {
	$self instvar link_
	eval $link_([$n1 id]:[$n2 id]) $op $args
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
	if {$file != ""} {
		set namtraceAllFile_ $file
	} else {
		unset namtraceAllFile_
	}
}

Simulator instproc namtrace-all-wireless {file optx opty} {
        $self instvar namtraceAllFile_


        if {$file != ""} {
                set namtraceAllFile_ $file

        } else {
                unset namtraceAllFile_

        }
        $self puts-nam-config "W -t * -x $optx -y $opty"
}

Simulator instproc initial_node_pos {nodep size} {
        $self puts-nam-config "n -t * -s [$nodep id] \
        -x [$nodep set X_] -y [$nodep set Y_] -Z [$nodep set Z_] -z $size \
        -v circle -c black"
}

Simulator instproc namtrace-some file {
	$self instvar namtraceSomeFile_
	set namtraceSomeFile_ $file
}

Simulator instproc namtrace-all-wireless {file optx opty} {
        $self instvar namtraceAllFile_  
 
        if {$file != ""} { 
                set namtraceAllFile_ $file
        } else {
                unset namtraceAllFile_
        }       
        $self puts-nam-config "W -t * -x $optx -y $opty"
}
        
Simulator instproc initial_node_pos {nodep size} {
        $self puts-nam-config "n -t * -s [$nodep id] \
-x [$nodep set X_] -y [$nodep set Y_] -Z [$nodep set Z_] -z $size \
-v circle -c black"
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

        # Polly Huang: to support abstract TCP simulations
        if {[lindex [split [$src info class] "/"] 1] == "AbsTCP"} {
	    $self at [$self now] "$self rtt $src $dst"
	    $dst set class_ [$src set class_]
        }

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
        $self instvar Node_ link_
        if { ![catch "$n1 info class Node"] } {
		set n1 [$n1 id]
	}
        if { ![catch "$n2 info class Node"] } {
		set n2 [$n2 id]
	}
	if [info exists link_($n1:$n2)] {
		return $link_($n1:$n2)
	}
	return ""
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

# Creates connection. First creates a source agent of type s_type and binds
# it to source.  Next creates a destination agent of type d_type and binds
# it to dest.  Finally creates bindings for the source and destination agents,
# connects them, and  returns a list of source agent and destination agent.
Simulator instproc create-connection-list {s_type source d_type dest pktClass} {
    set s_agent [new Agent/$s_type]
    set d_agent [new Agent/$d_type]
    $s_agent set fid_ $pktClass
    $d_agent set fid_ $pktClass
    $self attach-agent $source $s_agent
    $self attach-agent $dest $d_agent
    $self connect $s_agent $d_agent

    return [list $s_agent $d_agent]
}   

# This seems to be an obsolete procedure.
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
	$link errormodule $lossobj
}

# This function generates losses that can be visualized by nam.
Simulator instproc link-lossmodel {lossobj from to} {
	set link [$self link $from $to]
	$link insert-linkloss $lossobj
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

#### Polly Huang: Simulator class instproc to support abstract tcp simulations

Simulator instproc rtt { src dst } {
    $self instvar routingTable_ delay_
    set srcid [[$src set node_] id]
    set dstid [[$dst set node_] id]
    set delay 0
    set tmpid $srcid
    while {$tmpid != $dstid} {
        set nextid [$routingTable_ lookup $tmpid $dstid]
        set tmpnode [$self get-node-by-id $tmpid]
        set nextnode [$self get-node-by-id $nextid]
        set tmplink [[$self link $tmpnode $nextnode] link]
        set delay [expr $delay + [expr 2 * [$tmplink set delay_]]]
        set delay [expr $delay + [expr 8320 / [$tmplink set bandwidth_]]]
        set tmpid $nextid
    }
    $src rtt $delay
    return $delay
}

Simulator instproc abstract-tcp {} {
    $self instvar TahoeAckfsm_ RenoAckfsm_ TahoeDelAckfsm_ RenoDelAckfsm_ dropper_ 
    $self set TahoeAckfsm_ [new FSM/TahoeAck]
    $self set RenoAckfsm_ [new FSM/RenoAck]
    $self set TahoeDelAckfsm_ [new FSM/TahoeDelAck]
    $self set RenoDelAckfsm_ [new FSM/RenoDelAck]
    $self set nullAgent_ [new DropTargetAgent]
}





 #
 # tcl/pim/pim-init.tcl
 #
 # Copyright (C) 1997 by USC/ISI
 # All rights reserved.                                            
 #                                                                
 # Redistribution and use in source and binary forms are permitted
 # provided that the above copyright notice and this paragraph are
 # duplicated in all such forms and that any documentation, advertising
 # materials, and other materials related to such distribution and use
 # acknowledge that the software was developed by the University of
 # Southern California, Information Sciences Institute.  The name of the
 # University may not be used to endorse or promote products derived from
 # this software without specific prior written permission.
 # 
 # THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 # WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 # MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 # 
 # Contributed by Ahmed Helmy, Ported by Polly Huang (USC/ISI), 
 # http://www-scf.usc.edu/~bhuang
 # 
 #
Class PIM -superclass McastProtocol

# defining the flag values ...etc
PIM set SPT 0x1
PIM set WC 0x2
PIM set RP 0x4
PIM set CREATE 0x8
PIM set IIF_REG 0x20
PIM set REG 0x80
PIM set CACHE 0x200
PIM set ASSERTED 0x1000
PIM set SG 0x2000

PIM set ALL_PIM_ROUTERS 0xFFCD
PIM set IPPROTO_PIM 103
PIM set HELLO [expr [PIM set IPPROTO_PIM] + 0]
PIM set REGISTER [expr [PIM set IPPROTO_PIM] + 1]
PIM set REGISTER_STOP [expr [PIM set IPPROTO_PIM] + 2]
# XXX PIM set JOIN_PRUNE [expr [PIM set IPPROTO_PIM] + 3]
PIM set JOIN [expr [PIM set IPPROTO_PIM] + 3]
PIM set PRUNE [expr [PIM set IPPROTO_PIM] + 9]
PIM set BOOTSTRAP [expr [PIM set IPPROTO_PIM] + 4]
PIM set ASSERT [expr [PIM set IPPROTO_PIM] + 5]
PIM set GRAFT [expr [PIM set IPPROTO_PIM] + 6]
PIM set GRAFT_ACK [expr [PIM set IPPROTO_PIM] + 7]
PIM set C_RP_ADV [expr [PIM set IPPROTO_PIM] + 8]

PIM instproc init { sim node confArgs } {
	$self next
	$self instvar ns Node type
	set ns $sim
	set Node $node
	set type "PIM"
	$self initialize
	$self configure $confArgs
}

PIM proc config { sim } {
	set collectionList [PIM collectRPSet $sim]
	set rpset [lindex $collectionList 0]
	set pimProtos [lindex $collectionList 1]
	set pimNodes [lindex $collectionList 2]
	PIM injectRPSet $rpset $pimProtos $pimNodes
}

PIM instproc Register-off { } {
        $self instvar RegisterOff
        set RegisterOff 1
}

PIM proc collectRPSet { sim } {
# compute the RPSet globally and inject in each node XXX
	set rpset ""
	set pimProtos ""
	set pimNodes ""
	foreach id [$sim getNodeIDs] {
		set arbiter [[$sim set Node_($id)] getArbiter]
		if { [set pim [$arbiter getType "PIM"]] == -1 } {
			continue
		}
		# XXX while you're at it, get the pim capable nodes
		# to facilitate the pimNbr discovery later
		lappend pimNodes [$sim set Node_($id)]
		lappend pimProtos $pim
		$sim setPIMProto $id $pim
		if [$pim getCRP] {
			# puts "id $id is CRP"
			lappend rpset $id
		}
	}
	return [list $rpset $pimProtos $pimNodes]
}

PIM proc injectRPSet { rpset pimProtos pimNodes } {
	# puts "rpset $rpset"
	foreach proto $pimProtos {
		$proto setRPSet $rpset
		# XXX while you're at it... get pimNbrs
		$proto getNbrs $pimNodes
	}
}

PIM instproc getNbrs { pimNodes } {
	$self instvar pimNbrs Node
	set pimNbrs ""
	foreach node [$Node set neighbor_] {
		if { [lsearch -exact $pimNodes $node] != -1 } {
			lappend pimNbrs $node
		}
	}
	# puts "$self node [$Node id] pimNbrs $pimNbrs"
}

PIM instproc setRPSet { rpset } {
	$self instvar RPSet
	set RPSet $rpset
}

PIM instproc getCRP {} {
	$self instvar CRP
	return $CRP
}

PIM instproc configure confArgs {
	if { ! [set len [llength $confArgs]] } { return 0 }
	$self instvar CRP CBSR priority
	set CRP [lindex $confArgs 0]
	if { $len == 1 } { return 1 }
	set CBSR [lindex $confArgs 1]
	if { $len == 2 } { 
		set priority 0
		return 1
	}
	set priority [lindex $confArgs 2]
}

PIM instproc initialize { } {
	$self instvar Node messager CRP CBSR
	set messager [new Agent/Message/pim]
	[$Node getArbiter] addproto $self
	$Node attach $messager
	$messager set proto $self
	$messager set dst_ [PIM set ALL_PIM_ROUTERS]
	set CRP 0
	set CBSR 0
}

PIM instproc start {} {
	$self next
	$self instvar Node messager
        # puts "$self starting pim, joining allpim group [PIM set ALL_PIM_ROUTERS]"
	$Node join-group $messager [PIM set ALL_PIM_ROUTERS]
}

PIM instproc stop {} {
        $self instvar MRTArray groupArray cacheByGroup sourceArray
        if [info exists MRTArray] {   
                unset MRTArray   
        }
        if [info exists groupArray] {
                unset groupArray
        }
        if [info exists cacheByGroup] {
                unset cacheByGroup
        }
        if [info exists sourceArray] {
                unset sourceArray
        }
}       

PIM proc trace { fp } {

        # reparent the messagers 
        Agent/Message/pim superclass Agent/Message/trace
        
        foreach agent [Agent/Message/pim info instances] {
                $agent trace $fp
                $agent set-TxPrefix "PIMS"
                $agent set-RxPrefix "PIMR"
        }
        
        # not sure I want to trace registering now !
        # Agent/Message/reg superclass Agent/Message/trace
}       

### XXXX forcing assert w/ entry creation XXX ###
PIM instproc force-nextHop { group nxtHop } {
        $self instvar MRTArray ns
        $self findWC $group 1
        $MRTArray($group) setNextHop $nxtHop
        $MRTArray($group) setiif [$self get-iif-label $nxtHop]
}

###################################
### Message Agent stuff 
###################################

Class Agent/Message/trace -superclass Agent/Message

Agent/Message/trace instproc trace { file } {
        $self instvar tracer traceFile RxPrefix TxPrefix
        set tracer 1
        set traceFile $file
        set RxPrefix "@"
        set TxPrefix "-"
}

Agent/Message/trace instproc set-TxPrefix { str } {
        $self instvar TxPrefix
        set TxPrefix $str
}

Agent/Message/trace instproc set-RxPrefix { str } {
        $self instvar RxPrefix
        set RxPrefix $str
}

Agent/Message/trace instproc handle msg {
        $self instvar node_ tracer traceFile RxPrefix
        set rxvString "$RxPrefix Node [$node_ id] agent $self rxvd msg \
                $msg. @ time [[$node_ set ns_] now]"
        if [info exists tracer] {
                puts $traceFile $rxvString
        } else {
                puts $rxvString
        }
        $self next $msg  
}

Agent/Message/trace instproc transmit msg {
        $self instvar node_ tracer traceFile TxPrefix
        set sendString "$TxPrefix Node [$node_ id] agent $self Sending msg \
                $msg - time [[$node_ set ns_] now]"
        if [info exists tracer] {
                puts $traceFile $sendString
        } else {
                puts $sendString
        }
        $self send $msg
}

Agent/Message/trace instproc join-group group {
	$self instvar node_ tracer traceFile
	set string "J Node [$node_ id] agent $self join group $group \
		time [[$node_ set ns_] now]"
        if [info exists tracer] {
                puts $traceFile $string  
        } else {
                puts $string
        }
	$node_ join-group $self $group
}

Agent/Message/trace instproc leave-group group {
        $self instvar node_ tracer traceFile
        set string "L Node [$node_ id] agent $self leave group $group \
		time [[$node_ set ns_] now]"
        if [info exists tracer] {
                puts $traceFile $string
        } else {
                puts $string
        }
        $node_ leave-group $self $group
}

Agent/Message instproc transmit msg {
	$self send $msg
}

Agent/Message instproc handle msg {
}

Agent/Message instproc join-group group {
	$self instvar node_
	$node_ join-group $self $group
}

Agent/Message instproc leave-group group {
	$self instvar node_
	$node_ leave-group $self $group
}

####### to change the trace file during simulation
#####################

Simulator instproc change-all-traces { file } {
        $self instvar alltrace_ traceAllFile_
        #XXX clean up, flush and close old file then
        # assign new file
        $self flush-trace
        close $traceAllFile_
        $self trace-all $file
        foreach trace $alltrace_ {
                $trace attach $file
        }
}


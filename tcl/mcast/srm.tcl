# 
#  Copyright (c) 1997 by the University of Southern California
#  All rights reserved.
# 
#  Permission to use, copy, modify, and distribute this software and its
#  documentation in source and binary forms for non-commercial purposes
#  and without fee is hereby granted, provided that the above copyright
#  notice appear in all copies and that both the copyright notice and
#  this permission notice appear in supporting documentation. and that
#  any documentation, advertising materials, and other materials related
#  to such distribution and use acknowledge that the software was
#  developed by the University of Southern California, Information
#  Sciences Institute.  The name of the University may not be used to
#  endorse or promote products derived from this software without
#  specific prior written permission.
# 
#  THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
#  the suitability of this software for any purpose.  THIS SOFTWARE IS
#  PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
#  INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 
#  Other copyrights might apply to parts of this software and are so
#  noted when applicable.
# 


#
# Source srm-debug.tcl if you want to turn on tracing of events.
# Do not insert probes directly in this code unless you are sure
# that what you want is not available there.
#

Agent instproc traffic-source agent {
    $self instvar tg_
    set tg_ $agent
    $tg_ target $self
    $tg_ set addr_ [$self set addr_]
}

Agent/SRM set packetSize_  1024	;# assume default message size for repair is 1K

Agent/SRM set C1_	2.0
Agent/SRM set C2_	2.0
Agent/SRM set requestFunction_	"SRM/request"
Agent/SRM set requestBackoffLimit_	5

Agent/SRM set D1_	1.0
Agent/SRM set D2_	1.0
Agent/SRM set repairFunction_	"SRM/repair"

Agent/SRM set sessionDelay_ 1.0
Agent/SRM set sessionFunction_	"SRM/session"

Class Agent/SRM/Deterministic -superclass Agent/SRM
Agent/SRM/Deterministic set C2_ 0.0
Agent/SRM/Deterministic set D2_ 0.0

Class Agent/SRM/Probabilistic -superclass Agent/SRM
Agent/SRM/Probabilistic set C1_ 0.0
Agent/SRM/Probabilistic set D1_ 0.0

Agent/SRM instproc init {} {
    $self next
    $self instvar ns_ requestFunction_ repairFunction_
    set ns_ [Simulator instance]
    $self init-instvar sessionDelay_
    foreach var {C1_ C2_ D1_ D2_} {
	$self init-instvar $var
    }
    $self init-instvar requestFunction_
    $self init-instvar repairFunction_
    $self init-instvar sessionFunction_
    $self init-instvar requestBackoffLimit_
}

Agent/SRM instproc delete {} {
    $self instvar ns_ pending_ done_ session_ tg_
    foreach i [array names pending_] {
	$pending_($i) cancel
	delete $pending_($i)
    }
    $self cleanup
    delete $session_
    if [info exists tg_] {
	delete $tg_
    }
}

#
Agent/SRM instproc start {} {
    $self instvar node_ dst_		;# defined in Agent base class
    set dst_ [expr $dst_]		;# get rid of possibly leading 0x etc.
    $self cmd start

    $node_ join-group $self $dst_

    $self instvar ns_ session_ sessionFunction_
    set session_ [new $sessionFunction_ $ns_ $self]
    $session_ schedule
}

Agent/SRM instproc start-source {} {
    $self instvar tg_
    if ![info exists tg_] {
	error "No source defined for agent $self"
    }
    $tg_ start
}

Agent/SRM instproc sessionFunction f {
    $self instvar sessionFunction_
    set sessionFunction_ $f
}

Agent/SRM instproc requestFunction f {
    $self instvar requestFunction_
    set requestFunction_ $f
}

Agent/SRM instproc repairFunction f {
    $self instvar repairFunction_
    set repairFunction_ $f
}

#
Agent/SRM instproc recv {type args} {
    eval $self evTrace $proc $type $args
    eval $self recv-$type $args
}

Agent/SRM instproc recv-data {sender msgid} {
    # received an old message as data.  It really ought to have been a repair.
    $self recv-repair $sender $msgid
}

Agent/SRM instproc request {sender msgid} {
    # called when agent detects a lost packet
    $self instvar pending_ ns_ requestFunction_
    if [info exists pending_($sender:$msgid)] {
	error "duplicate loss detection in agent"
    }
    set pending_($sender:$msgid) [new $requestFunction_ $ns_ $self]
    $pending_($sender:$msgid) set-params $sender $msgid
    $pending_($sender:$msgid) schedule
}

Agent/SRM instproc recv-request {requestor sender msgid} {
    # called when agent receives a control message for an old ADU
    $self instvar pending_
    if [info exists pending_($sender:$msgid)] {
	# Either a request or a repair is pending
	$pending_($sender:$msgid) recv-request
    } else {
	# We have no events pending, only if the ADU is old, and
	# we have received that ADU.
	$self repair $requestor $sender $msgid
    }
}

Agent/SRM instproc repair {requestor sender msgid} {
    # called whenever agent receives a request, and packet exists
    $self instvar pending_ ns_ repairFunction_
    if [info exists pending_($sender:$msgid)] {
	error "duplicate repair detection in agent??  really??"
    }
    set pending_($sender:$msgid) [new $repairFunction_ $ns_ $self]
    $pending_($sender:$msgid) set requestor_ $requestor
    $pending_($sender:$msgid) set-params $sender $msgid
    $pending_($sender:$msgid) schedule
}

Agent/SRM instproc recv-repair {sender msgid} {
    $self instvar pending_
    if [info exists pending_($sender:$msgid)] {
	$pending_($sender:$msgid) recv-repair
    } else {
	# 1.  We didn't hear the request for the older ADU, or
	# 2.  This is a very late repair beyond the $3 d_{S,B}$ wait
	# What should we do?
	error "Oy vey!  How did we get here?"
    }
}

#
Agent/SRM instproc clear {obj s m} {
    $self instvar pending_ done_
    set done_($s:$m) $obj
    $self dump-stats $obj $s $m
    unset pending_($s:$m)
    if {[array size done_] > 32} {
	$self instvar ns_
	$ns_ at [expr [$ns_ now] + 0.01] "$self cleanup"
    }
}

Agent/SRM instproc cleanup {} {
    # We need to do this somewhere...now seems a good time
    $self instvar done_
    if [info exists done_] {
	foreach i [array names done_] {
	    delete $done_($i)
	}
	unset done_
    }
}

Agent/SRM instproc trace file {
    $self instvar trace_
    set trace_ $file
}

Agent/SRM instproc dump-stats {obj s m} {
    $self instvar trace_ ns_ node_
    if [info exists trace_] {
	puts $trace_ [concat [format "%7.4f" [$ns_ now]] node [$node_ id]  \
		[string range [$obj info class] 4 end] "<$s:$m>"	   \
		"serviceTime" [$obj serviceTime?] [$obj array get statistics_]]
    }
}

Agent/SRM instproc evTrace {op type args} {
    # no-op
}

#
#
# Note that the SRM event handlers are not rooted as TclObjects.
#
Class SRM
SRM instproc init {ns agent} {
    $self next
    $self instvar ns_ agent_
    set ns_ $ns
    set agent_ $agent
    $self init-statistics
}
    
SRM instproc set-params {sender msgid} {
    $self next
    $self instvar sender_ msgid_
    set sender_ $sender
    set msgid_  $msgid
}

SRM instproc cancel {} {
    $self instvar ns_ eventID_
    if [info exists eventID_] {
	$ns_ cancel $eventID_
	$self evTrace $proc
	unset eventID_
    }
}

SRM instproc init-statistics {} {
    $self instvar ns_ startTime_
    set startTime_ [$ns_ now]
}

SRM instproc serviceTime {} {
    $self instvar ns_ startTime_ serviceTime_
    set serviceTime_ [expr [$ns_ now] - $startTime_]
}

SRM instproc serviceTime? {} {
    $self instvar serviceTime_
    if ![info exists serviceTime_] {
	error "object $self was still in service"
    }
    set serviceTime_
}

SRM instproc evTrace args {
    # no-op
}

#
Class SRM/request -superclass SRM
SRM/request instproc init args {
    eval $self next $args
}

SRM/request instproc set-params args {
    eval $self next $args
    $self instvar agent_
    foreach var {C1_ C2_} {
	if ![catch "$agent_ set $var" val] {
	    $self instvar $var
	    set $var $val
	}
    }
    $self instvar backoff_ backoffCtr_ backoffLimit_
    set backoff_ 1
    set backoffCtr_ 0
    set backoffLimit_ [$agent_ set requestBackoffLimit_]
}

SRM/request instproc backoff? {} {
    $self instvar backoff_ backoffCtr_ backoffLimit_
    set retval $backoff_
    if {[incr backoffCtr_] <= $backoffLimit_} {
	incr backoff_ $backoff_
    }
    set retval
}

SRM/request instproc compute-delay {} {
    $self instvar C1_ C2_
    set rancomp [expr $C1_ + $C2_ * [uniform 0 1]]

    $self instvar agent_ sender_
    set delay [expr $rancomp * [$agent_ distance? $sender_]]
}

SRM/request instproc schedule {} {
    $self instvar ns_ eventID_ delay_
    set now [$ns_ now]
    set delay_ [expr [$self compute-delay] * [$self backoff?]]
    set fireTime [expr $now + $delay_]

    $self evTrace request $fireTime

    set eventID_ [$ns_ at $fireTime "$self send-request"]
}

SRM/request instproc send-request {} {
    $self instvar agent_ sender_ msgid_
    $agent_ send request $sender_ $msgid_

    $self instvar statistics_
    incr statistics_(#sent)
}

SRM/request instproc recv-request {} {
    $self instvar ns_ delay_ ignore_ statistics_
    if {[info exists ignore_] && [$ns_ now] < $ignore_} {
	incr statistics_(dupRQST)
    } else {
	$self cancel
	$self schedule          ;# or rather, reschedule-rqst 
	set ignore_ [expr [$ns_ now] + ($delay_ / 2)]
	incr statistics_(backoff)
    }
    $self evTrace recv-request $ignore_
}

SRM/request instproc recv-repair {} {
    $self instvar ns_ agent_ sender_ msgid_ ignore_ eventID_
    if [info exists eventID_] {
	$self serviceTime
	set ignore_ [expr [$ns_ now] + 3 * [$agent_ distance? $sender_]]
	$ns_ at $ignore_ "$agent_ clear $self $sender_ $msgid_"
	$self evTrace recv-repair $ignore_
	$self cancel
    } else {			;# we must be in the 3dS,B waitTime interval
	$self instvar statistics_
	incr statistics_(dupREPR)
    }
}

SRM/request instproc init-statistics {} {
    $self next
    $self array set statistics_ "dupRQST 0 dupREPR 0 #sent 0 backoff 0"
}

#
Class SRM/repair -superclass SRM
SRM/repair instproc init args {
    eval $self next $args
}

SRM/repair instproc set-params args {
    eval $self next $args
    $self instvar agent_
    foreach var {D1_ D2_} {
	if ![catch "$agent_ set $var" val] {
	    $self instvar $var
	    set $var $val
	}
    }
}

SRM/repair instproc compute-delay {} {
    $self instvar D1_ D2_
    set rancomp [expr $D1_ + $D2_ * [uniform 0 1]]

    $self instvar agent_ requestor_
    set delay [expr $rancomp * [$agent_ distance? $requestor_]]
}

SRM/repair instproc schedule {} {
    $self instvar ns_ eventID_
    set fireTime [expr [$ns_ now] + [$self compute-delay]]

    $self evTrace repair $fireTime

    set eventID_ [$ns_ at $fireTime "$self send-repair"]
}

SRM/repair instproc send-repair {} {
    $self instvar ns_ agent_ sender_ msgid_ requestor_
    $agent_ send repair $sender_ $msgid_

    $self instvar statistics_
    incr statistics_(#sent)
}

SRM/repair instproc recv-request {} {
    # duplicate (or multiple) requests
    $self instvar statistics_
    incr statistics_(dupRQST)
}

SRM/repair instproc recv-repair {} {
    $self instvar ns_ agent_ sender_ msgid_ eventID_ requestor_
    if [info exists eventID_] {
	$self serviceTime
	#
	# waitTime is identical to the ignore_ parameter in
	# SRM/request::recv-repair.  However, since recv-request for this class
	# is very simple, we do not need an instance variable to record
	# the current state, and hence only require a simple variable.
	#
	set waitTime [expr [$ns_ now] + 3 * [$agent_ distance? $requestor_]]
	$ns_ at $waitTime "$agent_ clear $self $sender_ $msgid_"
	$self evTrace recv-repair $waitTime
	$self cancel
    } else {			;# we must in the 3dS,B waitTime interval
	$self instvar statistics_
	incr statistics_(dupREPR)
    }
}

SRM/repair instproc init-statistics {} {
    $self next
    $self array set statistics_ "dupRQST 0 dupREPR 0 #sent 0"
}

#
Class SRM/session -superclass SRM
SRM/session instproc init args {
    eval $self next $args
    $self instvar agent_ sessionDelay_
    set sessionDelay_ [$agent_ set sessionDelay_]
}

SRM/session instproc delete {} {
    $self instvar $ns_ eventID_
    $ns_ cancel $eventID_
}

SRM/session instproc schedule {} {
    $self instvar ns_ sessionDelay_ eventID_

    # What is a reasonable interval to schedule session messages?
    set fireTime [expr $sessionDelay_ * [uniform 0.9 1.1]]
    # set fireTime [expr $sessionDelay_ * [uniform 0.9 1.1] * \
	    (1 + log([$self groupSize?])) ]

    $self instvar statistics_
    incr statistics_(#sent)

    set eventID_ [$ns_ at [expr [$ns_ now] + $fireTime] "$self send-session"]
}

SRM/session instproc send-session {} {
    $self instvar agent_
    $agent_ send session
    $self schedule
}

SRM/session instproc init-statistics {} {
    $self next
    $self array set statistics_ "#sent 0"
}

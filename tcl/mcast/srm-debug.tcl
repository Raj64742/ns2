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

set etrace_ stdout

proc ftime t {
    return [format "%7.4f" $t]
}

proc ft {ns obj} {
    return "[ftime [$ns now]] [[$obj set node_] id] $obj"
}

SRM instproc wlog type {
    $self instvar eventID_ sender_ msgid_
    if [info exists eventID_] {
	set e " eventID $eventID_"
    } else {
	set e ""
    }
    return "${type} <$sender_:$msgid_> $e"
}

SRM instproc evTrace {type args} {
    global etrace_
    $self instvar ns_ agent_
    if {[$class info instprocs evTrace-$type] == ""} {
	puts $etrace_ "[ft $ns_ $agent_] [$self wlog $type] $args"
    } else {
	puts $etrace_ "[ft $ns_ $agent_] [$self wlog $type] [eval $self evTrace-$type $args]"
    }
}

SRM instproc evTrace-request f		{ return  "at [ftime $f]" }
SRM instproc evTrace-repair f		{ return "at [ftime $f]" }
SRM instproc evTrace-recv-request t	{ return "until [ftime $t]" }
SRM instproc evTrace-recv-repair t	{ return "ignore-until [ftime $t]" } 

Agent/SRM instproc evTrace {op type args} {
    global etrace_
    $self instvar ns_
    if {[$class info instprocs evTrace-$op-$type] == ""} {
	puts $etrace_ "[ft $ns_ $self] $op $type $args"
    } else {
	puts $etrace_ "[ft $ns_ $self] $op $type [eval $self evTrace-$op-$type $args]"
    }
}

Agent/SRM instproc evTrace-send-repair  {s m} { return "<$s:$m>" }
Agent/SRM instproc evTrace-send-request {s m} { return "<$s:$m>" }
Agent/SRM instproc evTrace-recv-data    {s m} { return "<$s:$m>" }
Agent/SRM instproc evTrace-recv-repair  {s m} { return "<$s:$m>" }
Agent/SRM instproc evTrace-recv-request {r s m} {
    return "<$s:$m> requestor [expr $r >> 8]"	;# XXX
}
Agent/SRM instproc send {type args} {
    eval $self evTrace $proc $type $args
    eval $self cmd send $type $args
}

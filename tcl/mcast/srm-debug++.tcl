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

#requires srm-debug.tcl to also be sourced

Class SRM/request/debug -superclass SRM/request
Class SRM/repair/debug -superclass SRM/repair

Agent/SRM set requestFunction_ "SRM/request/debug"
Agent/SRM set repairFunction_  "SRM/repair/debug"

set etrace1_ stdout

SRM instproc evTrace1 {type args} {
    global etrace1_
    $self instvar ns_ agent_
    puts $etrace1_ [eval concat [ft $ns_ $agent_] [$self wlog $type] $args]
}

SRM/request/debug instproc compute-delay {} {
    $self instvar C1_ C2_ agent_ sender_ backoff_
    set unif [uniform 0 1]
    set rancomp [expr $C1_ + $C2_ * $unif]
    set dist [$agent_ distance? $sender_]
    set delay [expr $rancomp * $dist]

    $self evTrace1 request "dist($sender_) $dist U: $unif backoff $backoff_ delay $delay"
    set delay
}

SRM/repair/debug instproc compute-delay {} {
    $self instvar D1_ D2_ agent_ requestor_
    set unif [uniform 0 1]
    set rancomp [expr $D1_ + $D2_ * $unif]
    set dist [$agent_ distance? $requestor_]
    set delay [expr $rancomp * $dist]

    $self evTrace1 repair "dist($requestor_) $dist U: $unif delay $delay"
    set delay
}

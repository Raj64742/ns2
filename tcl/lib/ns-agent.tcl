#
# Copyright (c) 1996-1997 Regents of the University of California.
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-agent.tcl,v 1.11 1998/07/08 21:10:57 kfall Exp $
#

#
# OTcl methods for the Agent base class
#

Agent instproc port {} {
	$self instvar portID_
	return $portID_
}

#
# Lower 8 bits of dst_ are portID_.  this proc supports setting the interval
# for delayed acks
#       
Agent instproc dst-port {} {
	$self instvar dst_
	return [expr $dst_%256]
}

#
# Add source of type s_type to agent and return the source
#
Agent instproc attach-source {s_type} {
	set source [new Source/$s_type]
	$source attach $self
	$self set type_ $s_type
	return $source
}

#
# Attach tbf to an agent
#
Agent instproc attach-tbf { tbf } {
	$tbf target [$self target]
	$self target $tbf

}


#
# OTcl support for classes derived from Agent
#
Class Agent/Null -superclass Agent

Agent/Null instproc init args {
    eval $self next $args
}

Agent/LossMonitor instproc log-loss {} {
}

#Signalling agent attaches tbf differently as none of its signalling mesages
#go via the tbf
Agent/CBR/UDP/SA instproc attach-tbf { tbf } {
	$tbf target [$self target]
	$self target $tbf
	$self ctrl-target [$tbf target]
}


#
# A lot of agents want to store the maxttl locally.  However,
# setting a class variable based on the Agent::ttl_ variable
# does not help if the user redefines Agent::ttl_.  Therefore,
# Agents interested in the maxttl_ should call this function
# with the name of their class variable, and it is set to the
# maximum of the current/previous value.
#
# The function itself returns the value of ttl_ set.
#
# I use this function from agent constructors to set appropriate vars:
# for instance to set Agent/rtProto/DV::INFINITY, or
# Agent/SRM/SSM::ttlGroupScope_
# 
Agent proc set-maxttl {objectOrClass var} {
	if { [catch "$objectOrClass set $var" value] ||	\
	     ($value < [Agent set ttl_]) } {
		$objectOrClass set $var [Agent set ttl_]
	}
	$objectOrClass set $var
}

#
# Full Tcp constructors for other than the baseline Reno
# implementation
#

Agent/TCP/FullTcp/Tahoe instproc init {} {
	$self next
	$self instvar reno_fastrecov_
	set reno_fastrecov_ false
}

Agent/TCP/FullTcp/Sack instproc init {} {
	$self next
	$self instvar reno_fastrecov_ maxburst_
	set reno_fastrecov_ false
	set maxburst_ 5
}

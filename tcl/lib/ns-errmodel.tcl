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
#       This product includes software developed by the Daedalus Research
#       Group at the University of California Berkeley.
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
# Contributed by the Daedalus Research Group, UC Berkeley 
# (http://daedalus.cs.berkeley.edu)
#

#
# There are three levels to error generation.
# 1. Single State
#	rate_:  uniform error rate in pkt or byte
# 2. Two State
#	error-free (0) and error (1) states
#	each state has a ranvar determining the length each state
# 3. Multi-State:  extending Two-State in OTcl
#	each state has a ranvar determining the length each state
#	a matrix specifying transition probabilities
#
# Each state is an error model (which could be 1-state or multi-state),
# In addtion, the error model has a matrix of transition probabilities,
# and a start state for the model.  These usually corresond to
# homogeneous Markov chains, but are not restricted to them, because it
# is possible to change the transition probabilities on the fly and
# depending on past history, if you so desire.  Multi-state error models
# reside entirely in Tcl and aren't split between C and Tcl.  One-state
# error models are split objects and ErrorModel is derived from
# Connector.  As an example, a 2-state Markov error model is built-in,
# as ErrorModel/MultiState/TwoStateMarkov Finally, an error *module*
# contains a classifier, a set of dynamically-added ErrorModels, and
# enough plumbing to construct flow-based Errors.
#

ErrorModel/TwoState instproc init {rv0 rv1 {unit "pkt"}} {
	$self next
	$self unit $unit
	$self ranvar 0 $rv0
	$self ranvar 1 $rv1
}

Class ErrorModel/Uniform -superclass ErrorModel
Class ErrorModel/Expo -superclass ErrorModel/TwoState
Class ErrorModel/Empirical -superclass ErrorModel/TwoState

ErrorModel/Uniform instproc init {rate {unit "pkt"}} {
	$self next
	$self unit $unit
	$self set rate_ $rate
}

ErrorModel/Expo instproc init {avgList {unit "pkt"}} {
	set rv0 [new RandomVariable/Exponential]
	set rv1 [new RandomVariable/Exponential]
	$rv0 set avg_ [lindex $avgList 0]
	$rv1 set avg_ [lindex $avgList 1]
	$self next $rv0 $rv1 $unit
}

ErrorModel/Empirical instproc initrv {fileList {unit "pkt"}} {
	set rv0 [new RandomVariable/Empirical]
	set rv1 [new RandomVariable/Empirical]
	$rv0 loadCDF [lindex $fileList 0]
	$rv1 loadCDF [lindex $fileList 1]
	$self next $rv0 $rv1 $unit
}


Class ErrorModel/MultiState -superclass ErrorModel

ErrorModel/MultiState instproc init {states trans transunit nstates start} {
	# states_ is an array of states (errmodels),
	# transmatrix_ is the transition state model matrix,
	# nstates_ is the number of states
	# transunit_ is pkt/byte/time, and curstate_ is the current state
	# start is the start state, which curstate_ is initialized to

	$self instvar states_ transmatrix_ transunit_ nstates_ curstate_ eu_
	set states_ $states
	set transmatrix_ $trans
	set transunit_ $transunit
	set nstates_ $nstates
	set curstate_ $start
	set eu_ $transunit
	$self next
}

ErrorModel/MultiState instproc corrupt { } {
	$self instvar states_ transmatrix_ transunit_ curstate_

	set cur $curstate_
# XXX
	set retval [$curstate_ next]
	set curstate_ [$self transition]
	if { $cur != $curstate_ } {
		# If transitioning out, reset erstwhile current state
		$cur reset
	}
	return $retval
}

# XXX eventually want to put in expected times of staying in each state 
# before transition here.  Punt on this for now.
#ErrorModel instproc insert-error { parent } {
#	return [$self corrupt $parent]
#}

# Decide whom to transition to
ErrorModel/MultiState instproc transition { } {
	$self instvar states_ transmatrix_ transunit_ curstate_ nstates_

	for { set i 0 } { $i < $nstates_ } {incr i} {
		if { [lindex $states_ $i] == $curstate_ } {
			break
		}
	}
	# get the right transition list
	set trans [lindex $transmatrix_ $i]
	set p [uniform 0 1]
	set total 0
	for { set i 0 } { $i < $nstates_ } {incr i } {
		set total [expr $total + [lindex $trans $i]]
		if { $p <= $total } {
			return [lindex $states_ $i]
		}
	}
	puts "Misconfigured state transition: prob $p total $total $nstates_"
	return $curstate_
}

Class ErrorModel/TwoStateMarkov -superclass ErrorModel/TwoState

ErrorModel/TwoStateMarkov instproc init {rate eu {transition}} {
	$self next
	$self unit time

	set rv0 [new RandomVariable/Exponential]
	set rv1 [new RandomVariable/Exponential]
	$rv0 set avg_ [lindex $rate 0]
	$rv1 set avg_ [lindex $rate 1]
	$self ranvar 0 $rv0
	$self ranvar 1 $rv1

#	set p01 [lindex $transition 0]
#	set p10 [lindex $transition 1]
#	set trans [list [list [expr 1 - $p01] $p01] \
#			[list [expr 1 - $p01] $p01]]

#	# state 0 is the start state
#	$self next $states_ $trans $eu $i [lindex $states_ 0]
}


#
# the following is a "ErrorModule";
# it contains a classifier, a set of dynamically-added ErrorModels, and enough
# plumbing to construct flow-based Errors.
#
# It's derived from a connector
#

ErrorModule instproc init { cltype { clslots 29 } } {

	$self next
	set nullagent [[Simulator instance] set nullAgent_]

	set classifier [new Classifier/Hash/$cltype $clslots]
	$self cmd classifier $classifier
	$self cmd target [new Connector]
	$self cmd drop-target [new Connector]
	$classifier proc unknown-flow { src dst fid bucket } {
		puts "warning: classifier $self unknown flow s:$src, d:$dst, fid:$fid, bucket:$bucket"
	}
}

ErrorModule instproc insert errmodel {
	$self instvar models_
	$errmodel target [$self cmd target]
	$errmodel drop-target [$self cmd drop-target]
	if { [info exists models_] } {
		set models_ [concat $models_ $errmodel]
	} else {
		set models_ $errmodel
	}
}

ErrorModule instproc errormodels {} {
	$self instvar models_
	return $models_
}

ErrorModule instproc bind args {
        # this is to perform '$fem bind $errmod id'
        # and '$fem bind $errmod idstart idend'
    
        set nargs [llength $args]
        set errmod [lindex $args 0]
        set a [lindex $args 1]
        if { $nargs == 3 } {
                set b [lindex $args 2]
        } else {
                set b $a
        }       
        # bind the errmodel to the flow id's [a..b]
	set cls [$self cmd classifier]
        while { $a <= $b } {
                # first install the class to get its slot number
                # use the flow id as the hash bucket
                set slot [$cls installNext $errmod] 
                $cls set-hash $a 0 0 $a $slot
                incr a  
        }
}

ErrorModule instproc target args {
	if { $args == "" } {
		return [[$self cmd target] target]
	}
	set obj [lindex $args 0]

	[$self cmd target] target $obj
	[$self cmd target] drop-target $obj
}

ErrorModule instproc drop-target args {
	if { $args == "" } {
		return [[$self cmd drop-target] target]
	}

	set obj [lindex $args 0]

	[$self cmd drop-target] drop-target $obj
	[$self cmd drop-target] target $obj
}

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
# Support code for error generation based on packet-, byte- or time-based models.

# Note: Multi-state Markov models not supported yet (coming soon!).

proc newErrorU {rate unit} {
        set em [new ErrorModel]
        set rv [new RandomVariable/Uniform]
	$rv set min_ 0
	$rv set max_ [expr 2*$rate]
	$em ranvar $rv
	$em unit $unit
        $em set rate_ $rate
	return $em
}

proc newErrorE {rate unit} {
        set em [new ErrorModel]
	set rv [new RandomVariable/Exponential]
	$rv set avg_ $rate
	$em ranvar $rv
        $em set avg_ $rate
	$em unit $unit
	return $em
}	

proc newErrorM {unit} {
        set em [new ErrorModel/Markov time]
        set rvGood [new RandomVariable/Exponential]
        $rvGood set avg_ 1
        set rvBad [new RandomVariable/Exponential]
        $rvBad set avg_ 0.005
        $em ranvar $rvBad 1 0.5
        $em ranvar $rvGood 0 0.001
	$em unit $unit
        return $em
}

proc create-error {num rate errmodel unit} {
        global ns opt
        global lan node source

	# default is exponential errmodel
	if { $errmodel == "uniform"} {
		set em newErrorU
	} elseif { $errmodel == "markov" } {
		set em newErrorM
	} else {
		set em newErrorE
	}
	

        set src $node(0)
        set sifq [$lan get-ifq $src]
        set slabel [[$lan get-mac $src] set label_]
        for {set i 1} {$i <= $num} {incr i} {
                set dst $node($i)
		# Cdls is the Channel State Dependent Link Scheduler
                if ![string compare $opt(ifq) Cdls] {
                        [$lan get-ifq $dst] error $label0 [newErrorM]
                        $sifq error [[$lan get-mac $dst] set label_] \
					[newErrorM $unit]
                } else {
                        $lan install-error $src $dst [$em $rate $unit]
                        $lan install-error $dst $src [$em $rate $unit]
                }
        }
}

#
# the following is a "ErrorModule";
# it contains a classifier, a set of dynamically-added ErrModels, and enough
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
	$errmodel target [$self cmd target]
	$errmodel drop-target [$self cmd drop-target]
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

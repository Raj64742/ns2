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
#


# for now creating a new class for hier-node for the sake of minimum overlap. 
# However this may be combined with ns-node at a later date when hierarchical routing
# gets to become stable & operational.

Class HierNode -superclass Node


HierNode instproc init {addr args} {
    eval $self next $args
    $self instvar np_ id_ classifiers_ agents_ dmux_ neighbor_ address_ 
    # puts "id=$id_"
    set levels [AddrParams set hlevel_]
    for {set n 1} {$n <= $levels} {incr n} {
	set classifiers_($n) [new Classifier/Addr]
	$classifiers_($n) set mask_ [AddrParams set NodeMask_($n)]
	$classifiers_($n) set shift_ [AddrParams set NodeShift_($n)]
    }
    set address_ $addr
    # puts "node address= $address_"
}

HierNode instproc entry {} {
    if [Simulator set EnableMcast_] {
	$self instvar switch_
	return $switch_
    }
    $self instvar classifiers_
    return $classifiers_(1)
}


HierNode instproc add-hroute { dst target } {
    $self instvar classifiers_
    set al [$self split-addrstr $dst]
    set l [llength $al]
    for {set i 1} {$i <= $l} {incr i} {
	set d [lindex $al [expr $i-1]]
	if {$i == $l} {
	    $classifiers_($i) install $d $target
	} else {
	    $classifiers_($i) install $d $classifiers_([expr $i + 1]) 
	}
    }
}


HierNode instproc node-addr {} {
    $self instvar address_
    return $address_
}


#
# split up hier address string 
#
HierNode instproc split-addrstr addrstr {
    set L [split $addrstr .]
    return $L
}



# as of now, hierarchical routing support not extended for equal cost multi path routing
### feature may be added in future



#
# Copyright (c) 1996-1998 Regents of the University of California.
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
# @(#) $Header: 
#

#
#
# Special Base station nodes for communicating between wired and 
# wireless topologies in ns. Base stations are a hybrid form between 
# mobilenodes and hierarchical nodes. 
# XXXX does otcl support multiple inheritence? then maybe could 
# inheriting from hiernode and mobilenode.
# 

#
# The Node/MobileNode/BaseStationNode class
#
# ======================================================================
Class Node/MobileNode/BaseStationNode -superclass Node/MobileNode

Node/MobileNode/BaseStationNode instproc init args {
	$self next $args
}

Node/MobileNode/BaseStationNode instproc mk-default-classifier {} {
    $self instvar classifiers_ 
    
    set levels [AddrParams set hlevel_]
    for {set n 1} {$n <= $levels} {incr n} {
	set classifiers_($n) [new Classifier/Addr/Bcast]
	$classifiers_($n) set mask_ [AddrParams set NodeMask_($n)]
	$classifiers_($n) set shift_ [AddrParams set NodeShift_($n)]
    }
}


Node/MobileNode/BaseStationNode instproc entry {} {
    #XXX although mcast is not supported with wireless networking currently
    $self instvar ns_
    if ![info exist ns_] {
	set ns_ [Simulator instance]
    }
    if [$ns_ multicast?] { 
	$self instvar switch_
	return $switch_
    }
    $self instvar classifiers_
    return $classifiers_(1)
}

Node/MobileNode/BaseStationNode instproc add-hroute { dst target } {
	$self instvar classifiers_ rtsize_
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
	#
	# increase the routing table size counter - keeps track of rtg table size for 
	# each node
	set rtsize_ [expr $rtsize_ + 1]
}


Node/MobileNode/BaseStationNode instproc node-addr {} {
	$self instvar address_
	return $address_
}


Node/MobileNode/BaseStationNode instproc split-addrstr addrstr {
	set L [split $addrstr .]
	return $L
}

Node/MobileNode/BaseStationNode instproc add-target {agent port } {

    global RouterTrace AgentTrace
    $self instvar dmux_ classifiers_
    $agent set sport_ $port
    set level [AddrParams set hlevel_]

    if { $port == 255 } {	
	if { $RouterTrace == "ON" } {
	    #
	    # Send Target
	    #
	    set sndT [cmu-trace Send "RTR" $self]
	    $sndT target [$self set ll_(0)]
	    $agent target $sndT
	    
	    #
	    # Recv Target
	    #
	    set rcvT [cmu-trace Recv "RTR" $self]
	    $rcvT target $agent
	    for {set i 1} {$i <= $level} {incr i} {
		$classifiers_($i) defaulttarget $rcvT
		$classifiers_($i) bcast-receiver $rcvT
	    }
	    $dmux_ install $port $rcvT
	
	} else {
	    $agent target [$self set ll_(0)]
	    for {set i 1} {$i <= $level} {incr i} {
		$classifiers_($i) bcast-receiver $agent
		$classifiers_($i) defaulttarget $agent
	    }
	    $dmux_ install $port $agent
	    ##$classifiers_($level) defaulttarget $agent
	}
    } else {
	if { $AgentTrace == "ON" } {
	    #
	    # Send Target
	    #
	    set sndT [cmu-trace Send AGT $self]
	    $sndT target [$self entry]
	    $agent target $sndT
		
	    #
	    # Recv Target
	    #
	    set rcvT [cmu-trace Recv AGT $self]
	    $rcvT target $agent
	    $dmux_ install $port $rcvT

	} else {
	    $agent target [$self entry]
	    #for {set i 1} {$i <= $level} {incr i} {
		#$classifiers_($i) bcast-receiver $dmux_
		#}
	    $dmux_ install $port $agent
	}
    }
}


#
# Copyright (c) Sun Microsystems, Inc. 1998 All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#      This product includes software developed by Sun Microsystems, Inc.
#
# 4. The name of the Sun Microsystems, Inc nor may not be used to endorse or
#      promote products derived from this software without specific prior
#      written permission.
#
# SUN MICROSYSTEMS MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS
# SOFTWARE FOR ANY PARTICULAR PURPOSE.  The software is provided "as is"
# without express or implied warranty of any kind.
#
# These notices must be retained in any copies of any part of this software.
#

MIPEncapsulator instproc tunnel-exit mhaddr {
	$self instvar node_
	return [[$node_ set regagent_] set TunnelExit_($mhaddr)]
}

Class Node/MIPBS -superclass Node/Broadcast

Node/MIPBS instproc init { args } {
	eval $self next $args
	$self instvar regagent_ encap_ decap_ agents_ address_ dmux_ id_

	if { $dmux_ == "" } {
		error "serious internal error at Node/MIPBS\n"
	}
	set regagent_ [new Agent/MIPBS $self]
	$self attach $regagent_ 0
	$regagent_ set mask_ [AddrParams set NodeMask_(1)]
	$regagent_ set shift_ [AddrParams set NodeShift_(1)]
	$regagent_ set dst_ [expr (~0) << [AddrParams set NodeShift_(1)]]

	set encap_ [new MIPEncapsulator]
	set decap_ [new Classifier/Addr/MIPDecapsulator]

	#
	# install en/de-capsulators
	#
	lappend agents_ $decap_
	
	#
	# Check if number of agents exceeds length of port-address-field size
	#
	set mask [AddrParams set PortMask_]
	set shift [AddrParams set PortShift_]
	
	if {[expr [llength $agents_] - 1] > $mask} {
		error "\# of agents attached to node $self exceeds port-field length of $mask bits\n"
	}
	
	if [Simulator set EnableHierRt_] {
		set nodeaddr [AddrParams set-hieraddr $address_]
		
	} else {
		set nodeaddr [expr ( $address_ &			\
				[AddrParams set NodeMask_(1)] ) <<	\
					[AddrParams set NodeShift_(1) ]]
	}
	$encap_ set addr_ [expr ((1 & $mask) << $shift) | ( ~($mask << $shift) & $nodeaddr)]
	$encap_ target [$self entry]
	$encap_ set node_ $self
	
	$dmux_ install 1 $decap_

	$encap_ set mask_ [AddrParams set NodeMask_(1)]
	$encap_ set shift_ [AddrParams set NodeShift_(1)]
	$decap_ set mask_ [AddrParams set NodeMask_(1)]
	$decap_ set shift_ [AddrParams set NodeShift_(1)]
    
}

Class Node/MIPMH -superclass Node/Broadcast

Node/MIPMH instproc init { args } {
	eval $self next $args
	$self instvar regagent_
	set regagent_ [new Agent/MIPMH $self]
	$self attach $regagent_ 0
	$regagent_ set mask_ [AddrParams set NodeMask_(1)]
	$regagent_ set shift_ [AddrParams set NodeShift_(1)]
	$regagent_ set dst_ [expr (~0) << [AddrParams set NodeShift_(1)]]
}

Agent/MIPBS instproc init { node args } {
    eval $self next $args
    
    # if mobilenode, donot use bcasttarget; use target_ instead;
    if {[$node info class] != "MobileNode/MIPBS"} {
	$self instvar BcastTarget_
	set BcastTarget_ [new Classifier/Replicator]
	$self bcast-target $BcastTarget_
    }
    $self beacon-period 1.0	;# default value
}

Agent/MIPBS instproc clear-reg mhaddr {
	$self instvar node_ OldRoute_ RegTimer_
	if [info exists OldRoute_($mhaddr)] {
		$node_ add-route $mhaddr $OldRoute_($mhaddr)
	}
	if {[$node_ info class] == "MobileNode/MIPBS"} {
	    eval $node_ clear-hroute [AddrParams get-hieraddr $mhaddr]
	}
	if { [info exists RegTimer_($mhaddr)] && $RegTimer_($mhaddr) != "" } {
		[Simulator instance] cancel $RegTimer_($mhaddr)
		set RegTimer_($mhaddr) ""
	}
}

Agent/MIPBS instproc encap-route { mhaddr coa lifetime } {
	$self instvar node_ TunnelExit_ OldRoute_ RegTimer_
	set ns [Simulator instance]
        set encap [$node_ set encap_]

        if {[$node_ info class] == "MobileNode/MIPBS"} {
	    set addr [AddrParams get-hieraddr $mhaddr]
	    set a [split $addr]
	    set b [join $a .]
	    #puts "b=$b"
	    $node_ add-hroute $b $encap
	} else {
	    set or [[$node_ set classifier_] slot $mhaddr]
	    if { $or != $encap } {
		set OldRoute_($mhaddr) $or
		$node_ add-route $mhaddr $encap
	    }
	}
	set TunnelExit_($mhaddr) $coa
	if { [info exists RegTimer_($mhaddr)] && $RegTimer_($mhaddr) != "" } {
		$ns cancel $RegTimer_($mhaddr)
	}
	set RegTimer_($mhaddr) [$ns at [expr [$ns now] + $lifetime] \
				    "$self clear-reg $mhaddr"]
}

Agent/MIPBS instproc decap-route { mhaddr target lifetime } {
    $self instvar node_ RegTimer_
    # decap's for mobilenodes can have a default-target; in this 
    # case the def-target is the routing-agent.
    
    if {[$node_ info class] != "MobileNode/MIPBS"} {
	set ns [Simulator instance]
	[$node_ set decap_] install $mhaddr $target
	
	if { [info exists RegTimer_($mhaddr)] && $RegTimer_($mhaddr) != "" } {
	    $ns cancel $RegTimer_($mhaddr)
	}
	set RegTimer_($mhaddr) [$ns at [expr [$ns now] + $lifetime] \
		"$self clear-decap $mhaddr"]
    } else {
	
	[$node_ set decap_] defaulttarget [$node_ set ragent_]
    }
}

Agent/MIPBS instproc clear-decap mhaddr {
	$self instvar node_ RegTimer_
    if { [info exists RegTimer_($mhaddr)] && $RegTimer_($mhaddr) != "" } {
	[Simulator instance] cancel $RegTimer_($mhaddr)
	set RegTimer_($mhaddr) ""
    }
    [$node_ set decap_] clear $mhaddr
}

Agent/MIPBS instproc get-link { src dst } {
    $self instvar node_
    if {[$node_ info class] != "MobileNode/MIPBS"} {
	set ns [Simulator instance]
	return [[$ns link [$ns get-node-by-addr $src] \
		[$ns get-node-by-addr $dst]] head]
    } else { 
	return ""
    }
}

Agent/MIPBS instproc add-ads-bcast-link { ll } {
	$self instvar BcastTarget_
	$BcastTarget_ installNext [$ll head]
}

Agent/MIPMH instproc init { node args } {
    eval $self next $args
    # if mobilenode, donot use bcasttarget; use target_ instead;
    if {[$node info class] != "MobileNode/MIPMH" && \
	    [$node info class] != "SRNode/MIPMH" } {
	$self instvar BcastTarget_
	set BcastTarget_ [new Classifier/Replicator]
	$self bcast-target $BcastTarget_
    }
    $self beacon-period 1.0	;# default value
}

Agent/MIPMH instproc update-reg coa {
    $self instvar node_
    ## dont need to set up routing for mobilenodes, so..
    if {[$node_ info class] != "MobileNode/MIPMH" && \
	    [$node_ info class] != "SRNode/MIPMH"} {
	set n [Node set nn_]
	set ns [Simulator instance]
	set id [$node_ id]
	set l [[$ns link $node_ [$ns get-node-by-addr $coa]] head]
	for { set i 0 } { $i < $n } { incr i } {
	    if { $i != $id } {
		$node_ add-route $i $l
	    }
	}
    }
}
    
Agent/MIPMH instproc get-link { src dst } {
    $self instvar node_
    if {[$node_ info class] != "MobileNode/MIPMH" && \
	    [$node_ info class] != "SRNode/MIPMH" } {
	set ns [Simulator instance]
	return [[$ns link [$ns get-node-by-addr $src] \
		[$ns get-node-by-addr $dst]] head]
    } else {
	return ""
    }
}

Agent/MIPMH instproc add-sol-bcast-link { ll } {
	$self instvar BcastTarget_
	$BcastTarget_ installNext [$ll head]
}







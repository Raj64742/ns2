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

Class MobileNode/MIPBS -superclass Node/MobileNode/BaseStationNode

MobileNode/MIPBS instproc init {args} {
    #eval $self next $args
    $self next $args
    $self instvar regagent_ encap_ decap_ agents_ address_ dmux_ id_

   if { $dmux_ == "" } {
       set dmux_ [new Classifier/Addr/Reserve]
       $dmux_ set mask_ [AddrParams set PortMask_]
       $dmux_ set shift_ [AddrParams set PortShift_]
       
       if [Simulator set EnableHierRt_] {  
	   $self add-hroute $address_ $dmux_
       } else {
	   $self add-route $address_ $dmux_
       }
   } 
   
   set regagent_ [new Agent/MIPBS $self]
   $self attach $regagent_ [Node/MobileNode  set REGAGENT_PORT]
   #$regagent_ set mask_ [AddrParams set NodeMask_(1)]
   #$regagent_ set shift_ [AddrParams set NodeShift_(1)]
   #$regagent_ set dst_ [expr (~0) << [AddrParams set NodeShift_(1)]]
   

   $self attach-encap 
   $self attach-decap
   
}

MobileNode/MIPBS instproc attach-encap {} {
    $self instvar encap_ address_ 
    
    set encap_ [new MIPEncapsulator]
    set mask [AddrParams set PortMask_]
    set shift [AddrParams set PortShift_]
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
    #$encap_ set mask_ [AddrParams set NodeMask_(1)]
    #$encap_ set shift_ [AddrParams set NodeShift_(1)]
}

MobileNode/MIPBS instproc attach-decap {} {
    $self instvar decap_ dmux_ agents_
    
    set decap_ [new Classifier/Addr/MIPDecapsulator]
    lappend agents_ $decap_
    set mask [AddrParams set PortMask_]
    set shift [AddrParams set PortShift_]
    if {[expr [llength $agents_] - 1] > $mask} {
	error "\# of agents attached to node $self exceeds port-field length of $mask bits\n"
    }
    $dmux_ install [Node/MobileNode set DECAP_PORT] $decap_
    #$decap_ set mask_ [AddrParams set NodeMask_(1)]
    #$decap_ set shift_ [AddrParams set NodeShift_(1)]
    #$decap_ def-target [$self entry]
}
    
Class MobileNode/MIPMH -superclass Node/MobileNode/BaseStationNode

MobileNode/MIPMH instproc init { args } {
    eval $self next $args
    $self instvar regagent_ dmux_ address_
 
    if { $dmux_ == "" } {
	set dmux_ [new Classifier/Addr/Reserve]
	$dmux_ set mask_ [AddrParams set PortMask_]
	$dmux_ set shift_ [AddrParams set PortShift_]
	
	if [Simulator set EnableHierRt_] {  
	    $self add-hroute $address_ $dmux_
	} else {
	    $self add-route $address_ $dmux_
	}
    } 
    
    set regagent_ [new Agent/MIPMH $self]
    $self attach $regagent_ [Node/MobileNode set REGAGENT_PORT]
    #$regagent_ set mask_ [AddrParams set NodeMask_(1)]
    #$regagent_ set shift_ [AddrParams set NodeShift_(1)]
    #$regagent_ set dst_ [expr (~0) << [AddrParams set NodeShift_(1)]]
    $regagent_ node $self
}

#Class SRNode -superclass Node/MobileNode

Class SRNode/MIPMH -superclass SRNode

SRNode/MIPMH instproc init { args } {
    eval $self next $args
    $self instvar regagent_ dmux_ address_
    
    if { $dmux_ == "" } {
	set dmux_ [new Classifier/Addr/Reserve]
	$dmux_ set mask_ [AddrParams set PortMask_]
	$dmux_ set shift_ [AddrParams set PortShift_]
	
	if [Simulator set EnableHierRt_] {  
	    $self add-hroute $address_ $dmux_
	} else {
	    $self add-route $address_ $dmux_
	}
    } 
    eval $self next $args
    set regagent_ [new Agent/MIPMH $self]
    $self attach $regagent_ [Node/MobileNode set REGAGENT_PORT]
    #$regagent_ set mask_ [AddrParams set NodeMask_(1)]
    #$regagent_ set shift_ [AddrParams set NodeShift_(1)]
    #$regagent_ set dst_ [expr (~0) << [AddrParams set NodeShift_(1)]]
    $regagent_ node $self
}
		
		
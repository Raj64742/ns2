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

#####################################################################################

# API's for allocating bits to ns addressing structure
# These routines should be used for setting bits for portid (agent), for nodeid 
# (flat or hierarchical).
# DEFADDRSIZE_ and MAXADDRSIZE_ defined in ns-default.tcl


# Name :            set_address() 
# Synopsis :        set_address
# Description:      This allows default settings in the following manner:
#                    * 8bits portid
#                    * if mcast option (EnableMcast) set,
#                      * 1 bit for mcast and 7 bits nodeid
#                      * else 8 bits nodeid
#                    * if Hierarchical routing option (EnableHierRt_) set,
#                      * sets a 3 level hierarchy with 8 bits in each level
#                      * or 8 8 7 (if mcast)
#
#
#

# Name :            set_portaddress() 
# Synopsis :        set_portaddress <#bits for portid> 
# Description :     This should be used incase of need to extend portid. Similar to 
#                   set_address(). 
#                   * Sets portbits as specified (increases size of addressspace, if reqd)
#                   * Sets mcast bit (if specified)
#                   * Sets remaining bits for nodeid.
#
#
#
#

# Name :            set_hieraddress() 
# Synopsis :        set_hieraddress <#levels of hierarchy> <#bits for each level>> 
# Description :     This should be used for hierarchical routing with specific 
#                   requirement of number of hierarchy levels. 
#                   
#                   * Sets 8 bits for portbits 
#                   * Sets mcast bit (if specified)
#                   * Sets hierchical levels with bits specified for each level.
#

# Errors returned : * if # of bits specified less than 0.
#                   * if bit positions clash (contiguous # of requested free bits not 
#                     found)
#                   * if total # of bits exceed MAXADDRSIZE_
#                   * if hierarchical setting is attempted w/o setting right option.
#                   * if # hierarchy levels donot match with #bits specified (for
#                     each level). 
# 
######################################################################################

Class AddrParams 
Class AllocAddrBits


Simulator instproc get-allocaddr {} {
    $self instvar allocAddr_
    if ![info exists allocAddr_] {
	set allocAddr_ [new AllocAddrBits]
    }
    return $allocAddr_
}



# sets address to default values  

Simulator instproc set-address {} {
    set a [$self get-allocaddr]
    $a set size_ [AllocAddrBits set DEFADDRSIZE_]
    $a set-portbits 8
    if [Simulator set EnableMcast_] {
	$a set-mcastbits 1
    }
    set lastbit [expr 8 - [$a set mcastsize_]]
    if [Simulator set EnableHierRt_] {
	$a set-idbits 3 8 8 $lastbit
    }
    if ![Simulator set EnableHierRt_] {
    # else {
	$a set-idbits 1 $lastbit
    }
}




#sets port bits and allocates remaining bits to id

Simulator instproc set-portaddress bits {
    if {$bits < 1} {
	error "\# bits for port < 1"
    }
    set a [$self get-allocaddr]
    $a set size_ [AllocAddrBits set DEFADDRSIZE_]
    $a set-portbits $bits
    if [Simulator set EnableMcast_] {
	$a set-mcastbits 1
    }
    set lastbit [expr [$a set size_] - [$a set mcastsize_] - $bits]
    $a set-idbits 1 $lastbit
} 



#sets hierarchical idbits

Simulator instproc set-hieraddress {hlevel args} {
    if ![Simulator set EnableHierRt_] {
	error "set_hieraddress: EnableHierRt_ not set"
    }
    set a [$self get-allocaddr]
    $a set size_ [AllocAddrBits set MAXADDRSIZE_]
    eval $a set-portbits 8 
    if [Simulator set EnableMcast_] {
	eval $a set-mcastbits 1
    }
    eval $a set-idbits $hlevel $args
}




AllocAddrBits instproc init {} {
    eval $self next
    $self instvar size_ portsize_ idsize_ mcastsize_ params_
    set size_ 0
    set portsize_ 0
    set idsize_ 0
    set mcastsize_ 0
    set params_ [new AddrParams]
}


AllocAddrBits instproc get-address {} {
    $self instvar addrStruct_
    if ![info exists addrStruct_] {
	set addrStruct_ [new AllocAddr]
    }
    return $addrStruct_
}


AllocAddrBits instproc chksize {bit_num prog} {
    $self instvar size_ portsize_ idsize_ mcastsize_  
    if {$bit_num <= 0 } {
	error "$prog : \# bits less than 1"
    }
    set totsize [expr $portsize_ + $idsize_ + $mcastsize_]
    if {$totsize > [AllocAddrBits set MAXADDRSIZE_]} {
	error "$prog : Total \# bits exceed MAXADDRSIZE"
    }
    if {$totsize > [AllocAddrBits set DEFADDRSIZE_]} {
	set size_ [AllocAddrBits set MAXADDRSIZE_]
    }

}



AllocAddrBits instproc set-portbits {bit_num} {
    $self instvar size_ portsize_ params_
    set portsize_ $bit_num
    $self chksize $portsize_ "setport"
    set a [$self get-address] 
    set v [$a setbit $bit_num $size_]
    $params_ set PortMask_ [lindex $v 0]
    $params_ set PortShift_ [lindex $v 1]
    ### TESTING
    set mask [lindex $v 0]
    set shift [lindex $v 1]
    puts "size = $size_\n Portshift = $shift \n PortMask = $mask\n"
}



AllocAddrBits instproc set-mcastbits {bit_num} {
    $self instvar size_ mcastsize_ params_
    if {$bit_num != 1} {
	error "setmcast : mcastbit > 1"
    }
    set mcastsize_ $bit_num
    $self chksize mcastsize_ "setmcast"
    set mcastsize_ $bit_num
    set a [$self get-address] 
    set v [$a setbit $bit_num $size_]
    $params_ set McastMask_ [lindex $v 0]
    $params_ set McastShift_ [lindex $v 1]
    ### TESTING
    set mask [lindex $v 0]
    set shift [lindex $v 1]
    puts "size = $size_\n Mcastshift = $shift \n McastMask = $mask\n"
}


AllocAddrBits instproc set-idbits {nlevel args} {
    $self instvar size_ portsize_ idsize_ params_
    if {$nlevel != [llength $args]} {
	error "setid: hlevel < 1 OR nlevel and \# args donot match"
    }
    set a [$self get-address] 
    set old 0
    for {set i 0} {$i < $nlevel} {incr i} {
	set idlevel($i) [lindex $args $i]
	set idsize_ [expr $idsize_ + $idlevel($i)]
	$self chksize $idlevel($i) "setid"
	set v [$a setbit $idlevel($i) $size_]
     	$params_ set NodeMask_([expr $i+1]) [lindex $v 0]
     	$params_ set NodeShift_([expr $i+1]) [lindex $v 1]
	### TESTING
	set mask [lindex $v 0]
	set shift [lindex $v 1]
	puts "size = $size_\n Nodeshift = $shift \n NodeMask = $mask\n"
	set old [expr $old + $idlevel($i)]
    }
    
}


AddrParams instproc init {} {
    eval $self next
    $self instvar PortShift_ PortMask_ NodeShift_ NodeMask_ 
    $self instvar McastShift_ McastMask_
  
}

AddrParams proc instance {} {
    set ap [AddrParams info instances]
    if {$ap != ""} {
	return $ap
    }
    
    error "Cannot find instance of AddrParams"
}








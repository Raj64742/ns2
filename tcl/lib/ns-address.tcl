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


# Name :            set-address-format ()
# Options :         default, expanded, hierarchical (default) and hierarchical (with
#                    specific allocations) 
# Synopsis :        set-address-format <option> <additional info, if required by option> 
#
# Option:1          def (default settings)
# Synopsis :        set-address-format def
# Description:      This allows default settings in the following manner:
#                    * 8bits portid
#                    * 1 bit for mcast and 7 bits nodeid (mcast should be enabled
#                      before Simulator is inited (new Simulator is called)--> leads to
#                      unnecessary wasting of 1 bit even if mcast is not set.
#
#
# Option :2         expanded
# Synopsis :        set-address-format expanded
# Description:      This allows to expand the address space from default size 16 to 32
#                   with bit allocated to the field in the foll manner:
#                    * 8bits portid
#                    * 1 bit for mcast and 21 bits nodeid (and same comments regarding
#                       setting mcast as for default option above)
#
#
# Option :3         hierarchical (default)
# Synopsis :        set-address-format hierarchical
# Description:      This allows setting of hierarchical address as follows:
#                   * Sets 8bits portid
#                   * Sets mcast bit (if specified)
#                   * Sets default hierchical levels with:
#                      * 3 levels of hierarchy and
#                      * 8 bits in each level (looks like 8 8 8)
#                      * 8 8 7 if mcast is enabled.
#
#
# Option :4        hierarchical (specified)
# Synopsis :        set-address-format hierarchical <#n hierarchy levels> <#bits for 
#                   level1> <#bits for level 2> ....<#bits for nth level>
#                   e.g  
#                   set-address-format hierarchical 4 2 2 2 10
# Description:      This allows setting of hierarchical address as specified for each 
#                   level.
#                   * Sets 8bits portid
#                   * Sets mcast bit (if specified)
#                   * Sets hierchical levels with bits specified for each level.



# Name :            expand-port-field-bits ()
# Synopsis :        expand-port-field-bits <#bits for portid> 
# Description :     This should be used incase of need to extend portid. 
#                   This commnad may be used in conjuction with 
#                   set-address-format command explained above.
#                   expand-port-field-bits checks and raises error in the foll. cases
#                    * if the requested portsize cannot be accomodated (i.e  
#                         if sufficient num.of free bits are not available)
#                    * if requested portsize is less than equal to existing portsize.
#                   and incase of no errors sets port field with bits as specified.
#2
#
#
#


# Errors returned : * if # of bits specified less than 0.
#                   * if bit positions clash (contiguous # of requested free bits not 
#                     found)
#                   * if total # of bits exceed MAXADDRSIZE_
#                   * if expand-port-field-bits is attempted with portbits less or equal
#                     to the existing portsize.
#                   * if # hierarchy levels donot match with #bits specified (for
#                     each level). 
# 
######################################################################################



Class AddrParams 
Class AllocAddrBits


Simulator instproc get-AllocAddrBits {prog} {
	$self instvar allocAddr_
	if ![info exists allocAddr_] {
		set allocAddr_ [new AllocAddrBits]
	} elseif ![string compare $prog "new"] {
		# puts "Warning: existing Address Space was destroyed\n"
		set allocAddr_ [new AllocAddrBits]
	}
	return $allocAddr_
}


Simulator instproc set-address-format {opt args} {
	set len [llength $args]
	if {[string compare $opt "def"] == 0} {
		$self set-address 8 8
	} elseif {[string compare $opt "expanded"] == 0} {
		$self set-address 23 8
	} elseif {[string compare $opt "hierarchical"] == 0 && $len == 0} {
		if [Simulator set EnableMcast_] {
			
			$self set-hieraddress 3 6 8 8
		} else {
			$self set-hieraddress 3 7 8 8
		}
	} else {
		if {[string compare $opt "hierarchical"] == 0 && $len > 0} {
			eval $self set-hieraddress [lindex $args 0] [lrange $args 1 [expr $len - 1]]
		}
	} 
}


Simulator instproc expand-port-field-bits nbits {
	set a [$self get-AllocAddrBits "notnew"]
	if {$nbits <= [$a set portsize_]} {
		error "expand-port-field-bits:requested portsize less than or equal to existing portsize"
	} 

	$a expand-portbits $nbits
}


# sets address for nodeid and port fields 
# The order of setting bits for different fields does matter
# and should be as follows:
# mcast
# idbits
# and finally portbits
# this is true for both set-address and set-hieraddress

Simulator instproc set-address {node port} {
	set a [$self get-AllocAddrBits "new"]
	$a set size_ [AllocAddrBits set DEFADDRSIZE_]
	if {[expr $node + $port] > [$a set size_]} {
		$a set size_ [AllocAddrBits set MAXADDRSIZE_]
	}
	
	# one bit is set aside for mcast as default :: this waste of 1 bit may be avoided, if 
	# mcast option is enabled before the initialization of Simulator.
	#     if [Simulator set EnableMcast_] {
	$a set-mcastbits 1
	#     }

	set lastbit [expr $node - [$a set mcastsize_]]
	$a set-idbits 1 $lastbit
	$a set-portbits $port

}




#sets hierarchical bits

Simulator instproc set-hieraddress {hlevel args} {
	set a [$self get-AllocAddrBits "new"]
	$a set size_ [AllocAddrBits set MAXADDRSIZE_]
	if ![Simulator set EnableHierRt_] {
		Simulator set EnableHierRt_ 1
		Simulator set node_factory_ HierNode
	}
	if [Simulator set EnableMcast_] {
		$a set-mcastbits 1
	}
	eval $a set-idbits $hlevel $args
	$a set-portbits 8 
}




AllocAddrBits instproc init {} {
	eval $self next
	$self instvar size_ portsize_ idsize_ mcastsize_
	set size_ 0
	set portsize_ 0
	set idsize_ 0
	set mcastsize_ 0
}


AllocAddrBits instproc get-AllocAddr {} {
	$self instvar addrStruct_
	if ![info exists addrStruct_] {
		set addrStruct_ [new AllocAddr]
	}
	return $addrStruct_
}

AllocAddrBits instproc get-Address {} {
	$self instvar address_
	if ![info exists address_] {
		set address_ [new Address]
	}
	return $address_
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
	if { $size_ < [AllocAddrBits set MAXADDRSIZE_]} {
		if {$totsize > [AllocAddrBits set DEFADDRSIZE_]} {
			set size_ [AllocAddrBits set MAXADDRSIZE_]
			return 1
		} 
	}
	return 0

}



AllocAddrBits instproc set-portbits {bit_num} {
	$self instvar size_ portsize_ 
	set portsize_ $bit_num
	if [$self chksize portsize_ "setport"] {
		error "set-portbits: size_ has been changed."
	}
	
	set a [$self get-AllocAddr] 
	set v [$a setbit $bit_num $size_]
	
	AddrParams set PortMask_ [lindex $v 0]
	AddrParams set PortShift_ [lindex $v 1]
	### TESTING
	# set mask [lindex $v 0]
	# set shift [lindex $v 1]
	# puts "Portshift = $shift \n PortMask = $mask\n"

	set ad [$self get-Address]
	$ad portbits-are [AddrParams set PortShift_] [AddrParams set PortMask_]
}



AllocAddrBits instproc expand-portbits nbits {
	$self instvar portsize_ size_ mcastsize_ idsize_ hlevel_ hbits_ addrStruct_
	set temp $portsize_
	set portsize_ $nbits
	
	# these extra checks and re-allocation are done to ensure mcast bit is always the
	# MSB; so when size_ gets expanded as a result of expanding port, need to re-allocate 
	# mcast and id bits.
	
	if [$self chksize $nbits "expandport"] {
		set addrStruct_ [new AllocAddr]
		if {$mcastsize_ > 0} {
			$self set-mcastbits $mcastsize_
		}
		if {$idsize_ > 0} {
			eval $self set-idbits $hlevel_ $hbits_ 
		}
		$self set-portbits $nbits
	} else {
		set a [$self get-AllocAddr] 
		set v [$a expand-port [expr $nbits - $temp] $size_ $temp ]
		AddrParams set PortMask_ [lindex $v 0]
		AddrParams set PortShift_ [lindex $v 1]
		set ad [$self get-Address]
		$ad portbits-are [AddrParams set PortShift_] [AddrParams set PortMask_]
		### TESTING
		# set mask [lindex $v 0]
		# set shift [lindex $v 1]
		# puts "size = $size_\n Portshift = $shift \n PortMask = $mask\n"
	}
}



AllocAddrBits instproc set-mcastbits {bit_num} {
	$self instvar size_ mcastsize_
	if {$bit_num != 1} {
		error "setmcast : mcastbit > 1"
	}
	set mcastsize_ $bit_num

	#chk to ensure there;s no change in size
	if [$self chksize mcastsize_ "setmcast"] {
		# assert {$chk == 0} --> assert doesn't seem to work
		error "set-mcastbits: size_ has been changed."
	}
	set a [$self get-AllocAddr] 
	set v [$a setbit $bit_num $size_]
	AddrParams set McastMask_ [lindex $v 0]
	AddrParams set McastShift_ [lindex $v 1]
	### TESTING
	# set mask [lindex $v 0]
	# set shift [lindex $v 1]
	# puts "Mcastshift = $shift \n McastMask = $mask\n"

	set ad [$self get-Address]
	$ad mcastbits-are [AddrParams set McastShift_] [AddrParams set McastMask_]

}


AllocAddrBits instproc set-idbits {nlevel args} {
	$self instvar size_ portsize_ idsize_ hlevel_ hbits_
	if {$nlevel != [llength $args]} {
		error "setid: hlevel < 1 OR nlevel and \# args donot match"
	}
	set a [$self get-AllocAddr] 
	set old 0
	set idsize_ 0
	set nodebits 0
	AddrParams set hlevel_ $nlevel
	set hlevel_ $nlevel
	for {set i 0} {$i < $nlevel} {incr i} {
		set bpl($i) [lindex $args $i]
		set idsize_ [expr $idsize_ + $bpl($i)]
		
		# check to ensure there's no change in size
		set chk [$self chksize $bpl($i) "setid"]
		# assert {$chk ==0}
		if {$chk > 0} {
			error "set-idbits: size_ has been changed."
		}
		set v [$a setbit $bpl($i) $size_]
		AddrParams set NodeMask_([expr $i+1]) [lindex $v 0]
		set m([expr $i+1]) [lindex $v 0]
		AddrParams set NodeShift_([expr $i+1]) [lindex $v 1]
		set s([expr $i+1]) [lindex $v 1]
		lappend hbits_ $bpl($i)
		
	}
	AddrParams set nodebits_ $idsize_
	set ad [$self get-Address]
	eval $ad idsbits-are [array get s]
	eval $ad idmbits-are [array get m]
	
	### TESTING
	# set mask [lindex $v 0]
	# set shift [lindex $v 1]
	# puts "Nodeshift = $shift \n NodeMask = $mask\n"
}


### Hierarchical routing support

#
# create a real hier address from addr string 
#
AddrParams proc set-hieraddr addrstr {
	AddrParams instvar hlevel_ NodeShift_ NodeMask_
	set L [split $addrstr .]
	if { [llength $L] != $hlevel_ } {
		error "set-hieraddr: hierarchical address doesn't match with \# hier.levels\n"
	}
	set word 0
	for {set i 1} {$i <= $hlevel_} {incr i} {
		set word [expr [expr [expr [lindex $L [expr $i-1]] & $NodeMask_($i)] << $NodeShift_($i)] | [expr [expr ~[expr $NodeMask_($i) << $NodeShift_($i)]] & $word]]
		#TESTING
		# puts "word = $word"
	}
	return $word
}





#
# Returns number of elements at a given hierarchical level, that is visible to 
# a node.
#
AddrParams proc elements-in-level? {nodeaddr level} {
	AddrParams instvar hlevel_ domain_num_ cluster_num_ nodes_num_ def_
	set L [split $nodeaddr .] 
	set level [expr $level + 1]
	#
	# if no topology info found for last level, set default values
	#
	
	### for now, assuming only 3 levels of hierarchy 
	if { $level == 1} {
		return $domain_num_
	}
	if { $level == 2} {
		return [lindex $cluster_num_ [lindex $L 0]]
	}
	if { $level == 3} {
		set C 0
		set index 0
		while {$C < [lindex $L 0]} {
			set index [expr $index + [lindex $cluster_num_ $C]]
			incr C
		}
		return [lindex $nodes_num_ [expr $index + [lindex $L 1]]]
	}
		
}




#
# Given an node's address, Return the node 
#
Simulator instproc get-node-by-addr address {
	$self instvar Node_
	set n [Node set nn_]
	for {set q 0} {$q < $n} {incr q} {
		set nq $Node_($q)
		if {[string compare [$nq node-addr] $address] == 0} {
			return $nq
		}
	}
	error "get-node-by-addr:Cannot find node with given address"
}

#
# Given an node's address, Return the node-id
#
Simulator instproc get-node-id-by-addr address {
	$self instvar Node_
	set n [Node set nn_]
	for {set q 0} {$q < $n} {incr q} {
		set nq $Node_($q)
		if {[string compare [$nq node-addr] $address] == 0} {
			return $q
		}
	}
	error "get-node-id-by-addr:Cannot find node with given address"
}

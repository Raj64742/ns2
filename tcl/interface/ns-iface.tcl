 #
 # tcl/interface/ns-iface.tcl
 #
 # Copyright (C) 1997 by USC/ISI
 # All rights reserved.                                            
 #                                                                
 # Redistribution and use in source and binary forms are permitted
 # provided that the above copyright notice and this paragraph are
 # duplicated in all such forms and that any documentation, advertising
 # materials, and other materials related to such distribution and use
 # acknowledge that the software was developed by the University of
 # Southern California, Information Sciences Institute.  The name of the
 # University may not be used to endorse or promote products derived from
 # this software without specific prior written permission.
 # 
 # THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 # WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 # MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 # 
 # Ported by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 # 
 #
Class NetInterface -superclass InitObject
NetInterface set ifacenum 0
NetInterface proc getid {} {
	set id [NetInterface set ifacenum]
	NetInterface set ifacenum [expr $id + 1]
	return $id
}

NetInterface instproc init {} {
	$self next
	$self instvar id iface
	set id [NetInterface getid]
	set iface [new networkinterface]
	$iface label $id
	# puts "$self $iface creating i/f, label is $id"
}

NetInterface instproc set-label { num } {
	$self instvar iface id
	set id $num 
        $iface label $id
}


NetInterface instproc entry {} {
	$self instvar iface
	return $iface
}

NetInterface instproc exitpoint {} {
	$self instvar iface
	return $iface
}

NetInterface instproc id {} {   
        $self instvar id
        return $id
}

NetInterface instproc setNode node_ {
	$self instvar node iface
	set node $node_
	$iface target [$node entry]
}

NetInterface instproc getNode {} {
	$self instvar node
	return $node
}

###################### Class DuplexNetInterface #####################

Class DuplexNetInterface -superclass NetInterface
DuplexNetInterface instproc init {} {
	$self next
	$self instvar ifaceout id
	set ifaceout [new networkinterface]
	$ifaceout label $id
}

DuplexNetInterface instproc exitpoint {} {
	$self instvar ifaceout
	return $ifaceout
}

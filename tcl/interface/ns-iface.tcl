Class NetInterface
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

 #
 # tcl/mcast/pimDM.tcl
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
 # Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 # 
 #
Class pimDM -superclass DM

pimDM instproc init { sim node } {
	set type "pimDM"
	$self next $sim $node
}

pimDM instproc handle-cache-miss { srcID group iface } {
        $self instvar node_

	set oiflist ""
	set alloifs [$node_ get-oifs]
	set iifx [$node_ get-oifIndex [[$node_ rpf-nbr $srcID] id]]

	if { $iifx != $iface && $iface >= 0} {
		$self handle-wrong-iif $srcID $group $iface
		return
	}
	$node_ instvar outLink_

	foreach oifx $alloifs {
		if { $iifx != $oifx } {
			lappend oiflist $outLink_($oifx)
		}
	}
	$node_ add-mfc $srcID $group $iifx $oiflist
}

pimDM instproc handle-wrong-iif { srcID group iface } {
	$self instvar node_
	puts "warning: pimDM $self wrong incoming interface src:$srcID group:$group iface:$iface"
	$self instvar node_
	set from [[$node_ ifaceGetNode $iface] id]
	$self send-ctrl "prune" $srcID $group $from
}

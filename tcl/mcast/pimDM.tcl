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

pimDM set PruneTimeout 0.5

pimDM instproc init { sim node } {
	set type "pimDM"
	$self next $sim $node
}

pimDM instproc handle-cache-miss { srcID group iface } {
        $self instvar node_ ns_

	set oiflist ""

	if { $iface >= 0 } {
		set rpf_nbr [$node_ rpf-nbr $srcID]
		set inlink  [$node_ iif2link $iface]
		set rpflink [$ns_ link $rpf_nbr $node_]

		if { $inlink != $rpflink } {
			set from [$inlink src]
			$self send-ctrl "prune" $srcID $group [$from id]
			$node_ add-mfc $srcID $group $iface ""
			return
		}
		set rpfoif [$node_ link2oif [$ns_ link $node_ $rpf_nbr]]
	} else {
		set rpfoif ""
	}
	foreach oif [$node_ get-all-oifs] {
		if { $rpfoif != $oif } {
			lappend oiflist $oif
		}
	}
	$node_ add-mfc $srcID $group $iface $oiflist
}

pimDM instproc handle-wrong-iif { srcID group iface } {
	$self instvar node_ ns_
	set inlink  [$node_ iif2link $iface]
	set from [$inlink src]
	$self send-ctrl "prune" $srcID $group [$from id]
}



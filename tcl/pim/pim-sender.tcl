 #
 # tcl/pim/pim-sender.tcl
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
 # Contributed by Ahmed Helmy, Ported by Polly Huang (USC/ISI), 
 # http://www-scf.usc.edu/~bhuang
 # 
 #
# XXX can call send-join with: 
# 	"WC" 0 $group, this is a *,G join
#	"SG" $source $group, this is a S,G join
#	"WW" $RP 0, this is a *,*,RP join
PIM instproc send-join { code source group } {
	$self instvar messager ns Node MRTArray
	switch $code {
		"WC" {
			# include RP as src in the msg.. !
			set source [$MRTArray($group) getRP]
			set finaldst $source
			set dst [$self nexthopWC $group]
		}
		"SG" {
			# XXX
			set dst [$self nexthopSG $source $group]
		}
		"WW" {
			# XXX
			set dst [$self nexthopWW $source]
		}
		default { return 0 }
	}
	# if I am the next hop then return
	if { $dst == [$Node id] } { 
		# puts "node [$Node id] dst is same, won't send join"
		return 0 
	}
	set oifInfo [$Node get-oif [$ns set link_([$Node id]:$dst)]]
	set oif [lindex $oifInfo 1]
	# puts "node [$Node id] $self send join $dst $code $source $group $finaldst $oif"
	$self send [PIM set JOIN] $dst $code $source $group $finaldst $oif
}

PIM instproc send-prune { code source group } {
	$self instvar messager ns Node MRTArray
	set finaldst -1
	switch $code {
		"SG_RP" {
			set dst [$self nexthopWC $group]
			set finaldst [$MRTArray($group) getRP]
		}
	}
	if { $dst == [$Node id] } {
		# puts "node [$Node id] dst is same, won't send prune.."
		return 0
	}
	set oifInfo [$Node get-oif [$ns set link_([$Node id]:$dst)]]
	set oif [lindex $oifInfo 1]
	# puts "node [$Node id] $self send prune"
	$self send [PIM set PRUNE] $dst $code $source $group $finaldst $oif
}

PIM instproc send { type dst code source group finaldst oif } {
	$self instvar messager
	$messager set class_ $type
	#reset the dst just in case
	$messager set dst_ [PIM set ALL_PIM_ROUTERS]
	
	# XXX send to the oif directly like a raw socket!
	if { $oif != -1 } { $messager target $oif }

	# XXX may want to include the iface too.. later !!
	$self instvar Node
	
	# XXX include my id in the msg... chk how to get this info from
	# the pkt hdr at the message agent.. !! later.. ! 
	# if doing i/f routing include i/f address ! not yet supported.
	set originator [$Node id]

	# puts "node [$Node id] $self send $type/$dst/$code/$source/$group/$originator/$finaldst"

	$messager send "$type/$dst/$code/$source/$group/$originator/$finaldst"
}

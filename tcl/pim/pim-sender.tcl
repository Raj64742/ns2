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

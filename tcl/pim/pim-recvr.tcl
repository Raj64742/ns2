PIM instproc drop { replicator src dst } {
        $self instvar Node
        # XXX set ignore to avoid further calls on drop
        $replicator set ignore 1
}

# upcall handler [cache misses, wrong iifs,.. whole pkts.. ]
# don't use "args", it adds another level of listing.. ! use argslist.. !!!
PIM instproc upcall { argslist } {  
        # the args are passed as code, srcID, group, iface
        # puts "pim $self upcall $argslist"
        set code [lindex $argslist 0]
        ## remove code from the args list
        set argslist [lreplace $argslist 0 0]
        switch $code {
                "CACHE_MISS" { $self handle-cache-miss $argslist }
                "WRONG_IIF" { $self handle-wrong-iif $argslist }
                default { puts "no match for upcall, code is $code" }
        }
}

# XXX need to complete handling of wrong iif
PIM instproc handle-wrong-iif { argslist } {
	puts "WRONG IIF $argslist"
}

PIM instproc handle-pim-group { srcID group } {
        $self instvar Node pimNbrs ns
        # I don't check iif for pim group
        # puts "pim $self handle-pim-group $srcID $group"
        set oiflist ""
        
        # should never happen
        if { $srcID == [$Node id] } {
           puts "node [$Node id] $self, error... src local"
           return 0
        }
        # if src is not local, install only local agents
        
        # add-mfc src grp iif oiflist
        $Node add-mfc $srcID $group -1 $oiflist
}

# this draws heavily from process_cacheMiss in route.c
PIM instproc handle-cache-miss { argslist } {
        $self instvar Node groupArray MRTArray RPArray reg_vif_num

        set srcID [lindex $argslist 0]
        set group [lindex $argslist 1]
        set iface [lindex $argslist 2]
            
        # puts "pim $self handle-cache-miss $argslist"
        # XXX special processing for all pim routers..
        if { $group == [PIM set ALL_PIM_ROUTERS] } {
                $self handle-pim-group $srcID $group
                return
        }   
        
        # check if I am the DR for the source
        if [$self DR $srcID $iface] {
           # check if I am not the RP, then register
           # puts "I am the DR,.. i.e. local pkt" 
           if [info exists MRTArray($group)] {
                set RP [$MRTArray($group) getRP]
           } else {
                set RP [lindex [$self getMap $group] 0]
           }
           if { $RP >= 0 && $RP != [$Node id] } {  
                $self findSG $srcID $group 1 [PIM set REG]
		# now we don't do RPF checks in regs... need igmp for that
                $self change-cache $srcID $group "REG" -1
                return 1
           }
        } elseif { [info exists MRTArray($group)] } {
		# should do a longest match later..
                $self findSG $srcID $group 1 0
                # may add a list of cache entries to the *,G entry.. !
                $self change-cache $srcID $group "SG" $iface
        }
}

PIM instproc DR { srcID iface } {
        $self instvar Node
        # now only check if local
        if { $srcID != [$Node id] } { return 0 }
        return 1
}

PIM instproc recv-join { msg } {
	$self instvar Node MRTArray
	set dst [lindex $msg 0]
	# check if I am the dst.. o.w. perform join suppression.. ! later
	# puts "node [$Node id] recv-join msg $msg"
	if { [$Node id] != $dst } {
		# puts "node [$Node id] rxvd join destined to $dst"
		return 0 
	}
	set code [lindex $msg 1]
	set source [lindex $msg 2]
	set group [lindex $msg 3]
	set origin [lindex $msg 4]
	# may chk if grp valid before processing... merely safety.. ! later!
	switch $code {
		"WC" { 
			# process *,G join
			# puts "node [$Node id] got *,G join"
			# check the hash for the grp
			if [$self findWC $group 0] {
			   set notfound 0
			   set hashedRP [lindex [$MRTArray($group) getMap] 0]
			} else {
			   set notfound 1
			   set hashedRP [lindex [$self getMap $group] 0]
			}
			if { $hashedRP != $source } {
				# puts "hash mismatch dropping join"
				return 0
			}
			$self findWC $group 1
			$self add-oif $MRTArray($group) $origin "WC"
			if $notfound { $self send-join "WC" 0 $group }
			return 1
		}
		"SG" {
			# process S,G join
		}
		"WW" {
			# process *,*,RP join
		}
		default { return 0 }
	}
}

PIM instproc recv-prune { msg } {
	$self instvar Node MRTArray
	set dst [lindex $msg 0]
	# puts "node [$Node id] recv-prune msg $msg"
	if { [$Node id] != $dst } {
		# puts "node [$Node id] rxvd prune destined to $dst"
		# perform prune overrides here.. !
		return 0
	}
	set code [lindex $msg 1]
	set source [lindex $msg 2]
	set group [lindex $msg 3]
	set origin [lindex $msg 4]
	switch $code {
		"SG_RP" {
		   # puts "node [$Node id] got SG RP prune"
		   $self findSG $source $group 1 [PIM set RP]
		   set ent $MRTArray($source:$group)
		   $self del-oif $ent $origin "SG_RP"
		   if { [$self compute-oifs $ent] == "" && 
					![$Node check-local $group] } {
			$self send-prune "SG_RP" $source $group
		   }

		}
	}
}

PIM instproc recv-assert { msg } {

}

PIM instproc reg-stop { srcAdd group drAdd } {
	$self instvar regStopAgent
	if { ! [info exists regStopAgent] } {
		set regStopAgent [new Agent/Message/regStop $self]
	}
	$regStopAgent set dst $drAdd
	# construct the reg stop msg.. etc
}

PIM instproc recv-register { msg } {
	$self instvar Node MRTArray decapAgent regVif regVifNum
	# puts "node [$Node id] recv register"
	# format "$type/$mysrcAdd/$origsrcAdd/$group/$nullBit/$borderBit/$msg"
	# type is removed in handle
	set drAdd [lindex $msg 0]
	set origAdd [lindex $msg 1]
	set group [lindex $msg 2]
	set mesg [lindex $msg 5]

	# puts "drAdd $drAdd origAdd $origAdd group $group"

	# process null and border bits
	# if no longest match, send reg stop..to mysrcAdd or drAdd
	if { ! [info exists MRTArray($group)] } { 
		# send registerStop.. need regStopAgent for this.. !!XXXchk
		return 0
	}
	set srcID [expr $origAdd >> 8]
	$self findSG $srcID $group 1 0
	set ent $MRTArray($srcID:$group)
	$ent setflags [PIM set IIF_REG]
	# if no cache, chk oifs, if null, chck if there is local agent.. 
	if { [$self compute-oifs $ent] == "" && ![$Node check-local $group] } {
		# puts " null oif src $srcID grp $group, should send reg stop"
		# XXX send reg-stop.. to do
		return 0
	}

	if { ! [info exists decapAgent] } {
		set decapAgent [new Agent/Message]
		# check how to include the pkt label.. target a reg_vif
		# reg_vif targets the node entry.. XXXXXXXX chk chk
		# install the reg_vif number as iif...
		set regVif [$Node get-vif]
		set regVifNum [expr { 
			([$regVif info class] == "NetInterface") ? 
				[$regVif id] : -1}]
		$decapAgent target [$regVif entry]
	}

	if { ! [expr [$ent getflags] & [PIM set CACHE]] } {
	  # puts "installing cache w/iif register"
	  $self change-cache $srcID $group "SG" $regVifNum
	}

	$decapAgent set addr_ $origAdd
	$decapAgent set dst_ $group
	# XXX see if we should include the cls in the msg and set it
	# XXX
	$decapAgent set class_ 2

	$decapAgent send "$mesg"
}

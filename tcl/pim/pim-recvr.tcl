 #
 # tcl/pim/pim-recvr.tcl
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

PIM instproc handle-wrong-iif { argslist } {
        #puts "WRONG IIF $argslist"
        $self instvar MRTArray Node
        set srcID [lindex $argslist 0]
        set group [lindex $argslist 1]
        set iface [lindex $argslist 2]

        if { ! [info exists MRTArray($srcID:$group)] } {
             #puts "_node [$Node id] proto $self: error, wrongiif & no S,G"
             return 0
        }
        set flags [$MRTArray($srcID:$group) getflags]
        if { ! [expr $flags & [PIM set CACHE]] } {
           #puts "_node [$Node id] proto $self: error, wrongiif & no cache"
           return 0
        }
 
        if [expr $flags & [PIM set IIF_REG]] {
           #puts "IIF_REG set checking for switch to spt"
           # XXX to be completed  
           # if { $iface == iif on RPF nbr towards scrID } { }
           # better make getting iif towards src modular to use elsewhere too
                if [expr $flags & [PIM set SPT]] {
                   # should never happen
                   #puts "_node [$Node id] proto $self: error, spt and iifreg!"
                }
           # XXX set SPT bit, turn off iif_reg & change iif in cache
           set flags [expr $flags & ~[PIM set IIF_REG] | [PIM set SPT]]   
           # change iif in cache and SG entry !!
           return 1
        }

        # XXX if { iif(WC) != iif(SG) && iface == iif(SG) } {
        # switch to spt, and prune off of shared tree
        # }
        # where iif(SG) is iif towards RPF nbr to the src !

        # if iface is in oiflist(computeoifs) then trigger assert
        set oifs [$self compute-oif-ids $MRTArray($srcID:$group)]
        if { [set idx [$self index $oifs $iface]] != -1 } {   
                set code [expr {($flags & [PIM set SG]) ? "SG" : "WC"}]
                set oiface [$Node label2iface $iface]
                $self send-assert $srcID $group $code $oiface
        } else {
                # puts "wrong iif, but iface not in oiflist !!!"
                return 0
        }
}

# this draws heavily from process_cacheMiss in route.c
PIM instproc handle-cache-miss { argslist } {
        $self instvar Node MRTArray
        set srcID [lindex $argslist 0]
        set group [lindex $argslist 1]
        set iface [lindex $argslist 2]
      
        if [$self DR $srcID $iface] {
           if [info exists MRTArray($group)] {
                set RP [$MRTArray($group) getRP]
           } else {
                set RP [lindex [$self getMap $group] 0]
           }
           if { $RP != [$Node id] } {
                # XXX allow turning off registers for SPTs.. !
                $self instvar RegisterOff
                if [info exists RegisterOff] {
                        #puts "RegisterOff"
                        $self findSG $srcID $group 1 0
                        $MRTArray($srcID:$group) setiif -1
                        $self change-cache $srcID $group "SG" -1
                } else {
                        $self findSG $srcID $group 1 [PIM set REG]
                        # now we don't do RPF checks in regs... need igmp!
                        $MRTArray($srcID:$group) setiif -1
                       $self change-cache $srcID $group "REG" -1
                }
                return 1
           }  elseif { [info exists MRTArray($group)] ||
                                [info exists MRTArray($srcID:$group)] } {
                #puts "_node [$Node id] I am the RP"
                # should do a longest match later..
                $self findSG $srcID $group 1 0
                # may add a list of cache entries to the *,G entry.. !
                $MRTArray($srcID:$group) setiif -1
                $self change-cache $srcID $group "SG" -1
           }
        } elseif { [set ent [$self longest_match $srcID $group]] != 0 } {
                # XXX note: can do like implem, always install the
                # rite iif in cache,... move iif into change-cache ! later!
                # check iif
                set iif [$ent getiif]
                if { $iif == -2 } {
                   #puts "_node [$Node id] proto $self, err: iif -2"
                  return -1
                }
                if { $iface == $iif || $iif == -1 } {
                   $self findSG $srcID $group 1 0
                   $MRTArray($srcID:$group) setiif $iface
                   # may add a list of cache entries to the *,G entry.. !
                   $self change-cache $srcID $group "SG" $iface
                } elseif { [$ent getType] == "SGEnt" &&
                           ! [expr [$ent getflags] & [PIM set SPT]] } {
                   if [info exists MRTArray($group)] {
                     set WCiif [$MRTArray($group) getiif]
                     if { $iface == $WCiif } {
                         $self change-cache $srcID $group "SG" $iface
                         # puts "matched on SG, but iif is WC"
                     }
                   }
                } else {
                        # puts "wrong iif in cache miss !!!"
                }
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
        #puts "node [$Node id] recv-join msg $msg"
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
                        if $notfound {
                                if { ! [$self findWC $group 1] } {
                                        # puts "failed findWC $group.. XXX"   
                                        return 0
                                }
                                $self send-join "WC" 0 $group
                                set ent $MRTArray($group)
                                set iif [$self get-iif-label [$ent getNextHop]]
                                $ent setiif $iif
                        }
                        $self add-oif $MRTArray($group) $origin "WC"
                        return 1
                }
                "SG" {
                        # process S,G join
                        # XXX just for source specific for now .. check XXX
                        set notfound 1
                        if [$self findSG $source $group 0 [PIM set SG]] {
                                set notfound 0
                        }
                        if $notfound {
                           $self findSG $source $group 1 [PIM set SG]     
                           $self send-join "SG" $source $group
                           set ent $MRTArray($srcID:$group)
                           set iif [$self get-iif-label [$ent getNextHop]]
                           $ent setiif $iif
                        }
                        $self add-oif $MRTArray($source:$group) $origin "SG"
                        return 1
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
        set code [lindex $msg 1]
        set source [lindex $msg 2]
        set group [lindex $msg 3]
        set origin [lindex $msg 4]
        #puts "node [$Node id] recv-prune msg $msg"
        if { [$Node id] != $dst } {
                #puts "node [$Node id] rxvd prune destined to $dst"
                # perform prune overrides here.. !
           if { [set ent [$self longest_match $source $group]] == 0 } {
                return 0
           }
           #puts "found longest match"
           set iif [$self get-iif-label $origin]
           #puts "incom i/f is $iif, ent iif [$ent getiif]"
           if { [$ent getiif] != $iif } {
               return 0
           }
           if { [$self compute-oifs $ent] == "" &&
                ![$Node check-local $group] } {
                return 0
           }
           # XXX check this further.. !! later !
           #puts "sending prune overrides.. !!"
#          set sendcode [expr {($code == "SG_RP") ? "SG" : "WC"}]
# XXX temp hack... only *,Gs..
           set sendcode "WC"
           $self send-join $sendcode $source $group
           return 1
        }
        switch $code {
                "SG_RP" {
                   set longest [$self longest_match $source $group]
                   if { $longest == 0 } {
                        #puts "Rxvd prune and no longest match !!!"
                       return 0
                   }
                   #puts "node [$Node id] got SG RP prune"
                   $self findSG $source $group 1 [PIM set RP]
                   set ent $MRTArray($source:$group)
                   $self del-oif $ent $origin "SG_RP"
                   if { [$self compute-oifs $ent] == "" &&
                                        ![$Node check-local $group] } {
                        $self send-prune "SG_RP" $source $group  
                   }
                   if { $longest != 0 } {
                        $ent setiif [$longest getiif]
                   }
                }
        }
}

PIM instproc recv-assert { msg } {
        $self instvar Node MRTArray ns
                        
        set dst [lindex $msg 0]
        set code [lindex $msg 1]
        set source [lindex $msg 2]
        set group [lindex $msg 3]
        set origin [lindex $msg 4]
                   
        #puts "node [$Node id] rxvd Assert code $code, src $source, group $group"
                        
        # XXX process asserts as in pim spec.. !
        # XXX note that the following may break when same nodes are
        # connected by multiple links & i/fs.. !XXXX 
        set link [$ns set link_([$Node id]:$origin)]
        set oifInfo [$Node get-oif $link]
        set index [lindex $oifInfo 0]
        set iif [lindex $oifInfo 1]
        # puts "_node [$Node id] got assert on iif $iif, index $index"

        # if no longest match, do nothing
        set longest [$self longest_match $source $group]
        if { $longest == 0 } { 
                #puts "_node [$Node id] got assert,.. no longest match!"
                return 0
        }
        
        set flags [$longest getflags]

        set winner -1   
        # check if iif belongs to oif
        set oifs [$self compute-oif-ids $longest]
        #puts "iif is $iif and oiflist $oifs index $index"
        if { [set idx [$self index $oifs $index]] != -1 } {
                set mycode [expr {($flags & [PIM set SG]) ? "SG" : "WC"}]
                if { $code != $mycode } {
                        if { $code == "SG" } {
                                set winner 0
                        } else {
                                set winner 1
                        }
                } else {
                   # compare addresses
                   if { [$Node id] > $origin } {
                        set winner 1
                   } else {
                        set winner 0 
                   }
                }
        
                # if I am the winner then Assert, o.w. remove oif
                if $winner {
                   # XXX this is a fix to the spec bug.. !! XXX
                   if { [$longest getType] != "SGEnt" ||
                                ! [expr $flags & [PIM set CACHE]] } {
                      #puts "_node [$Node id] won't assert, inactive matchXXX"
                      return 0
                   }
                                
                   #puts "_node [$Node id] Assert winner... asserting !"
                   $self send-assert $source $group $mycode $iif
                   return 1
                } else {
                   if { $mycode == "WC" && $code == "SG" } {
                        #puts "_node [$Node id] lost assert, creating SG_RP"
                        # create SG_RP; i.e. prune oif, and set RP bit
                        set msg [list [$Node id] "SG_RP" $source $group $origin]
                        $self recv-prune $msg
                        $MRTArray($source:$group) setflags [PIM set RP]
                   } elseif { $mycode == "WC" } {
                        #puts "code WC lost assert deleting oif"
                        $self del-oif $MRTArray($group) $origin "WC"
                   } else {
                        #puts "code SG lost assert... TBD.."
                   }
                }
        } elseif { $index == [$longest getiif] } {
                #puts "_node [$Node id] downstream router rxvd ASSERT... !!!"

               if { $code == "WC" } {
                   if [info exists MRTArray($group)] {
                        $MRTArray($group) setNextHop $origin
                        $MRTArray($group) setflags [PIM set ASSERTED]
                   }
                } else {
                        $MRTArray($source:$group) setNextHop $origin  
                        $MRTArray($source:$group) setflags [PIM set ASSERTED]   
                }
        } else {
                #puts "not upstream nor downstream.. !!"
        }
}

PIM instproc reg-stop { srcAdd group drAdd } {
	$self instvar regStopAgent
	if { ! [info exists regStopAgent] } {
		set regStopAgent [new Agent/Message/regStop $self]
	}
	$regStopAgent set dst_ $drAdd
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
                $ent setiif $regVifNum
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

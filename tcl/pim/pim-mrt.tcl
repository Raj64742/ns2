 #
 # tcl/pim/pim-mrt.tcl
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
############################### Class Ent ##########################

Class Ent

Ent instproc init {} {   
        $self instvar next_hop flags
        set next_hop -1
        set flags 0
	set map ""
}

Ent instproc setflags { flgs } {
        $self instvar flags
        set flags [expr $flgs | $flags]
        return $flags
}

Ent instproc getflags {} {
        $self instvar flags
        return $flags   
}

Ent instproc setNextHop { nxt } {
	$self instvar next_hop
	set next_hop $nxt
}

Ent instproc getNextHop {} {
	$self instvar next_hop
	return $next_hop
}

Ent instproc setiif { iface } {
        $self instvar iif
        # this is an interface label
        set iif $iface
}

Ent instproc getiif {} {
        $self instvar iif
        if [info exists iif] {
                return $iif
        }
        return -2
}
        
Ent instproc getType {} {
        return [$self info class]
}

Ent instproc add-oif { index oif } {
	$self instvar oifArray
	set oifArray($index) $oif
}

Ent instproc del-oif { index oif } {
	$self instvar pruneArray
	set pruneArray($index) $oif
	if [info exists oifArray($index)] {
		unset oifArray($index)
	}
}

Ent instproc rem-oif { index oif } {
        $self instvar oifArray
        if [info exists oifArray($index)] {
                unset oifArray($index)
        }
}

Ent instproc get-prunelist { } {
        $self instvar pruneArray
        set prunelist ""
        foreach oif [array names pruneArray] { 
                lappend prunelist $pruneArray($oif)
        }
        return $prunelist
}

Ent instproc is-pruned { index } {
	$self instvar pruneArray
	return [info exists pruneArray($index)]
}

Ent instproc remove-prune { index } {
	$self instvar pruneArray
	if [info exists pruneArray($index)] {
		unset pruneArray($index)
	}
}

Ent instproc get-oiflist { } {
	$self instvar oifArray
	set oiflist ""
	foreach oif [array names oifArray] {
		lappend oiflist $oifArray($oif)
	}
	return $oiflist
}

Ent instproc is-in-oiflist { index } {
	$self instvar oifArray
	return [info exists oifArray($index)]
}

Ent instproc setMap { mp } {
        $self instvar map
        set map $mp
}

Ent instproc getMap {} {
        $self instvar map
        return $map   
}
                
Ent instproc getRP {} {
        $self instvar map
        return [lindex $map 0]
}       

Ent instproc getHashValue {} {
        $self instvar map
        return [lindex $map 1]
}

################################## Class WCEnt #######################

Class WCEnt -superclass Ent

WCEnt instproc init { grp } {
	$self next
	$self instvar group
	set group $grp
}

WCEnt instproc get-group {} {
	$self instvar group
	return $group
}

WCEnt instproc nexthop {} {
	$self instvar next_hop
	if $next_hop { return $next_hop }
	
}

########################### Class SGEnt #######################

Class SGEnt -superclass Ent

SGEnt instproc init { src grp } {
        $self next
	$self instvar flags source group
	set source $src 
	set group $grp
}

SGEnt instproc get-group {} {
	$self instvar group
	return $group
}

SGEnt instproc setRegAgent { agent } {
	$self instvar RegAgent
	set RegAgent $agent
}

SGEnt instproc getRegAgent { } {
	$self instvar RegAgent
	return $RegAgent
}

SGEnt instproc clearflags { flgs } {
	$self instproc flags
	set flags [expr ~flgs & flags]
}

############################## MRT manipulations ######################

PIM instproc mcastGrp group {
	if [expr $group & 0x8000] {
		return 1
	}
	return 0
}

PIM instproc findWC { group create } {
	$self instvar MRTArray
	if { ! [$self mcastGrp $group] } {
		return 0
	}
	if [info exists MRTArray($group)] {
		return 1
	}
	if { ! $create } {
		return 0 
	}
	return [$self createWC $group]
}

PIM instproc createWC { group } {
	set map [$self getMap $group]
	if { $map == "" } { return 0 }
	$self instvar MRTArray
	set newWC [new WCEnt $group]
	$newWC setMap $map
	set MRTArray($group) $newWC
	return 1
}

PIM instproc getMap { group } {
	set maxHash 0
	$self instvar RPSet
	if { $RPSet == "" } { return "" }
	set hashedRP [lindex $RPSet 0]
	foreach RP $RPSet {
		set Hash [$self hash $RP $group]
		if { $Hash > $maxHash } { 
			set maxHash $Hash
			set hashedRP $RP
		}
	}
	return [list $hashedRP $maxHash]
}

PIM instproc hash { RP group } {
	# XXX hack.. 
	return $RP
}

PIM instproc findSG { source group create flags } {
	$self instvar MRTArray
	if [info exists MRTArray($source:$group)] {
		return 1
	}
	# may want to check the flags.. later.. !!
	if { ! $create } { return 0 }
	return [$self createSG $source $group $flags]
}

PIM instproc add-entry { source group } {
	$self instvar sourceArray groupArray
	lappend sourceArray($source) $group
	lappend groupArray($group) $source
}

PIM instproc createSG { source group flags } {
	$self instvar MRTArray Node ns
	# if REG entry then should get the RP add also
	$self add-entry $source $group
	if { $flags & [PIM set REG] } {
		if [info exists MRTArray($group)] {
                   set map [$MRTArray($group) getMap]
		   $self send-prune "SG_RP" $source $group
		} else {
		   set map [$self getMap $group]
                   if { $map == "" } {
                        puts "_node [$Node id] obj $self, err: can't get RP"
                        return 0
                   }
		}
                set RP [lindex $map 0]
		# create a register agent
		set regAgent [new Agent/Message/reg $source $group $RP $self]
		$Node attach $regAgent
		$regAgent set dst_ [expr $RP << 8 | [[[$ns getPIMProto $RP] set messager] port]]
	} elseif { $flags & [PIM set IIF_REG] } {
		# XXX to be done
		# establish a reg-stop rate limiting timer
		# establish a reg counter
	}
	set newSG [new SGEnt $source $group]
	$newSG setflags	$flags
	if { $flags & [PIM set REG] } { 
                $newSG setMap $map
		$newSG setRegAgent $regAgent
	}
	set MRTArray($source:$group) $newSG
}

PIM instproc longest_match  { source group } {
        $self instvar MRTArray
        if [info exists MRTArray($source:$group)] {
                return $MRTArray($source:$group)
        }
        if [info exists MRTArray($group)] { 
                return $MRTArray($group)
        }
        set RP [lindex [$self getMap $group] 0]
        if [info exists MRTArray($RP)] {
                return $MRTArray($RP)
        }
        return 0
}

PIM instproc local_address { id } {
	$self instvar Node
	if { [$Node id] == $id } {
		return 1
	}
	return 0
}

 #
 # tcl/pim/pim-mfc.tcl
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
PIM instproc change-cache { srcID group code iif } {
        $self instvar Node MRTArray messager ns
        set oiflist [$self compute-oifs $MRTArray($srcID:$group)]
        # exclude oifs leading back to the source
        if { [lsearch -exact [$Node set neighbor_] [$ns set Node_($srcID)]] != -1 } {
                set oifInfo [$Node get-oif [$ns set link_([$Node id]:$srcID)]]
                set oif [lindex $oifInfo 1]
                set oiflist [$self except-prunes $oiflist $oif]
        }
	switch $code {
           "REG" {
                #install the agent in the oiflist... as in reg_vif
                
                lappend oiflist [$MRTArray($srcID:$group) getRegAgent]
                # XXX hack to make this work on lans... cuz we don't do
                # igmp and interface routing... assume the src is single
                # homed, and forward to the first oif if lan.. !!
                set nbrs [$Node set neighbor_]
                set firstNbr [lindex $nbrs 0]
                set link [$ns set link_([$Node id]:[$firstNbr id])]
                if { [$link info class] == "DummyLink" } {
                   set alloifs [$Node get-oifs]
                   if { [llength alloifs] > 1 } {
                        puts "***WARNING: multihomed host"
                   }
                   lappend oiflist [lindex [$Node get-oif $link] 1]
                }
           }
           "SG" {
           }
           # check other codes.. etc.. !!
        }
        $self add-cache $srcID $group
        $Node add-mfc $srcID $group $iif $oiflist
}

PIM instproc add-cache { source group } {
        $self instvar MRTArray cacheByGroup
        $MRTArray($source:$group) setflags [PIM set CACHE]
        if [info exists cacheByGroup($group)] { 
                if { [lsearch -exact $cacheByGroup($group) $source] != -1 } {
                        return 0
                }  
        }
        lappend cacheByGroup($group) $source
}

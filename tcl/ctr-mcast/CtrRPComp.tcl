 #
 # tcl/ctr-mcast/CtrRPComp.tcl
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

########## CtrRPComp Class ##################################
Class CtrRPComp

CtrRPComp instproc init sim {
    $self instvar ns

    set ns $sim
}
    
CtrRPComp instproc compute-rpset {} {
    $self instvar ns

#    set Node [$ns set Node_]
#    set routingTable [$ns set routingTable_]

    ### initialization
    set ldomain ""
    set i 0
    set n [Node set nn_]
    while { $i < $n } {
        set connected($i) 0
        incr i
    }

    ### connected region algorithm
    set i 0
    set n [Node set nn_]
    while { $i < $n } {
        set n1 [$ns set Node_($i)]

        set j 0
        set m [llength $ldomain]
        while {$j < $m} {
            set lvertix [lindex $ldomain $j]
            set vertix [lindex $lvertix 0]
            if {[[$ns set routingTable_] lookup [$n1 id] [$vertix id]] >= 0} {
                lappend lvertix $n1
                set ldomain [lreplace $ldomain $j $j $lvertix]
                set connected($i) 1
		#puts "connected to [$vertix id] [$n1 id]"
                break
	    }
            incr j
        }

        if {!$connected($i)} {
            set lvertix ""
            lappend lvertix $n1
            set connected($i) 1
	    #puts "new region [$n1 id]"
            lappend ldomain $lvertix
	}

        incr i
    }
    

    ### for each region, set rpset
    foreach lvertix $ldomain {
        set hasbsr 0
        set rpset ""

	### find rpset for the region
        foreach vertix $lvertix {
	    set arbiter [$vertix getArbiter]
	    set ctrdm [$arbiter getType "CtrMcast"]
            if [$ctrdm set c_bsr] {set hasbsr 1}
            if [$ctrdm set c_rp] {lappend rpset [$vertix id]}
        }

	### if exist bsr, install rpset
        if [set hasbsr] {
            foreach vertix $lvertix { 
		set arbiter [$vertix getArbiter]
		set ctrdm [$arbiter getType "CtrMcast"]
		$ctrdm set rpset $rpset
	    }
        } else {
            foreach vertix $lvertix { 
		set arbiter [$vertix getArbiter]
		set ctrdm [$arbiter getType "CtrMcast"]
                $ctrdm set rpset "" 
                puts "no c_bsr"
            }
        }
    }
}

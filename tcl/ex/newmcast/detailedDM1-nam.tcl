 #
 # tcl/ex/newmcast/detailedDM1-nam.tcl
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
proc nam_config {net} {
        $net node 0 circle
        $net node 1 circle
        $net node 2 circle
        $net node 3 circle
        $net node 4 circle
        $net node 5 circle
        

        mklink $net 0 1 1.5Mb 10ms left-down
        mklink $net 0 2 1.5Mb 10ms down-right
        mklink $net 1 3 1.5Mb 10ms left-down
        mklink $net 1 4 1.5Mb 10ms right-down
        mklink $net 2 4 1.5Mb 10ms left-down
        mklink $net 2 5 1.5Mb 10ms right-down

        #$net queue 0 1 0.5
        #$net queue 1 0 0.5
	#$net queue 3 1 0.5

	$net color 2 black
	$net color 1 blue
        $net color 0 yellow
	# prune/graft packets
        $net color 30 purple
        $net color 31 green

        # chart utilization from 1 to 2 width 5sec
        # chart avgutilization from 1 to 2 width 5sec
}
proc link-up {src dst} {
        global sim_annotation
        set sim_annotation "link-up $src $dst"
	ecolor $src $dst green
}

proc link-down {src dst} {
        global sim_annotation
        set sim_annotation "link-down $src $dst"
	ecolor $src $dst red
}

proc node-up src {
	ncolor $src green
}

proc node-down src {
	ncolor $src red
}


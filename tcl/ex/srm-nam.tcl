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

#
# Maintainer: Kannan Varadhan <kannan@isi.edu>
# Version Date: $Date: 1997/10/23 20:53:33 $
#
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/ex/Attic/srm-nam.tcl,v 1.6 1997/10/23 20:53:33 kannan Exp $ (USC/ISI)
#

proc nam_config {net} {
        $net node 0 other
        $net node 1 circle
        $net node 2 circle
        $net node 3 circle

        mklink $net 0 1 1.5Mb 10ms left
        mklink $net 0 2 1.5Mb 10ms right-up
        mklink $net 0 3 1.5Mb 10ms right-down

#        $net queue 2 3 0.5
#        $net queue 3 2 0.5

	$net color 0 white		;# data source
	$net color 40 blue		;# session
	$net color 41 red		;# request
	$net color 42 green		;# repair
        $net color 1 Khaki		;# source node
        $net color 2 goldenrod
        $net color 3 sienna
        $net color 4 HotPink
        $net color 5 maroon
        $net color 6 orchid
        $net color 7 purple
        $net color 8 snow4
        $net color 9 PeachPuff1

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

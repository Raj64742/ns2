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
# Version Date: $Date: 1997/10/23 20:53:27 $
#
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/ex/Attic/srm-adapt-rep-nam.tcl,v 1.3 1997/10/23 20:53:27 kannan Exp $ (USC/ISI)
#

proc nam_config {net} {
    $net node 0 other
    $net node 1 box
    $net node 2 circle
    $net node 3 circle
    $net node 4 circle
    $net node 5 circle
    $net node 6 circle
    $net node 7 circle
    $net node 8 circle
    
    mklink $net 1 0 1.5Mb 10ms right
    mklink $net 2 0 1.5Mb 10ms right-down
    mklink $net 3 0 1.5Mb 10ms up
    mklink $net 4 0 1.5Mb 10ms left-down
    mklink $net 5 0 1.5Mb 10ms left
    mklink $net 6 0 1.5Mb 10ms left-up
    mklink $net 7 0 1.5Mb 10ms down
    mklink $net 8 0 1.5Mb 10ms right-up

    $net queue 1 0 0

    $net color 0 white			;# data packets
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
	ecolor $src $dst green
}

proc link-down {src dst} {
	ecolor $src $dst red
}

proc node-up src {
	ncolor $src green
}

proc node-down src {
	ncolor $src red
}


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
#

proc makelinks { net bw delay pairs } {
	global ns node
	foreach p $pairs {
		set src [lindex $p 0]
		set dst [lindex $p 1]
		set angle [lindex $p 2]

	        mklink $net $src $dst $bw $delay $angle
	}
}

proc nam_config {net} {
	foreach k "0 1 2 3 4 5 6 7 8 13 14" {
	        $net node $k circle
	}
	foreach k "9 10 11 12" {
	        $net node $k square
	}

	makelinks $net 1.5Mb 10ms {
		{ 9 0 right-up }
		{ 9 1 right }
		{ 9 2 right-down }
		{ 10 3 right-up }
		{ 10 4 right }
		{ 10 5 right-down }
		{ 11 6 right-up }
		{ 11 7 right }
		{ 11 8 right-down }
	}

	makelinks $net 1.5Mb 40ms {
		{ 12 9 right-up }
		{ 12 10 right }
		{ 12 11 right-down }
	}

	makelinks $net 1.5Mb 10ms {
		{ 13 12 down } 
	}

	makelinks $net 1.5Mb 50ms {
		{ 14 12 right }
	}

        $net queue 14 12 0.5
        $net queue 12 14 0.5
        $net queue 10 3 0.5

        $net color 1 red
        $net color 2 white
        $net color 3 blue
        $net color 4 yellow
        $net color 5 LightBlue

        # chart utilization from 1 to 2 width 5sec
        # chart avgutilization from 1 to 2 width 5sec
}

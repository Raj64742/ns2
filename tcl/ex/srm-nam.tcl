# This is a example file for C++ implementation of srm
# This is a example file for C++ implementation of srm, simple-srm.tcl

proc nam_config {net} {
        $net node 0 circle
        $net node 1 circle
        $net node 2 circle
        $net node 3 circle

        mklink $net 0 1 1.5Mb 10ms right
        mklink $net 1 2 1.5Mb 10ms right-up
        mklink $net 1 3 1.5Mb 10ms right-down


        $net color 1 red
        $net color 2 blue
        $net color 3 green
        $net color 4 yellow
	# prune/graft packets
        $net color 30 purple
        $net color 31 bisque
	
	# RTCP reports
	$net color 32 green

        # chart utilization from 1 to 2 width 5sec
        # chart avgutilization from 1 to 2 width 5sec
}

# This is a example file for C++ implementation of srm
# This is a example file for C++ implementation of srm, srm2.tcl

proc nam_config {net} {
        $net node 0 circle
        $net node 1 circle
        $net node 2 circle
        $net node 3 circle
        $net node 4 circle
        $net node 5 circle

        mklink $net 4 3 1.5Mb 10ms right
        mklink $net 3 5 1.5Mb 10ms right-up
        mklink $net 3 2 1.5Mb 10ms right-down
        mklink $net 2 1 1.5Mb 10ms right
        mklink $net 1 0 1.5Mb 10ms right

        $net queue 0 1 0.5
        $net queue 1 2 0.5
        $net queue 2 3 0.5
        $net queue 3 5 0.5
        $net queue 4 3 0.5
        $net queue 5 3 0.5


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

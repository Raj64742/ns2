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

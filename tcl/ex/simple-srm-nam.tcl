proc nam_config {net} {
        $net node 0 circle
        $net node 1 circle
        $net node 2 circle
        $net node 3 circle

        mklink $net 0 1 1.5Mb 10ms right
        mklink $net 1 2 1.5Mb 10ms right-up
        mklink $net 1 3 1.5Mb 10ms right-down

#        $net queue 2 3 0.5
#        $net queue 3 2 0.5

        $net color 0 blue
        $net color 1 red
        $net color 2 white
	$net color 3 green
	$net color 4 black

        # chart utilization from 1 to 2 width 5sec
        # chart avgutilization from 1 to 2 width 5sec
}

proc nam_config {net} {
        $net node 0 circle
        $net node 1 circle
        $net node 2 circle
        $net node 3 circle

        mklink $net 0 1 1.5Mb 10ms right
        mklink $net 1 2 1.5Mb 10ms right-up
        mklink $net 1 3 1.5Mb 10ms right-down

        $net queue 0 1 0.5
        $net queue 1 0 0.5
        $net queue 3 1 0.5

	$net color 2 black

	$net color 112 red
	$net color 106 green
	$net color 104 blue
	$net color 1 blue
        $net color 0 yellow
}










proc nam_config {net} {
        $net node 0 circle
        $net node 1 circle
        $net node 2 circle
        $net node 3 circle
        $net node 4 circle
	$net node 5 circle

	mklink $net 3 5 1.5Mb 10ms down
	mklink $net 3 2 1.5Mb 10ms right
	mklink $net 5 2 1.5Mb 10ms up-right
	mklink $net 3 4 1.5Mb 10ms up
	mklink $net 2 1 1.5Mb 10ms down
	mklink $net 5 0 1.5Mb 10ms down

        $net color 1 red
        $net color 2 black

        $net color 112 red
        $net color 106 green
        $net color 104 blue

}

proc nam_config {net} {
        $net node 0 circle
        $net node 1 circle
        $net node 2 circle
        $net node 3 circle
        $net node 4 circle
	$net node 5 circle
	$net node 6 circle
	$net node 7 circle
	$net node 8 circle
	$net node 9 circle

	mklink $net 3 2 1.5Mb 10ms down
	mklink $net 2 5 1.5Mb 10ms left
	mklink $net 5 6 1.5Mb 10ms up
	mklink $net 6 3 1.5Mb 10ms right
	mklink $net 3 5 1.5Mb 10ms down-left
	mklink $net 6 2 1.5Mb 10ms down-right
	mklink $net 3 8 1.5Mb 10ms up
	mklink $net 6 7 1.5Mb 10ms up
	mklink $net 7 8 1.5Mb 10ms right
	mklink $net 8 6 1.5Mb 10ms down-left
	mklink $net 7 3 1.5Mb 10ms down-right
	mklink $net 8 4 1.5Mb 10ms up-left
	mklink $net 4 7 1.5Mb 10ms down-left
	mklink $net 2 1 1.5Mb 10ms down
	mklink $net 5 9 1.5Mb 10ms down
	mklink $net 4 0 1.5Mb 10ms up

        $net color 1 red
        $net color 2 black

        $net color 112 red
        $net color 106 green
        $net color 104 blue

}

proc nam_config {net} {
	$net node 0 circle
	$net node 1 circle
	$net node 2 circle
	$net node 3 circle
	$net node 4 circle
	$net node 5 circle

	mklink $net 0 1 1.5Mb 10ms right
	mklink $net 0 2 10Mb 5ms left-up
	mklink $net 0 3 10Mb 5ms left
	mklink $net 0 4 10Mb 5ms left-down
	mklink $net 0 5 10Mb 5ms down

	$net queue 0 1 0.5

}


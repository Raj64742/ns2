proc nam_config {net} {

	# hosts
        $net node  0 circle
        $net node  1 circle
	$net node  2 circle
	$net node  3 circle

	# backbone
	$net node  4 square
	$net node  5 square
	$net node  6 square
	$net node  7 square

	# host-router links
        mklink $net  0  4  10Mb  2ms right-down
        mklink $net  1  4  10Mb  3ms right-up
	mklink $net  7  2  10Mb  4ms right-up
	mklink $net  7  3  10Mb  5ms right-down

	# router-router links
	mklink $net  4  5 1.5Mb 20ms right	; $net queue  4  5  -90
	mklink $net  5  6 1.5Mb 20ms right	; $net queue  5  6  -90
	mklink $net  6  7 1.5Mb 20ms right	; $net queue  6  7  -90
  
	# The fallback paths
	$net node  8 square
	mklink $net  4  8 1.5Mb 20ms up-right	; $net queue  4  8  100
	mklink $net  8  5 1.5Mb 20ms right-down

	$net node  9 square
	mklink $net  5  9 1.5Mb 20ms up-right	; $net queue  5  9  100
	mklink $net  9  6 1.5Mb 20ms right-down

	$net node 10 square
	mklink $net  6 10 1.5Mb 20ms right-up	; $net queue  6 10  100
	mklink $net 10  7 1.5Mb 20ms down-right

        $net color 0 blue
        $net color 1 red
        $net color 2 white

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

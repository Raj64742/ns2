proc nam_config {net} {
        $net node 0 circle
        $net node 1 circe
	$net node 2 square

        mklink $net 0 1 1.5Mb 10ms right
	mklink $net 0 2 5Mb 2ms right-up
	mklink $net 2 1 5Mb 2ms down-right

        $net color 0 blue
        $net color 1 red
        $net color 2 white

	$net queue 0 1 90

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

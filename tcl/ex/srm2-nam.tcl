proc nam_config {net} {
    $net node 0 box
    $net node 1 other
    $net node 2 other
    $net node 3 other
    $net node 4 circle
    $net node 5 circle
    
    mklink $net 0 1 1.5Mb 10ms right
    mklink $net 1 2 1.5Mb 10ms right
    mklink $net 2 3 1.5Mb 10ms right
    mklink $net 3 4 1.5Mb 10ms right-up
    mklink $net 3 5 1.5Mb 10ms right-down

    $net queue 0 1 0

    $net color 0 blue
    $net color 1 red
    $net color 2 white
    $net color 3 green
    $net color 4 magenta
    $net color 5 yellow
    $net color 6 black

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

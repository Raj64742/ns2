proc nam_config {net} {
    $net node 0 other
    $net node 1 box
    $net node 2 circle
    $net node 3 circle
    $net node 4 circle
    $net node 5 circle
    $net node 6 circle
    $net node 7 circle
    $net node 8 circle
    
    mklink $net 1 0 1.5Mb 10ms right
    mklink $net 2 0 1.5Mb 10ms right-down
    mklink $net 3 0 1.5Mb 10ms up
    mklink $net 4 0 1.5Mb 10ms left-down
    mklink $net 5 0 1.5Mb 10ms left
    mklink $net 6 0 1.5Mb 10ms left-up
    mklink $net 7 0 1.5Mb 10ms down
    mklink $net 8 0 1.5Mb 10ms right-up

    $net queue 1 0 0

    $net color 0 white			;# data packets
    $net color 1 blue			;# source control packets
    $net color 2 chocolate
    $net color 3 red
    $net color 4 brown
    $net color 5 tan
    $net color 6 gold
    $net color 7 salmon
    $net color 8 maroon
    $net color 9 sienna

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


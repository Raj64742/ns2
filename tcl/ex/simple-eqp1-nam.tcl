proc nam_config {net} {
    $net node 0 circle
    $net node 1 circle
    $net node 2 other
    $net node 3 other
    $net node 4 box

    mklink $net 0 2  10Mb  2ms right-down
    mklink $net 1 2  10Mb  2ms right-up

    mklink $net 2 3 1.5Mb 10ms right-up
    mklink $net 3 4 1.5Mb 10ms right-down
    mklink $net 2 4 1.5Mb 10ms right

    $net color 0 blue
    $net color 1 red
    $net color 2 white

    $net queue 2 3 0
    $net queue 2 4 0

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

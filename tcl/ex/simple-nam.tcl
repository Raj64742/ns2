proc nam_config {net} {
        $net node 0 circle
        $net node 1 circe
        $net node 2 circe
        $net node 3 circe

        mklink $net 0 2 1.5Mb 10ms right
        mklink $net 2 3 1.5Mb 10ms right
#        mklink $net 0 1 1.5Mb 10ms right-up
        mklink $net 1 2 1.5Mb 10ms right-down

        $net queue 0 2 0.5
        $net queue 2 0 0.5

        $net color 1 red
        $net color 2 white

        # chart utilization from 1 to 2 width 5sec
        # chart avgutilization from 1 to 2 width 5sec
}

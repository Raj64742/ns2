proc nam_config {net} {
	set nmax 5
        $net node 0 other
	for {set i 0} {$i <= $nmax} {incr i} {
		$net node $i circle
	}

	set chainMax [expr $nmax - 2]
	set j 0
	for {set i 1} {$i <= $chainMax} {incr i} {
		mklink $net $j $i 1.5Mb 10ms right
		set j $i
	}
	mklink $net $chainMax [expr $nmax - 1]	1.5Mb 10ms right-down
	mklink $net $chainMax $nmax		1.5Mb 10ms right-up

#        $net queue 2 3 0.5
#        $net queue 3 2 0.5

	$net color 0 white		;# data source
	$net color 40 blue		;# session
	$net color 41 red		;# request
	$net color 42 green		;# repair
        $net color 1 Khaki		;# source node
        $net color 2 goldenrod
        $net color 3 sienna
        $net color 4 HotPink
        $net color 5 maroon
        $net color 6 orchid
        $net color 7 purple
        $net color 8 snow4
        $net color 9 PeachPuff1

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


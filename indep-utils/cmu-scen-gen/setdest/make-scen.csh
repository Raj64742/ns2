#!/bin/csh

unset noclobber

set outdir = bump-scen

set maxspeed = 1
set numnodes = 50
set maxx = 1500
set maxy = 300
set simtime = 900

foreach pt (5 10 15 20 25 30 40 50)
foreach scen ( 1 2 3 4 5)
echo scen $scen : setdest -n $numnodes -p $pt -s $maxspeed -t $simtime \
      -x $maxx -y $maxy
time ./setdest -n $numnodes -p $pt -s $maxspeed -t $simtime \
      -x $maxx -y $maxy \
      >$outdir/scen-${maxx}x${maxy}-${numnodes}-${pt}-${maxspeed}-${scen}
echo
end
end

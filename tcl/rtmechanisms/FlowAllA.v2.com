# To run this:  "csh FlowAllA.v2.com".
#
# This uses: Flow4A.v2.tcl, FlowsA.v2.tcl, Setred.v2.tcl
set randomseed=12345
set filename=fairflow.xgr
# With RED: 
../../ns Flow4A.v2.tcl unforced $randomseed
sleep 5
../../ns Flow4A.v2.tcl unforced1 $randomseed
sleep 5
../../ns Flow4A.v2.tcl forced $randomseed
sleep 5
../../ns Flow4A.v2.tcl forced1 $randomseed
sleep 5
../../ns Flow4A.v2.tcl combined $randomseed
# With Drop-Tail: 
##../../ns Flow4A.v2.tcl droptail1 $randomseed
##../../ns Flow4A.v2.tcl droptail2 $randomseed
## Drop-tail queues in bytes not implemented yet in ns-2.
##../../ns Flow4A.v2.tcl droptail3 $randomseed
##../../ns Flow4A.v2.tcl droptail4 $randomseed
## To make S graphs:
#
#csh Diagonal1.com $filename.u 4 Packets Random
#csh Diagonal1.com $filename.u1 5 Bytes Random
#csh Diagonal1.com $filename.f 6 Bytes Forced
#csh Diagonal1.com $filename.f1 7 Packets Forced
#csh Diagonal1.com $filename.c 8 Combined All
#
##csh Diagonal2.com $filename.d1 10 Bytes Packets 
##csh Diagonal2.com $filename.d2 11 Packets Packets
##csh Diagonal2.com $filename.d3 12 Bytes Bytes
##csh Diagonal2.com $filename.d4 13 Packets Bytes
##
## To make S-graphs for the combined flow:
#
#csh Diagonal3.com $filename.c.4 1
#csh Time.com fairflow.rpt.sec 1
#csh Drops.com fairflow.rpt.drop fairflow.rpt.forced 1
#csh DropRatio.com fairflow2.xgr 1
#csh ArrivalRate.com $filename.c.5 1
#
##cp *.1.ps ~/paper/red/penalty/figures


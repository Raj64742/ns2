# To run this:  "csh FlowAll.v2.com".
#
set randomseed=12345
set filename=fairflow.xgr
# With RED: 
../../ns Flow4.v2.tcl unforced $randomseed
../../ns Flow4.v2.tcl unforced1 $randomseed
../../ns Flow4.v2.tcl forced $randomseed
../../ns Flow4.v2.tcl forced1 $randomseed
../../ns Flow4.v2.tcl combined $randomseed
# With Drop-Tail: 
../../ns Flow4.v2.tcl droptail1 $randomseed
../../ns Flow4.v2.tcl droptail2 $randomseed
## Drop-tail queues in bytes not implemented yet in ns-2.
##../../ns Flow4.v2.tcl droptail3 $randomseed
##../../ns Flow4.v2.tcl droptail4 $randomseed
## To make S graphs:
csh Diagonal1.com $filename.u 4 Packets Random
csh Diagonal1.com $filename.u1 5 Bytes Random
csh Diagonal1.com $filename.f 6 Bytes Forced
csh Diagonal1.com $filename.f1 7 Packets Forced
csh Diagonal1.com $filename.c 8 Combined All
csh Diagonal2.com $filename.d1 10 Bytes Packets 
csh Diagonal2.com $filename.d2 11 Packets Packets
csh Diagonal2.com $filename.d3 12 Bytes Bytes
csh Diagonal2.com $filename.d4 13 Packets Bytes

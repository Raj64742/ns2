#test-alloc-address.tcl #

set ns [new Simulator]

Simulator set EnableMcast_ 1
Simulator set EnableHierRt_ 1
# 3 possible address settings

$ns set-address 
#$ns set-hieraddress 3 4 3 15
#$ns set-portaddress 20

set pm [AddrParams instance]
set nodeshift [$pm set NodeShift_(1)]
puts "nodeshift(1) = $nodeshift"

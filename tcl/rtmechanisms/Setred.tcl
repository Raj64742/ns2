#
# change default RED parameters
#
proc set_Red { node1 node2 } {
    set_Red_Oneway $node1 $node2
    set_Red_Oneway $node2 $node1
}

proc set_Red_Oneway { node1 node2 } {
    global ns
    [[$ns link $node1 $node2] queue] set mean_pktsize_ 1000
    [[$ns link $node1 $node2] queue] set bytes_ true
    [[$ns link $node1 $node2] queue] set wait_ false
    [[$ns link $node1 $node2] queue] set maxthresh_ 20
}


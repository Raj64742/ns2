#
# Copyright (C) 2001 by USC/ISI
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 

#
# Maintainer: Kun-chan Lan <kclan@isi.edu>
# Version Date: $Date: 2001/10/04 22:37:20 $
#
#
# Unbalanced dumbell topology
# Used by isiweb.tcl
#        clients      servers
#
#       s+11 s+10          2  3
#          \ |             | / 
#            1 ---7---X--- 0 -- 6-[other clients]
#          / | [isi-www]   |\
#        s+8 s+9           4 5 
#                  
#               
#

proc my-duplex-link {ns n1 n2 bw delay queue_method queue_length} {

       $ns duplex-link $n1 $n2 $bw $delay $queue_method
       $ns queue-limit $n1 $n2 $queue_length
       $ns queue-limit $n2 $n1 $queue_length
}

proc create_topology {} {
global ns n verbose num_node other_client lan_server 
set num_server 40
set num_client 1200
set lan_client 200
set lan_server 1
set queue_method RED
set queue_method DropTail
set queue_length 50
set other_client [expr $num_client - $lan_client]
set num_node [expr 13 + [expr $num_client + $num_server]]


set delay [new RandomVariable/Empirical]
$delay loadCDF cdf/2pm.delay.cdf

if {$verbose} { puts "Creating $num_server server, $num_client client dumbell topology..." }

for {set i 0} {$i < $num_node} {incr i} {
	set n($i) [$ns node]
}

# EDGES (from-node to-node length a b):
# node 0 connects to the non-isi server and client pools
# node 7 is the isi web server
# node 1 connects to the isi client pool
my-duplex-link $ns $n(0) $n([expr $num_node - 1]) 100Mb 1ms $queue_method $queue_length
my-duplex-link $ns $n([expr $num_node - 1]) $n(7) 100Mb 1ms $queue_method $queue_length
my-duplex-link $ns $n(7) $n(1) 100Mb 0.5ms $queue_method $queue_length

my-duplex-link $ns $n(0) $n(2) 10Mb 2ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(3) 10Mb 2ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(4) 10Mb 2ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(5) 10Mb 2ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(6) 1Mb 10ms $queue_method $queue_length

my-duplex-link $ns $n(1) $n([expr $num_server + $other_client + 8]) 100Mb 500us $queue_method $queue_length
my-duplex-link $ns $n(1) $n([expr $num_server + $other_client + 9]) 100Mb 500us $queue_method $queue_length
my-duplex-link $ns $n(1) $n([expr $num_server + $other_client + 10]) 100Mb 500us $queue_method $queue_length
my-duplex-link $ns $n(1) $n([expr $num_server + $other_client + 11]) 100Mb 500us $queue_method $queue_length

for {set i 0} {$i < $num_server} {incr i} {
    set base [expr $i / 10]
    set linkD [$delay value]
    set bandwidth [uniform 20 30]
    my-duplex-link $ns $n([expr $base + 2]) $n([expr $i + 8]) [expr $bandwidth * 1000000] [expr $linkD * 0.001] $queue_method $queue_length
    if {$verbose} {puts "\$ns duplex-link \$n([expr $base + 2]) \$n([expr $i + 8]) [expr $bandwidth * 1000000] [expr $linkD * 0.001] $queue_method"}
}
if {$verbose} {puts "done creating server"}


for {set i 0} {$i < $other_client} {incr i} {
    set delay [uniform 10 50]
    set bandwidth [uniform 0.1 0.9]
    my-duplex-link $ns $n(6) $n([expr $i + $num_server + 8]) [expr $bandwidth * 1000000 ] [expr $delay * 0.001] $queue_method $queue_length
    if {$verbose} {puts "\$ns duplex-link \$n(6) \$n([expr $i + $num_server + 8]) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method"}
}
if {$verbose} {puts "done other clients"}



for {set i 0} {$i < $lan_client} {incr i} {
    set base [expr $i / 10]
    set delay [uniform 0.5 1.0]
    set bandwidth 10.0
    $ns duplex-link $n([expr $base + [expr $num_server + $other_client + 8]]) $n([expr [expr $i + $num_server + $other_client] + 12]) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method
    if {$verbose} {puts "\$ns duplex-link \$n([expr $base + [expr $num_server + $other_client + 8]]) \$n([expr [expr $i + $num_server + $other_client]  + 12]) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method"}
}
if {$verbose} {puts "done creating isi clients"}


$ns set dst_ "";  #define list of web servers
for {set i 0} {$i <= $num_server} {incr i} {
    $ns set dst_ "[$ns set dst_] [list [expr $i + 7]]"
}
if {$verbose} {puts "server set: [$ns set dst_]"}

$ns set src_ "";  #define list of web clients 
for {set i 0} {$i < $other_client} {incr i} {
    $ns set src_ "[$ns set src_] [list [expr [expr $i + $num_server] + 8]]"
}
for {set i 0} {$i < $lan_client} {incr i} {
    $ns set src_ "[$ns set src_] [list [expr [expr $i + $num_server + $other_client] + 12]]"
}
if {$verbose} {puts "client set: [$ns set src_]"}


if {$verbose} { puts "Finished creating topology..." }
}
# end of create_topology


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
# Version Date: $Date: 2001/11/28 18:47:54 $
#
#
# Unbalanced dumbell topology
# Used by isigen.tcl
#
#         ISI hosts      remote hosts
#
#      [ISI clients]
#                        
#       s+11 s+10                2 3 4 [2,3,4,5: web servers]
#          \ |                    \|/ 
#            1 ---9------------X---0 --6-[web clients]
#          / |  [isi svr]         /|\
#        s+8 s+9                 5 7 8
#                                  |  \
#                                  |  [ftp clients]
#                               [ftp server]
#

proc my-duplex-link {ns n1 n2 bw delay queue_method queue_length} {

       $ns duplex-link $n1 $n2 $bw $delay $queue_method
#       [$n1 get-module "Manual"] add-route-to-adj-node -default $n2
#       [$n2 get-module "Manual"] add-route-to-adj-node -default $n1
       $ns queue-limit $n1 $n2 $queue_length
       $ns queue-limit $n2 $n1 $queue_length
}

proc create_topology {} {
global ns n verbose num_node num_nonisi_web_client num_isi_server num_ftp_client

set num_web_server 40        
set num_web_client 960      
set num_isi_client 160     
set num_isi_server 1           
set num_ftp_server 10    
set num_ftp_client 100     
set queue_method RED
set queue_method DropTail
set queue_length 50
set num_nonisi_web_client [expr $num_web_client - $num_isi_client]
set num_node [expr 15 + [expr $num_web_client + $num_web_server + $num_ftp_server + $num_ftp_client]]

set wwwInDelay [new RandomVariable/Empirical]
$wwwInDelay loadCDF cdf/www.inbound.delay.cdf
set wwwOutDelay [new RandomVariable/Empirical]
$wwwOutDelay loadCDF cdf/www.outbound.delay.cdf
set WWWinBW [new RandomVariable/Empirical]
$WWWinBW loadCDF cdf/www.inbound.BW.cdf
set WWWoutBW [new RandomVariable/Empirical]
$WWWoutBW loadCDF cdf/www.outbound.BW.cdf

set ftpInDelay [new RandomVariable/Empirical]
$ftpInDelay loadCDF cdf/ftp.inbound.delay.cdf
set ftpOutDelay [new RandomVariable/Empirical]
$ftpOutDelay loadCDF cdf/ftp.outbound.delay.cdf
set FTPinBW [new RandomVariable/Empirical]
$FTPinBW loadCDF cdf/ftp.inbound.BW.cdf
set FTPoutBW [new RandomVariable/Empirical]
$FTPoutBW loadCDF cdf/ftp.outbound.BW.cdf

if {$verbose} { puts "Creating dumbell topology..." }

for {set i 0} {$i < $num_node} {incr i} {
	set n($i) [$ns node]
}

# EDGES (from-node to-node length a b):
# node 0 connects to the non-isi server and client pools
# node 9 is the isi web/ftp server
# node 1 connects to the isi client pool
my-duplex-link $ns $n(0) $n([expr $num_node - 1]) 100Mb 1ms $queue_method $queue_length
my-duplex-link $ns $n([expr $num_node - 1]) $n(9) 100Mb 1ms $queue_method $queue_length
my-duplex-link $ns $n(9) $n(1) 100Mb 0.5ms $queue_method $queue_length

my-duplex-link $ns $n(0) $n(2) 50Mb 2ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(3) 50Mb 2ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(4) 50Mb 2ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(5) 50Mb 2ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(6) 2Mb 10ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(7) 100Mb 2ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(8) 2Mb 10ms $queue_method $queue_length

my-duplex-link $ns $n(1) $n([expr $num_web_server + $num_nonisi_web_client + $num_ftp_server + $num_ftp_client + 10]) 100Mb 500us $queue_method $queue_length
my-duplex-link $ns $n(1) $n([expr $num_web_server + $num_nonisi_web_client + $num_ftp_server + $num_ftp_client + 11]) 100Mb 500us $queue_method $queue_length
my-duplex-link $ns $n(1) $n([expr $num_web_server + $num_nonisi_web_client + $num_ftp_server + $num_ftp_client + 12]) 100Mb 500us $queue_method $queue_length
my-duplex-link $ns $n(1) $n([expr $num_web_server + $num_nonisi_web_client + $num_ftp_server + $num_ftp_client + 13]) 100Mb 500us $queue_method $queue_length

for {set i 0} {$i < $num_web_server} {incr i} {
    set base [expr $i / 10]
    set delay [$wwwInDelay value]
    set bandwidth [$WWWinBW value]
    my-duplex-link $ns $n([expr $base + 2]) $n([expr $i + 10]) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method $queue_length
    if {$verbose} {puts "\$ns duplex-link \$n([expr $base + 2]) \$n([expr $i + 10]) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method"}
}
if {$verbose} {puts "done creating web server"}


for {set i 0} {$i < $num_nonisi_web_client} {incr i} {
    	set delay [$wwwOutDelay value]
    	set bandwidth [$WWWoutBW value]
    	my-duplex-link $ns $n(6) $n([expr $i + $num_web_server + 10]) [expr $bandwidth * 1000000 ] [expr $delay * 0.001] $queue_method $queue_length
    	if {$verbose} {puts "\$ns duplex-link \$n(6) \$n([expr $i + $num_web_server + 10]) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method"}
}
if {$verbose} {puts "done creating non-isi web clients"}




for {set i 0} {$i < $num_ftp_server} {incr i} {
    	set delay [$ftpInDelay value]
    	set bandwidth [$FTPinBW value]
    	my-duplex-link $ns $n(7) $n([expr $i + $num_web_server + $num_nonisi_web_client + 10]) [expr $bandwidth * 1000000 ] [expr $delay * 0.001] $queue_method $queue_length
    	if {$verbose} {puts "\$ns duplex-link \$n(7) \$n([expr $i + $num_web_server + $num_nonisi_web_client + 10]) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method"}
}
if {$verbose} {puts "done creating ftp servers"}


for {set i 0} {$i < $num_ftp_client} {incr i} {
    	set delay [$ftpOutDelay value]
    	set bandwidth [$FTPoutBW value]
    	my-duplex-link $ns $n(8) $n([expr $i + $num_web_server + $num_nonisi_web_client + $num_ftp_server + 10]) [expr $bandwidth * 1000000 ] [expr $delay * 0.001] $queue_method $queue_length
    	if {$verbose} {puts "\$ns duplex-link \$n(8) \$n([expr $i + $num_web_server + $num_nonisi_web_client + $num_ftp_server + 10]) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method"}
}
if {$verbose} {puts "done creating ftp clients"}



for {set i 0} {$i < $num_isi_client} {incr i} {
    set base [expr $i / 10]
    set delay [uniform 0.5 1.0]
    set bandwidth 10.0
    my-duplex-link $ns $n([expr $base + [expr $num_web_server + $num_nonisi_web_client + $num_ftp_server + $num_ftp_client + 10]]) $n([expr [expr $i + $num_web_server + $num_nonisi_web_client + $num_ftp_server + $num_ftp_client] + 14]) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method $queue_length
    if {$verbose} {puts "\$ns duplex-link \$n([expr $base + [expr $num_web_server + $num_nonisi_web_client + $num_ftp_server + $num_ftp_client + 10]]) \$n([expr [expr $i + $num_web_server + $num_nonisi_web_client + $num_ftp_server + $num_ftp_client]  + 14]) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method"}
}
if {$verbose} {puts "done creating isi clients"}


$ns set dstW_ "";  #define list of web servers
for {set i 0} {$i <= $num_web_server} {incr i} {
    $ns set dstW_ "[$ns set dstW_] [list [expr $i + 9]]"
}
if {$verbose} {puts "WWW server set: [$ns set dstW_]"}

$ns set srcW_ "";  #define list of web clients 
for {set i 0} {$i < $num_nonisi_web_client} {incr i} {
    $ns set srcW_ "[$ns set srcW_] [list [expr [expr $i + $num_web_server ] + 10]]"
}
for {set i 0} {$i < $num_isi_client} {incr i} {
    $ns set srcW_ "[$ns set srcW_] [list [expr [expr $i + $num_web_server + $num_nonisi_web_client + $num_ftp_server + $num_ftp_client] + 14]]"
}
if {$verbose} {puts "WWW client set: [$ns set srcW_]"}




$ns set dstF_ "";  #define list of ftp servers
for {set i 0} {$i <  1} {incr i} {
    $ns set dstF_ "[$ns set dstF_] [list [expr $i + 9]]"
}
for {set i 0} {$i < $num_ftp_server} {incr i} {
    $ns set dstF_ "[$ns set dstF_] [list [expr [expr $i + $num_web_server + $num_nonisi_web_client ] + 10]]"
}
if {$verbose} {puts "FTP server set: [$ns set dstF_]"}

$ns set srcF_ "";  #define list of ftp clients 
for {set i 0} {$i < $num_ftp_client} {incr i} {
    $ns set srcF_ "[$ns set srcF_] [list [expr [expr $i + $num_web_server + $num_nonisi_web_client + $num_ftp_server ] + 10]]"
}
for {set i 0} {$i < $num_isi_client} {incr i} {
    $ns set srcF_ "[$ns set srcF_] [list [expr [expr $i + $num_web_server + $num_nonisi_web_client + $num_ftp_server + $num_ftp_client] + 14]]"
}
if {$verbose} {puts "FTP client set: [$ns set srcF_]"}






if {$verbose} { puts "Finished creating topology..." }
}
# end of create_topology


#!/usr/bin/perl -w

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
# This work is supported by DARPA through SAMAN Project
# (http://www.isi.edu/saman/), administered by the Space and Naval
# Warfare System Center San Diego under Contract No. N66001-00-C-8066
#

$c1="";
$s1="";
$j=0;

local(@datan);
local(@n);

#estimate delay and bottleneck bandwidth for inbound traffic
open(INBW,"> inbound.BW") || die("cannot open inbound.BW\n");

while (<>) {
	($client,$server,$time,$seq) = split(' ',$_);

        #estimate bottleneck bandwidth between remove servers and ISI clients
       	if (($c1 ne $client ) || ($s1 ne $server)) {
        	if ( $j gt 0 ) {
		   @datan = sort numerically @n;
		   $m=int($j/2);
#		   print INBW "$c1 $s1 $datan[$m]\n";
		   print INBW "$datan[$m]\n";
		}
		$#n=0;
		$j=0;
	} else {
	        $len = $seq - $q;
		$interval = ($time - $t) * 1000000;
		$bw =  $len / $interval;
		$n[$j]=$bw;
		$j=$j + 1;
	}
	$c1=$client;
	$s1=$server;
	$t=$time;
	$q=$seq;

}
close(INBW);

sub numerically { $a <=> $b; }

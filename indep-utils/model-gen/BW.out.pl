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

$oldc="";
$olds="";
$c="";
$s="";
$c1="";
$s1="";
$d=0;
$a=0;
$oldq=0;
$i=0;
$j=0;

local(@datan);
local(@n);
local(@dataq);
local(@q);

#estimate delay and bottleneck bandwidth for outbound traffic
open(OUTBW,"> outbound.BW") || die("cannot open outbound.BW\n");
open(OUTDELAY,"> outbound.delay") || die("cannot open outbound.delay\n");

while (<>) {
	($client,$server,$timed,$timea,$seq) = split(' ',$_);

	$gap=$seq - $oldq;
	$thresh=$d + 0.001;

        #estimate delay between ISI server and remote clients
	$delay=$timea - $timed;
       	if (($c1 ne $client ) || ($s1 ne $server)) {
        	if ( $j gt 0 ) {
		   @dataq = sort numerically @q;
		   $m=int($j/2);
#		   print OUTDELAY "$c1 $s1 $dataq[$m]\n";
		   print OUTDELAY "$dataq[$m]\n";
		}
		$#q=0;
		$q[0]=$delay;
		$j=1;
	} else {
		$q[$j]=$delay;
		$j=$j + 1;
	}
	$c1=$client;
	$s1=$server;

        #estimate bottleneck bandwidth between ISI server and remote clients
        if ( ($gap eq 1460) && ($client eq $oldc) && ($server eq $olds) && ($timed lt $thresh)) {
		$gap1=$timed - $d;
		$gap2=$timea - $a;
        	if ( $gap2 gt $gap1) {
        		if (($c ne $client) || ($s ne $server)) {
        			if ($i gt 0 ) {
		   			@datan = sort numerically @n;
				   	$m=int($i/2);
					$bw=0.01117/$datan[$m];
					if ( $bw lt 100) {
#				   		print OUTBW "$c $s  $bw\n";
				   		print OUTBW "$bw\n";
					}
				}
				$#n=0;
				$n[0]=$gap2;
				$i=1;
			} else {
				$n[$i]=$gap2;
				$i=$i + 1;
			}
			$c=$client;
			$s=$server;
		}
	}

	$oldc=$client;
	$olds=$server;
	$d=$timed;
	$a=$timea;
	$oldq=$seq;

}
close(OUTBW);
close(OUTDELAY);

sub numerically { $a <=> $b; }
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
# An tcl script that extracts DATA/ACK packets from/to ISI domain, 
# used by SAMAN ModelGen
#
# This work is supported by DARPA through SAMAN Project
# (http://www.isi.edu/saman/), administered by the Space and Naval
# Warfare System Center San Diego under Contract No. N66001-00-C-8066
#

$isiPrefix="129.1.80";

open(OUT,"> outbound.seq") || die("cannot open outbound.seq\n");
open(IN,"> inbound.seq") || die("cannot open inbound.seq\n");

while (<>) {
        ($time1,$time2,$ip11,$ip12,$ip13,$ip14,$srcPort,$dummy1,$ip21,$ip22,$ip23,$ip24,$dstPort,$dummy2,$flag,$seqb,$seqe,$size,$size1) = split(/[.:() ]/,$_);


	$sTime=join(".",$time1,$time2);
	$dummy1="";
	$dummy2="";
	$flag="";

	if ( ($seqe ne "ack") && ($seqb eq "")) {
	   $seqb=$seqe;
	   $seqe=$size;
	   $size=$size1;
	}

	$prefixc=join(".",$ip11,$ip12,$srcPort);
	$prefixs=join(".",$ip21,$ip22,$dstPort);
	$client=join(".",$ip11,$ip12,$ip13,$ip14,$srcPort);
	$server=join(".",$ip21,$ip22,$ip23,$ip24,$dstPort);

        if ( $srcPort eq "80" ) {
		$server=join(".",$ip11,$ip12,$ip13,$ip14,$srcPort);
	 	$client=join(".",$ip21,$ip22,$ip23,$ip24,$dstPort);
        }



        #data packet from ISI
	if ( $prefixc eq $isiPrefix) {

		if ( $seqe ne "ack" ) {
			if ( $size eq 1460 ) {
				print OUT "$client $server $seqe $sTime data\n"
			}	
                }
	}
	#ACK packet to ISI
	if (($prefixs eq $isiPrefix) && ($seqe eq "ack")) {   
		print OUT "$client $server $size $sTime ack\n"
	}
        #data packet to ISI
        if ( ($srcPort eq "80") && ($prefixc ne $isiPrefix)) {
		if ( $seqe ne "ack" ) {
			if ( $size eq 1460 ) {
				print IN "$client $server $sTime $seqe\n";
			}	
                }
	}

}
close(OUT);
close(IN);

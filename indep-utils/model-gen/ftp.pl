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
# An perl script that seperate inbound and outbound traffic of ISI domain, 
# used by SAMAN ModelGen
#
# This work is supported by DARPA through SAMAN Project
# (http://www.isi.edu/saman/), administered by the Space and Naval
# Warfare System Center San Diego under Contract No. N66001-00-C-8066
#

sub usage {
        print STDERR <<END;
      usage: $0  [-w FilenameExtention]
        Options:
            -w string  specify the filename extention

END
        exit 1;
}
BEGIN {
        $dblibdir = "./";
        push(@INC, $dblibdir);
}

use DbGetopt;
require "dblib.pl";
my(@orig_argv) = @ARGV;
&usage if ($#ARGV < 0);
my($prog) = &progname;
my($dbopts) = new DbGetopt("w:?", \@ARGV);
my($ch);
while ($dbopts->getopt) {
        $ch = $dbopts->opt;
        if ($ch eq 'w') {
                $fext = $dbopts->optarg;
	} else {
                &usage;
        };
};

$sarrivef=join(".",$fext,"sess.arrive");
$farrivef=join(".",$fext,"file.inter");
$sizef=join(".",$fext,"size");
$filef=join(".",$fext,"fileno");

open(FSARRIVE,"> $sarrivef") || die("cannot open $sarrivef\n");
open(FFARRIVE,"> $farrivef") || die("cannot open $farrivef\n");
open(FSIZE,"> $sizef") || die("cannot open $sizef\n");
open(FFILE,"> $filef") || die("cannot open $filef\n");


$prevS="";
$prevD="";
$prevPort="";
$total=0;
$fileno=1;

while (<>) {
        ($ip11,$ip12,$ip13,$ip14,$srcPort,$ip21,$ip22,$ip23,$ip24,$dstPort,$time1,$time2,$size) = split(/[. ]/,$_);

       
        $srcPort="";

	$src=join(".",$ip11,$ip12,$ip13,$ip14);
	$dst=join(".",$ip21,$ip22,$ip23,$ip24);
	$time=join(".",$time1,$time2);


        #only look at ftp data
        if (($src eq $prevS) && ($dst eq $prevD)) {
	   	if ($dstPort eq $prevPort) {
	   		$total=$total+$size;
		} else {
			if ($total ne 0) {
				print FSIZE "$total\n";
			}
			$total=$size+0;
			$fileno=$fileno+1;
			$finter=$time-$startTime;
#			print FFARRIVE "$src $dst $dstPort $startTime $time\n";
			if ($finter gt 0) {
				print FFARRIVE "$finter\n";
			}
			$startTime=$time;
		}
	} else {
		if ($prevS ne "") {
			print FFILE "$fileno\n";
		}
		if ($total ne 0) {
			print FSIZE "$total\n";
		}
		$total=$size+0;
		$fileno=1;
		print FSARRIVE "$time\n";
		$startTime=$time;
	}
	$prevS=$src;
	$prevD=$dst;
	$prevPort=$dstPort;
}

print FFILE "$fileno\n";
if ($total ne 0) {
	print FSIZE "$total\n";
}

close(FSARRIVE);
close(FFARRIVE);
close(FSIZE);
close(FFILE);

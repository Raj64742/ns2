# -*-	Mode:Perl; tab-width:8; indent-tabs-mode:t -*- 
#
# pre-process.pl
# Copyright (C) 1999 by USC/ISI
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
# Contributed by Nader Salehi (USC/ISI), http://www.isi.edu/~salehi
# 
#!/local/bin/perl

die "usage: $0 working-directory\n" if ($#ARGV < 0);
my($working_dir) = $ARGV[0];
chdir($working_dir) || die "cannot chdir $working_dir\n";

@FILES = `ls *.tex`;

foreach $filename (@FILES) {
    chop $filename;
    &pre_process_file($filename);
}


sub pre_process_file
{
    local($filename) = @_;
    $program_env = 0;
    $change = 0;
    open(FILE, "$filename") || die "Cannot open $filename: $_";
    $outFile = "$filename.new";
    open(OUTFILE, ">$outFile") || die "Cannot create temp file $_";
    while (<FILE>) {
	if (/\\begin\s*{(program)}/) {
	    $program_env = 1;
	    $change = 1;
	    s/$1/verbatim/g;
	}			# if
	if ($program_env == 1) {
	    s/\\;/\#/g;
	    s/{\s*\\cf\s*(\#?.*)}/$1/g;
	    s/\\([{|}])/$1/g;
	    s/\\\*(.*)\*\//\/\*$1\//g;
	    if (/\\end\s*{(program)}/) {
		$program_env = 0;
		s/$1/verbatim/g;p
	    }			# if
	}			# if
	s/\\code{([^\$}]*)}/{\\ss $1}/g && ($change = 1);
	s/\\code{\s*\$([^}]*)}/{\\em $1}/g && ($change = 1);
	s/\\code{[^\w]*([^\$]*)([^}]*)}/{\\ss $1 $2}/g && s/\$/\\\$/g && ($change = 1);
#	s/\\proc\[\]{([^}]*)}/{\\ss $1 }/g && ($change = 1);
	print OUTFILE unless (/^%/);
    }				# while
    close(OUTFILE);
    close(FILE);
    if ($change == 1) {
	print "$filename has been altered!\n";
	rename($filename, "$filename.org");
	rename($outFile, $filename);
    } else {
	unlink($outFile);
    }				# else
}

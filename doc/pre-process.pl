#!/local/bin/perl

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

#!/local/bin/perl
require "ctime.pl";

%MonthTbl = ('jan', 'January', 'feb', 'February', 'mar', 'March', 'apr', 'April', 'may', 'May', 'jun', 'June', 'jul', 'July', 'aug', 'August', 'sep', 'September', 'oct', 'October', 'nov', 'November', 'dec', 'December');


&change_title();

@FILES = `ls *.html`;
foreach $filename (@FILES) {
    chop $filename
    &adjust_buttons($filename);

}				# foreach


sub adjust_buttons 
{
    local($filename) = @_;

    $change = 0;
    $outFile = "$filename.new";
    open(FILE, "$filename") || die "Cannot open $filename: $_";
    open(OUTFILE, ">$outFile") || die "Cannot open $outFile: $_";
    while (<FILE>) {
	s/(SRC\s*=\s*")[^>"].*(next_motif.+gif)/$1$2/g && ($change = 1);
	s/(SRC\s*=\s*")[^>"].*(up_motif.+gif)/$1$2/g && ($change = 1);
	s/(SRC\s*=\s*")[^>"].*(index_motif.+gif)/$1$2/g && ($change = 1);
	s/(SRC\s*=\s*")[^>"].*(contents_motif.+gif)/$1$2/g && ($chagne = 1);
	s/(SRC\s*=\s*")[^>"].*(previous_motif.+gif)/$1$2/g && ($change = 1);
	print OUTFILE;
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
}				# adjust_buttons

sub change_title {
    $Date = &ctime(time), "\n";
    $Date =~ /^(\w+)\s+(\w+)\s+(\d+)\s+(\d+):(\d+):(\d+)[^\d]*(\d+)/;
    $temp = $2;
    $temp =~ tr/A-Z/a-z/;
    $month = $MonthTbl{$temp};
    $year = $7;
    $day = $3;
    
    print "$month $day, $year\n";
    @FILES = `ls *.tex`;
    
    $change = 0;
    $filename = "index.html";
    $outFile = "$filename.new";
    $title = "<i>ns</i> Notes and Documents";
    $author = "The VINT Project<br>A collaboratoin between researchers at<br>UC Berkeley, LBL, USC/ISI, and Xerox PARC.<br>Kevin Fall, Editor<br>Kannan Varadhan, Editor";
    
    open(FILE, "$filename") || die "Cannot open $filename: $_";
    open(OUTFILE, ">$outFile") || die "Cannot create temp file $_";
    while (<FILE>) {
	s/^(<FONT SIZE="\+2">)\s*title/$1 $title/ && ($change = 1);
	s/^author(<\/TD>)/$author $1/ && ($change = 1);
	s/^(<FONT SIZE="\+1">)date/$1$month $day, $year/ && ($change = 1);
	print OUTFILE;
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
}				# change_title

proc finish file {
 
        set f [open temp.rands w]
        puts $f "TitleText: $file"
        puts $f "Device: Postscript"

        exec rm -f temp.p temp.d
        exec touch temp.d temp.p
        #
        # split queue/drop events into two separate files.
        # we don't bother checking for the link we're interested in
        # since we know only such events are in our trace file
        #
        exec awk {
                {
                        if (($1 == "+" || $1 == "-" ) && \
                            ($5 == "tcp" || $5 == "ack" || $5 == "cbr"))\
                                        print $2, 10 * $9 + ($11 % 90) * 0.01
                }
        } out.tr > temp.p
        exec awk {
                {
                        if ($1 == "d")
                                print $2, $8 + ($11 % 90) * 0.01
                }
        } out.tr > temp.d

        puts $f \"packets  
        flush $f
        exec cat temp.p >@ $f
        flush $f
        # insert dummy data sets so we get X's for marks in data-set 4
        puts $f [format "\n\"skip-1\n0 1\n\n\"skip-2\n0 1\n\n"]

        puts $f \"drops
        flush $f
        #
        # Repeat the first line twice in the drops file because
        # often we have only one drop and xgraph won't print marks
        # for data sets with only one point.
        #
        exec head -1 temp.d >@ $f
        exec cat temp.d >@ $f
        close $f
	exec rm -f temp.p temp.d
	exec rm -f out.tr
        exec xgraph -bb -tk -nl -m -x time -y packet temp.rands &

        exit 0
}

#
# Set xgraph to 1 to use xgraph, and to 0 to use S.
# Set xgraph to 2 to make S-graphs later
set xgraph 1
set flowgraphfile fairflow.xgr
set timegraphfile fairflow1.xgr
set fracgraphfile fairflow2.xgr
set friendlygraphfile fairflow3.xgr
# drop_interval gets reset in proc flowDump
set drop_interval 2.0
set pthresh 100
#-------------------------------------------------------------------

proc finish file {

	#
	# split queue/drop events into two separate files.
	# we don't bother checking for the link we're interested in
	# since we know only such events are in our trace file
	#
	set awkCode {
		{
			if (($1 == "+" || $1 == "-" ) && \
			  ($5 == "tcp" || $5 == "ack"))
				print $2, $8 + ($11 % 90) * 0.01 >> "temp.p";
			else if ($1 == "d")
				print $2, $8 + ($11 % 90) * 0.01 >> "temp.d";
		}
	}

	set f [open temp.rands w]
	puts $f "TitleText: $file"
	puts $f "Device: Postscript"
	
	exec rm -f temp.p temp.d 
	exec touch temp.d temp.p
	exec awk $awkCode out.tr

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
	exec xgraph -bb -tk -nl -m -x time -y packet temp.rands &
	
	exit 0
}

# plot queue size and average queue size
proc plotQueue { name } {

	#
	# Plot the queue size and average queue size, for RED gateways.
	#
	set awkCode {
		{
			if ($1 == "Q" && NF>2) {
				print $2, $3 >> "temp.q";
				set end $2
			}
			else if ($1 == "a" && NF>2)
				print $2, $3 >> "temp.a";
		}
	}
	set f [open temp.queue w]
	puts $f "TitleText: $name"
	puts $f "Device: Postscript"

	exec rm -f temp.q temp.a 
	exec touch temp.a temp.q
	exec awk $awkCode out.tr

	puts $f \"queue
	flush $f
	exec cat temp.q >@ $f
	flush $f
	puts $f \n\"ave_queue
	flush $f
	exec cat temp.a >@ $f
	###puts $f \n"thresh
	###puts $f 0 [[ns link $r1 $r2] get thresh]
	###puts $f $end [[ns link $r1 $r2] get thresh]
	close $f
	puts "running xgraph for queue plot..."
	exec xgraph -bb -tk -x time -y queue temp.queue &
}

# plot average queue size
proc plotAveQueue { name } {

	global xgraph
	#
	# Plot the queue size and average queue size, for RED gateways.
	#
	set awkCode {
		{
			if ($1 == "a" && NF>2)
			    print $2, $3 >> "temp.a";
		}
	}
	set f [open temp.queue w]
	puts $f "TitleText: $name"
	puts $f "Device: Postscript"

	exec rm -f temp.a 
	exec touch temp.a 
	exec awk $awkCode out.tr

	puts $f \"queue
	flush $f
	puts $f \n\"ave_queue
	flush $f
	exec cat temp.a >@ $f
	close $f
	puts "running xgraph for queue plot..."
	if { $xgraph == 1 } {
	  exec xgraph -bb -tk -x time -y queue temp.queue &
	}
	if { $xgraph == 0 } {
	  exec csh queue.com temp.queue 
	}
}

#--------------------------------------------------------------------

#
# Arrange for tcp source stats to be dumped for $tcpSrc every
# $interval seconds of simulation time
#
proc tcpDump { tcpSrc interval } {
    global ns
    proc dump { src interval } {
	ns at [expr [$ns now] + $interval] "dump $src $interval"
	puts [$ns now]/cwnd=[$src get cwnd]/ssthresh=[$src get ssthresh]/ack=[$src get ack]
    }
    $ns at 0.0 "dump $tcpSrc $interval"
}

proc openTrace { stopTime testName } {
	global ns r1 k1    
	set traceFile [open out.tr w]
	$ns at $stopTime \
		"close $traceFile ; finish $testName"
	set T [$ns trace]
	$T attach $traceFile
	return $T
}

#---------------------------------------------------------------

proc flowmonDump { fm dump link stop } {
    global ns drop_interval
    if {[$ns now] < $stop} {
   	$dump $link $fm
	set next [expr [$ns now] + $drop_interval]
	$ns at $next "flowmonDump $fm $dump $link $stop"
    }
}

proc create_flowstats { link dump stoptime } {
    
    global ns r1 r2 r1fm flowfile drop_interval flowdesc

    set r1fm [$ns makeflowmon Fid]
    set flowdesc [open $flowfile w]
    $r1fm attach $flowdesc
    $ns attach-fmon $link $r1fm 1
    $ns at $drop_interval "flowmonDump $r1fm $dump $link $stoptime"
}

proc create_flowstats1 { link dump stoptime } {

    global ns r1 r2 r1fm flowfile drop_interval flowdesc
    
    set r1fm [$ns makeflowmon Fid]
    set flowdesc [open $flowfile w]
    $r1fm attach $flowdesc
    $ns attach-fmon $link $r1fm 1
    $ns at $drop_interval "flowmonDump $r1fm $dump $link $stoptime"
}

proc create_flowstats2 { link dump stoptime } {

    global ns r1 r2 r1fm flowfile drop_interval flowdesc
    
    set r1fm [$ns makeflowmon Fid]
    set flowdesc [open $flowfile w]
    $r1fm attach $flowdesc
    $ns attach-fmon $link $r1fm 0
    $ns at $drop_interval "flowmonDump $r1fm $dump $link $stoptime"
}

#------------------------------------------------------------------

#
# awk code used to produce:
#       x axis: # arrivals for this flow+category / # total arrivals [bytes]
#       y axis: # drops for this flow+category / # drops this category [pkts]
proc unforcedmakeawk { } {
    global category
    if {[string compare $category "unforced"] == 0} {
	set awkCode {
	    BEGIN { prev=-1; print "\"flow 0"; }
	    {
		if ($5 != prev) {
		    print " "; print "\"flow " $5;
		    if ($13 > 0 && $14 > 0) {
			print 100.0 * $9/$13, 100.0 * $10 / $14;
		    }
		    prev = $5;
		}
		else if ($13 > 0 && $14 > 0) {
		    print 100.0 * $9 / $13, 100.0 * $10 / $14;
		}
	    }
	}
	return $awkCode
    } elseif {[string compare $category "forced"] == 0} {
	set awkCode {
	    BEGIN { prev=-1; print "\"flow 0" }
	    {
		if ($5 != prev) {
		    print " "; print "\"flow " $5;
		    if ($13 > 0 && ($16-$14) > 0) {
			print 100.0 * $9/$13, 100.0 * ($18-$10) / ($16-$14);
		    }
		    prev = $5;
		}
		else if ($13 > 0 && ($16-$14) > 0) {
		    print 100.0 * $9 / $13, 100.0 * ($18-$10) / ($16-$14);
		}
	    }
	}
	return $awkCode
    } else {
	puts stderr "Error: unforcedmakeawk: drop category $category unknown."
	return {}
    }
}

#
# awk code used to produce:
#       x axis: # arrivals for this flow+category / # total arrivals [bytes]
#       y axis: # drops for this flow+category / # drops this category [pkts]
# Make sure that N > 2.3 / P^2, for N = # drops this category [pkts],
#       P = y axis value
proc unforcedmakeawk1 { } {
        set awkCode {
            BEGIN { print "\"flow 0" }
            {
                if ($5 != prev) {
                        print " "; 
			print "\"flow " $5;
			drops = 0; flow_drops = 0; arrivals = 0;
			flow_arrivals = 0;
			byte_arrivals = 0; flow_byte_arrivals = 0;
		}
		drops += $14;
		flow_drops += $10;
		arrivals += $12;
		byte_arrivals += $13;
		flow_arrivals += $8;
		flow_byte_arrivals += $9;
		p = flow_arrivals/arrivals;
		if (p*p*drops >= 2.3) {
		  print 100.0 * flow_byte_arrivals/byte_arrivals, 
		    100.0 * flow_drops / drops;
		  drops = 0; flow_drops = 0; arrivals = 0;
		  flow_arrivals = 0;
		  byte_arrivals = 0; flow_byte_arrivals = 0;
		} else {
		    printf "p: %8.2f drops: %d\n", p, drops
		}
		prev = $5
            }
        }
        return $awkCode
}

#printf "prev=%d,13=%d,17=%d,15=%d\n",prev,$13,$17,$15;
#
# awk code used to produce:
#       x axis: # arrivals for this flow+category / # total arrivals [bytes]
#       y axis: # drops for this flow+category / # drops this category [bytes]
proc forcedmakeawk { } {
    global category
    if {[string compare $category "forced"] == 0} {
	set awkCode {
	    BEGIN { prev=-1; print "\"flow 0"; }
	    {
		if ($5 != prev) {
		    print " "; print "\"flow " $5;
		    if ($13 > 0 && ($17-$15) > 0) {
			print 100.0 * $9/$13, 100.0 * ($19-$11) / ($17-$15);
			prev = $5;
		    }
		}
		else if ($13 > 0 && ($17-$15) > 0) {
		    print 100.0 * $9 / $13, 100.0 * ($19-$11) / ($17-$15);
		}
	    }
	}
	return $awkCode
    } elseif {[string compare $category "unforced"] == 0} {
	set awkCode {
	    BEGIN { prev=-1; print "\"flow 0"; }
	    {
		if ($5 != prev) {
		    print " "; print "\"flow " $5;
		    if ($13 > 0 && $15 > 0) {
			print 100.0 * $9/$13, 100.0 * $11 / $15;
			prev = $5;
		    }
		}
		else if ($13 > 0 && $15 > 0) {
		    print 100.0 * $9 / $13, 100.0 * $11 / $15;
		}
	    }
	}
	return $awkCode
    } else {
	puts stderr "Error: forcedmakeawk: drop category $category unknown."
	return {}
    }
}

#
# awk code used to produce:
#      x axis: # arrivals for this flow+category / # total arrivals [bytes]
#      y axis: # drops for this flow / # drops [pkts and bytes combined]
proc allmakeawk_old { } {
    set awkCode {
	BEGIN { prev=-1; frac_bytes=0; frac_packets=0; frac_arrivals=0; cat0=0; cat1=0}
	{
	    if ($5 != prev) {
		print " "; print "\"flow "$5;
		prev = $5
	    }
	    if (cat1 + cat0 > 0) {
		if (frac_packets + frac_bytes > 0) {
		    cat1_part = frac_packets * cat1 / ( cat1 + cat0 ) 
		    cat0_part = frac_bytes * cat0 / ( cat1 + cat0 ) 
		    print 100.0 * frac_arrivals, 100.0 * ( cat1_part + cat0_part )
		}
		frac_bytes = 0; frac_packets = 0; frac_arrivals = 0;
		cat1 = 0; cat0 = 0;
		prevtime = $1
	    }
	    if ($14 > 0) {
		frac_packets = $10/$14;
	    }
	    else {
		frac_packets = 0;
	    }
	    if (($17-$15) > 0) {
		frac_bytes = ($19-$11)/($17-$15);
	    }
	    else {
		frac_bytes = 0;
	    }
	    if ($13 > 0) {
		frac_arrivals = $9/$13;
	    }
	    else {
		frac_arrivals = 0;
	    }
	    cat0 = $16-$14;
	    cat1 = $14;
	}
	END {
	    if (frac_packets + frac_bytes > 0 && cat1 + cat0 > 0) {
		cat1_part = frac_packets * cat1 / ( cat1 + cat0 ) 
		cat0_part = frac_bytes * cat0 / ( cat1 + cat0 ) 
		print 100.0 * frac_arrivals, 100.0 * ( cat1_part + cat0_part )
	    }
	}
    }
    return $awkCode
}

#
# awk code used to produce:
#      x axis: # arrivals for this flow+category / # total arrivals [bytes]
#      y axis: # drops for this flow / # drops [pkts and bytes combined]
proc allmakeawk { } {
    set awkCode {
        BEGIN {prev=-1; tot_bytes=0; tot_packets=0; forced_total=0; unforced_total=0}
        {
            if ($5 != prev) {
                print " "; print "\"flow ",$5;
                prev = $5
            }
            tot_bytes = $19-$11;
            forced_total= $16-$14;
            tot_packets = $10;
            tot_arrivals = $9;
            unforced_total = $14;
            if (unforced_total + forced_total > 0) {
                if ($14 > 0) {
                    frac_packets = tot_packets/$14;
                }
                else { frac_packets = 0;}
                if ($17-$15 > 0) {
                    frac_bytes = tot_bytes/($17-$15);
                }
                else {frac_bytes = 0;} 
                if ($13 > 0) {
                    frac_arrivals = tot_arrivals/$13;
                }
                else {frac_arrivals = 0;}
                if (frac_packets + frac_bytes > 0) {
                    unforced_total_part = frac_packets * unforced_total / ( unforced_total + forced_total)
                    forced_total_part = frac_bytes * forced_total / ( unforced_total + forced_total)
                    print 100.0 * frac_arrivals, 100.0 * ( unforced_total_part +forced_total_part)
                }
            }
        }
    }
    return $awkCode
}

#--------------------------------------------------------------

proc create_flow_graph { graphtitle graphfile awkprocedure } {
    global flowfile 
    
    exec rm -f $graphfile
    set outdesc [open $graphfile w]
    #
    # this next part is xgraph specific
    #
    puts $outdesc "TitleText: $graphtitle"
    puts $outdesc "Device: Postscript"
    
    puts "writing flow xgraph data to $graphfile..."
    
    catch {exec sort -n +1 -o $flowfile $flowfile} result
    exec awk [$awkprocedure] $flowfile >@ $outdesc
    close $outdesc
}

# plot drops vs. arrivals
proc finish_flow { name } {
	global flowgraphfile xgraph awkprocedure
	create_flow_graph $name $flowgraphfile $awkprocedure
	puts "running xgraph for comparing drops and arrivals..."
	if { $xgraph == 1 } {
	    exec xgraph -bb -tk -nl -m -lx 0,100 -ly 0,100 -x "% of data bytes" -y "% of discards" $flowgraphfile &
	} 
	if { $xgraph == 0 } {
		exec csh diagonal.com $flowgraphfile &
	}
	exit 0
}

# plot drops vs. arrivals, for unforced drops.
proc plot_dropsinpackets { name flowgraphfile } {
    global xgraph queuetype
    create_flow_graph $name $flowgraphfile unforcedmakeawk
    puts "running xgraph for comparing drops and arrivals..."
    if { $xgraph == 1 } {
	exec xgraph -bb -tk -nl -m -lx 0,100 -ly 0,100 -x "% of data bytes" -y "% of discards (in packets).  Queue in $queuetype" $flowgraphfile &
    } 
    if { $xgraph == 0 } {
	exec csh diagonal.com $flowgraphfile &
    }
    exit 0
}

# plot drops vs. arrivals, for unforced drops.
proc plot_dropsinpackets1 { name flowgraphfile } {
	global xgraph queuetype
	create_flow_graph $name $flowgraphfile unforcedmakeawk1
	puts "running xgraph for comparing drops and arrivals..."
	if { $xgraph == 1 } {
	    exec xgraph -bb -tk -nl -m -lx 0,100 -ly 0,100 -x "% of data bytes" -y "% of discards (in packets).  Queue in $queuetype" $flowgraphfile &
	} 
	if { $xgraph == 0 } {
		exec csh diagonal.com $flowgraphfile &
	}
	exit 0
}

# plot drops vs. arrivals, for forced drops.
proc plot_dropsinbytes { name flowgraphfile } {
    global xgraph queuetype
    create_flow_graph $name $flowgraphfile forcedmakeawk
    puts "running xgraph for comparing drops and arrivals..."
    if { $xgraph == 1 } {
	exec xgraph -bb -tk -nl -m -lx 0,100 -ly 0,100 -x "% of data bytes" -y "% of discards (in bytes) Queue in $queuetype" $flowgraphfile &
    } 
    if { $xgraph == 0 } {
	exec csh diagonal.com $flowgraphfile &
    }
    exit 0
}

# plot drops vs. arrivals, for combined metric drops.
proc plot_dropscombined { name flowgraphfile } {
    global xgraph 
    create_flow_graph $name $flowgraphfile allmakeawk
    puts "running xgraph for comparing drops and arrivals..."
    if { $xgraph == 1 } {
	exec xgraph -bb -tk -nl -m -lx 0,100 -ly 0,100 -x "% of data bytes" -y "% of discards (combined metric)" $flowgraphfile &
    } 
    if { $xgraph == 0 } {
	exec csh diagonal.com $flowgraphfile &
    }
    exit 0
}

#--------------------------------------------------------------------------

# awk code used to produce:
#      x axis: time
#      y axis: per-flow drop ratios
proc time_awk { } {
        set awkCode {
            BEGIN { print "\"flow 0"}
            {
                if ($1 != prevtime && prevtime > 0){
			if (cat1 + cat0 > 0) {
			  cat1_part = frac_packets * cat1 / ( cat1 + cat0 )
			  cat0_part = frac_bytes * cat0 / ( cat1 + cat0 )
                          print prevtime, 100.0 * ( cat1_part + cat0_part )
			}
			frac_bytes = 0; frac_packets = 0; 
			cat1 = 0; cat0 = 0;
			prevtime = $1
		}
		if ($5 != prev) {
			print " "; print "\"flow "prev;
			prev = $5
		}
		if ($3==0) { 
			if ($15>0) {frac_bytes = $11 / $15}
			else {frac_bytes = 0}
			cat0 = $14
		} if ($3==1) { 
			if ($14>0) {frac_packets = $10 / $14}
			else {frac_packets = 0}
			cat1 = $14
		}
		prevtime = $1
            }
	    END {
		cat1_part = frac_packets * cat1 / ( cat1 + cat0 )
		cat0_part = frac_bytes * cat0 / ( cat1 + cat0 )
                print prevtime, 100.0 * ( cat1_part + cat0_part )
	    }
        }
        return $awkCode
}

# plot time vs. per-flow drop ratio
proc create_time_graph { graphtitle graphfile } {
        global flowfile awkprocedure

        exec rm -f $graphfile
        set outdesc [open $graphfile w]
        #
        # this next part is xgraph specific
        #
        puts $outdesc "TitleText: $graphtitle"
        puts $outdesc "Device: Postscript"

        puts "writing flow xgraph data to $graphfile..."

        exec sort -n +1 -o $flowfile $flowfile
        exec awk [time_awk] $flowfile >@ $outdesc
        close $outdesc
}

# Plot per-flow bandwidth vs. time.
proc plot_dropmetric { name } {
	global timegraphfile xgraph
	create_time_graph $name $timegraphfile 
	puts "running time xgraph for plotting arrivals..."
	if { $xgraph == 1 } {
	  exec xgraph -bb -tk -m -ly 0,100 -x "time" -y "Bandwidth(%)" $timegraphfile &
	}
	if { $xgraph == 0 } {
		exec csh bandwidth.com $timegraphfile &
	}
}

#--------------------------------------------------------------------------

# awk code used to produce:
#      x axis: time
#      y axis: per-flow bytes
proc byte_awk { } {
        set awkCode {
            BEGIN { new = 1 }
            {
		class = $1;
		time = $2;
		bytes = $3;
		if (class != prev) {
			prev = class;
			if (new==1) {new=0;}
			else {print " "; }
			print "\"flow "prev;
		}
		if (bytes > oldbytes[class]) {
		  if (oldtime[class]==0) {
		  	interval = $4;
		  } else { interval = time - oldtime[class]; }
		  if (interval > 0) {
		    bitsPerSecond = 8*(bytes - oldbytes[class])/interval;
		  }
		  print time, 100*bitsPerSecond/(bandwidth*1000);
		  print time, 100*bitsPerSecond/(bandwidth*1000);
		}
	  	oldbytes[class] = bytes;
		oldtime[class] = time;
            }
        }
        return $awkCode
}

proc reclass_awk { } {
	set awkCode {
		{
		print " ";
		printf "\"%s\n", $3
		print $1, 0;
		print $1, 100;
		}
	}
}

# plot time vs. per-flow bytes
proc create_bytes_graph { graphtitle infile graphfile bandwidth } {
	global penaltyfile
        set tmpfile /tmp/fg1[pid]
	# print: time class bytes interval
	set awkCode {
		{ printf "%4d %8d %16d $4d\n", $4, $2, $6, $7; }
	}

        exec rm -f $graphfile
        set outdesc [open $graphfile w]
        #
        # this next part is xgraph specific
        #
        puts $outdesc "TitleText: $graphtitle"
        puts $outdesc "Device: Postscript"

        exec rm -f $tmpfile 
        puts "writing flow xgraph data to $graphfile..."
	exec awk $awkCode $infile | sort > $tmpfile
        exec awk [byte_awk] bandwidth=$bandwidth $tmpfile >@ $outdesc
	exec rm -f $tmpfile
	## exec awk [reclass_awk] $penaltyfile >@ $outdesc
        close $outdesc
}

# Plot per-flow bytes vs. time.
proc plot_bytes { name infile outfile bandwidth } {
	global xgraph
	create_bytes_graph $name $infile $outfile $bandwidth 
	puts "running xgraph for plotting per-flow bytes..."
	if { $xgraph == 1 } {
	  exec xgraph -bb -tk -m -ly 0,100 -x "time" -y "Bandwidth(%)" $outfile &
	}
	if { $xgraph == 0 } {
		exec csh bandwidth.com $outfile &
	}
}

#--------------------------------------------------------

# awk code used to produce:
#      x axis: time
#      y axis: aggregate drop ratios in packets
proc frac_awk { } {
        set awkCode {
            {
                if ($1 > prevtime){
                        if (prevtime > 0) print prevtime, 100.0 * frac
			prevtime = $1
			frac = $16/$12
		}
            }
	    END { print prevtime, 100.0 * frac }
        }
        return $awkCode
}

# plot time vs. aggregate drop ratio
proc create_frac_graph { graphtitle graphfile } {
        global flowfile awkprocedure

        exec rm -f $graphfile
        set outdesc [open $graphfile w]
        #
        # this next part is xgraph specific
        #
        puts $outdesc "TitleText: $graphtitle"
        puts $outdesc "Device: Postscript"

        puts "writing flow xgraph data to $graphfile..."

        exec sort -n +1 -o $flowfile $flowfile
        exec awk [frac_awk] $flowfile >@ $outdesc
        close $outdesc
}

# plot true average of arriving packets that are dropped
proc plot_dropave { name } {
	global flowgraphfile fracgraphfile xgraph awkprocedure
	create_frac_graph $name $fracgraphfile 
	puts "running time xgraph for plotting drop ratios..."
	if { $xgraph == 1 } {
	  exec xgraph -bb -tk -m -x "time" -y "Drop_Fraction(%)" $fracgraphfile &
	}
	if { $xgraph == 0 } {
		exec csh dropave.com $fracgraphfile &
	}
}

#--------------------------------------------------------------------

# plot tcp-friendly bandwidth 
# "factor" is packetsize/rtt, for packetsize in bytes and rtt in msec.
# bandwidth is in Kbps, goodbandwidth is in Bps
proc create_friendly_graph { graphtitle graphfile ratiofile bandwidth } {
	set awkCode {
		BEGIN { print "\"reference"; drops=0; packets=0;}
		{
		drops = $6 - drops;
		packets = $4 - packets;
		rtt = 0.06
		if (drops > 0) {
		  dropratio = drops/packets;
		  goodbandwidth = 1.22*factor/sqrt(dropratio);
		  print $2, 100*goodbandwidth*8/(bandwidth*1000);
		}
		drops = $6; packets = $4;
		}
	}
	set packetsize 1500
	set rtt 0.06
	set factor [expr $packetsize / $rtt]

        exec rm -f $graphfile
        set outdesc [open $graphfile w]
        #
        # this next part is xgraph specific
        #
        puts $outdesc "TitleText: $graphtitle"
        puts $outdesc "Device: Postscript"

        puts "writing friendly xgraph data to $graphfile..."

	exec cat Ref >@ $outdesc
	exec awk $awkCode bandwidth=$bandwidth factor=$factor $ratiofile >@ $outdesc
        close $outdesc
}

# Plot tcp-friendly bandwidth.
proc plot_friendly { name bandwidth } {
	global friendlygraphfile xgraph ratiofile
	puts "beginning time xgraph for tcp-friendly bandwidth..."
	create_friendly_graph $name $friendlygraphfile $ratiofile $bandwidth
	puts "running time xgraph for tcp-friendly bandwidth..."
	if { $xgraph == 1 } {
	  exec xgraph -bb -tk -m -ly 0,200 -x "time" -y "Bandwidth(%)" $friendlygraphfile &
	}
	if { $xgraph == 0 } {
		exec csh bandwidth.com $friendlygraphfile &
	}
}

#--------------------------------------------------------------------

proc tcpDumpAll { tcpSrc interval label } {
    global ns
    proc dump { src interval label } {
	global ns
	$ns at [expr [$ns now] + $interval] "dump $src $interval $label"
	puts time=[$ns now]/class=$label/ack=[$src set ack_]
    }
    puts $label:window=[$tcpSrc set window_]/packet-size=[$tcpSrc set packetSize_]
    $ns at 0.0 "dump $tcpSrc $interval $label"
}

# dump stats for single flow f only
proc printFlow { f outfile fm } {
    global ns
    puts $outfile [list [$ns now] [$f set flowid_] 0 0 [$f set flowid_] [$f set src_] [$f set dst_] [$f set parrivals_] [$f set barrivals_] [$f set epdrops_] [$f set ebdrops_] [$fm set parrivals_] [$fm set barrivals_] [$fm set epdrops_] [$fm set ebdrops_] [$fm set pdrops_] [$fm set bdrops_] [$f set pdrops_] [$f set bdrops_]]
}

proc flowDump { link fm } {
    global category pthresh drop_interval ns flowdesc
	if {$category == "unforced"} {
	    if {[$fm set epdrops_] >= $pthresh} {
		$fm dump
		$fm reset
		foreach f [$fm flows] {
		  $f reset
		}
		set drop_interval 2.0
	    } else {
		set drop_interval 1.0
	    }
	} elseif {[string compare $category "forced"] == 0} {
	    if {[expr [$fm set pdrops_] - [$fm set epdrops_]] >= $pthresh} {
		$fm dump
		$fm reset
                foreach f [$fm flows] {
                  $f reset
                } 
		set drop_interval 2.0
	    } else {
		set drop_interval 1.0
	    }
	} elseif {[string compare $category "combined"] == 0} {
	    if {[$fm set pdrops_] >= $pthresh} {
		$fm dump
                $fm reset
                foreach f [$fm flows] {
                  $f reset
                } 
		set drop_interval 2.0
	    } else {
		set drop_interval 1.0
	    } 
	} else {
	    puts stderr "Error: flowDump: drop category $category unknown."
	}
    # check data
    # if data good (if N > 2.3/p^2, N is field 14, flowid is 2, packet
    # drops is 10 ), 
    #     $fm resetcounter
    #     $flowmgr total-drops-pthresh n
    #     copy data to permanent file
    # if data not good, $flowmgr total-drops-pthresh n 
}

proc flowDump1 { link fm } {
    flowDump $link $fm
}

proc new_tcp { startTime source dest window class dump size } {
    global ns
    set tcp [$ns create-connection TCP/Reno $source TCPSink $dest $class]
    $tcp set window_ $window
    if {$size > 0}  {
	$tcp set packetSize_ $size
    }
    set ftp [$tcp attach-source FTP]
    $ns at $startTime "$ftp start"
    if {$dump == 1 } {
	tcpDumpAll $tcp 20.0 $class
    }
}

proc new_cbr { startTime source dest pktSize interval class } {
    global ns
    set cbr [$ns create-connection CBR $source LossMonitor $dest $class]
    if {$pktSize > 0} {
	$cbr set packetSize_ $pktSize
    }
    $cbr set interval_ $interval
    $ns at $startTime "$cbr start"
}

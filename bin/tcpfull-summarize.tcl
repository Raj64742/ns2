#
# program to process a (full) tcp trace file and summarize
# various things (dataful acks, pure acks, etc).
#
# invoke with "tclsh thisfile infile outprefix"
#

set seqmod 0

proc forward_segment { time seqno } {
	global segchan
	puts $segchan "$time $seqno"
}

proc forward_emptysegment { time seqno } {
	global emptysegchan
	puts $emptysegchan "$time $seqno"
}
proc backward_dataful_ack { time ackno } {
	global ackchan
	puts $ackchan "$time $ackno"
}
proc backward_pure_ack { time ackno } {
	global packchan
	puts $packchan "$time $ackno"
}
proc drop_pkt { time ackno } {
	global dropchan
	puts $dropchan "$time $ackno"
}

proc ctrl { time tflags seq } {
	global ctrlchan
	puts $ctrlchan "$time $seq"
}

proc ecnecho_pkt { time ackno } {
	global ecnchan
	puts $ecnchan "$time $ackno"
}

proc cong_act { time seqno } {
	global cactchan
	puts $cactchan "$time $seqno"
}

proc salen { time ackno } {
	global sachan
	puts $sachan "$time $ackno"
}

set synfound 0
proc parse_line line {
	global synfound active_opener passive_opener seqmod

	set okrecs { - + d }
	if { [lsearch $okrecs [lindex $line 0]] < 0 } {
		return
	}
	set sline [split $line " "]
	if { [llength $sline] < 15 } {
		puts stderr "apparently not in extended full-tcp format!"
		exit 1
	}
	set field(op) [lindex $sline 0]
	set field(time) [lindex $sline 1]
	set field(tsrc) [lindex $sline 2]
	set field(tdst) [lindex $sline 3]
	set field(ptype) [lindex $sline 4]
	set field(len) [lindex $sline 5]
	set field(pflags) [lindex $sline 6]
	set field(fid) [lindex $sline 7]
	set field(src) [lindex $sline 8]
	set field(srcaddr) [lindex [split $field(src) "."] 0]
	set field(srcport) [lindex [split $field(src) "."] 1]
	set field(dst) [lindex $sline 9]
	set field(dstaddr) [lindex [split $field(dst) "."] 0]
	set field(dstaddr) [lindex [split $field(dst) "."] 1]
	if { $seqmod != 0 } {
		set field(seqno) [expr [lindex $sline 10] % $seqmod]
	} else {
		set field(seqno) [lindex $sline 10]
	}
	set field(uid) [lindex $sline 11]
	if { $seqmod != 0 } {
		set field(tcpackno) [expr [lindex $sline 12] % $seqmod]
	} else {
		set field(tcpackno) [lindex $sline 12]
	}
	set field(tcpflags) [lindex $sline 13]
	set field(tcphlen) [lindex $sline 14]
	set field(salen) [lindex $sline 15]

	if { !($synfound) && [expr $field(tcpflags) & 0x02] } {
		global reverse
		set synfound 1
		if { [info exists reverse] && $reverse } {
			set active_opener $field(dst)
			set passive_opener $field(src)
		} else {
			set active_opener $field(src)
			set passive_opener $field(dst)
		}
	}

	set interesting 0

	if { $field(op) == "+" && [lsearch "tcp ack" $field(ptype)] >= 0} {
		set interesting 1
	}

	if { $interesting && [expr $field(tcpflags) & 0x03] } {
		# either SYN or FIN is on
		if { $field(src) == $active_opener && \
		    $field(dst)  == $passive_opener } {
			ctrl $field(time) $field(tcpflags) \
				[expr $field(seqno) + $field(len) - $field(tcphlen)]
		} elseif { $field(src) == $passive_opener && \
		    $field(dst) == $active_opener } {
			ctrl $field(time) $field(tcpflags) \
				$field(tcpackno)
		}
	}

	if { $interesting && $field(src) == $active_opener && $field(dst) == $passive_opener } {
		set topseq [expr $field(seqno) + $field(len) - $field(tcphlen)]
		if { $field(len) > $field(tcphlen) } {
			forward_segment $field(time) $topseq
		} else {
			forward_emptysegment $field(time) $topseq
		}
		if { [string index $field(pflags) 3] == "A" } {
			cong_act $field(time) $topseq
		}
		return
	}

	if { $interesting && $field(len) > $field(tcphlen) &&
	    $field(src) == $passive_opener && $field(dst) == $active_opener } {
		# record acks for the forward direction that have data
		backward_dataful_ack $field(time) $field(tcpackno)
		if { [string index $field(pflags) 0] == "C"  &&
			[string last N $field(pflags)] >= 0 } {
			ecnecho_pkt $field(time) $field(tcpackno)
		}
		return
	}

	if { $interesting && $field(len) == $field(tcphlen) &&
	    $field(src) == $passive_opener && $field(dst) == $active_opener } {
		# record pure acks for the forward direction
		backward_pure_ack $field(time) $field(tcpackno)
		if { [string index $field(pflags) 0] == "C" &&
			[string last N $field(pflags)] >= 0 } {
			ecnecho_pkt $field(time) $field(tcpackno)
		}
		if { $field(salen) > 0 } {
			salen $field(time) $field(tcpackno)
		}
		return
	}

	if { $field(op) == "d" && $field(src) == $active_opener &&
	    $field(dst) == $passive_opener } {
		drop_pkt $field(time) \
			[expr $field(seqno) + $field(len) - $field(tcphlen)]
		return
	}

}

proc parse_file chan {
	while { [gets $chan line] >= 0 } {
		parse_line $line
	}
}

proc dofile { infile outfile } {
	global ackchan packchan segchan dropchan ctrlchan emptysegchan ecnchan cactchan sachan

        set ackstmp $outfile.acks ; # data-full acks
        set segstmp $outfile.p; # segments
	set esegstmp $outfile.es; # empty segments
        set dropstmp $outfile.d; # drops
        set packstmp $outfile.packs; # pure acks
	set ctltmp $outfile.ctrl ; # SYNs + FINs
	set ecntmp $outfile.ecn ; # ECN acks
	set cacttmp $outfile.cact; # cong action
	set satmp $outfile.sack; # sack info
        exec rm -f $ackstmp $segstmp $esegstmp $dropstmp $packstmp $ctltmp $ecntmp $cacttmp $satmp

	set ackchan [open $ackstmp w]
	set segchan [open $segstmp w]
	set emptysegchan [open $esegstmp w]
	set dropchan [open $dropstmp w]
	set packchan [open $packstmp w]
	set ctrlchan [open $ctltmp w]
	set ecnchan [open $ecntmp w]
	set cactchan [open $cacttmp w]
	set sachan [open $satmp w]

	set inf [open $infile r]

	parse_file $inf

	close $ackchan
	close $segchan
	close $emptysegchan
	close $dropchan
	close $packchan
	close $ctrlchan
	close $ecnchan
	close $cactchan
	close $sachan
}

proc getopt {argc argv} { 
        global opt
        lappend optlist m r

        for {set i 0} {$i < $argc} {incr i} {
                set arg [lindex $argv $i]
                if {[string range $arg 0 0] != "-"} continue

                set name [string range $arg 1 end]
                set opt($name) [lindex $argv [expr $i+1]]
        }
}

getopt $argc $argv
set base 0

if { [info exists opt(m)] && $opt(m) != "" } {
	global seqmod base opt argc argv
	set seqmod $opt(m)
	incr argc -2
	incr base 2
}

if { [info exists opt(r)] && $opt(r) != "" } {
	global reverse base argc argv
	set reverse 1
	incr argc -1
	incr base
}

if { $argc < 2 || $argc > 3 } {
	puts stderr "usage: tclsh \[-m wrapamt\] \[-r\] tcpfull-summarize.tcl tracefile outprefix \[reverse\]"
	exit 1
} elseif { $argc == 3 } {
	if { [lindex $argv [expr $base + 2]] == "reverse" } {
		global reverse
		set reverse 1
	}
}
dofile [lindex $argv [expr $base]] [lindex $argv [expr $base + 1]]
exit 0

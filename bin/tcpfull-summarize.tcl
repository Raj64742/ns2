#
# program to process a (full) tcp trace file and summarize
# various things (dataful acks, pure acks, etc).
#
# invoke with "tclsh thisfile infile outprefix"
#
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

set synfound 0
proc parse_line line {
	global synfound active_opener passive_opener

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
	set field(seqno) [lindex $sline 10]
	set field(uid) [lindex $sline 11]
	set field(tcpackno) [lindex $sline 12]
	set field(tcpflags) [lindex $sline 13]
	set field(tcphlen) [lindex $sline 14]

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
		if { $field(len) > $field(tcphlen) } {
			forward_segment $field(time) \
				[expr $field(seqno) + $field(len) - $field(tcphlen)]
		} else {
			    forward_emptysegment $field(time) \
				[expr $field(seqno) + $field(len) - $field(tcphlen)]
		}
		return
	}

	if { $interesting && $field(len) > $field(tcphlen) &&
	    $field(src) == $passive_opener && $field(dst) == $active_opener } {
		# record acks for the forward direction that have data
		backward_dataful_ack $field(time) $field(tcpackno)
		return
	}

	if { $interesting && $field(len) == $field(tcphlen) &&
	    $field(src) == $passive_opener && $field(dst) == $active_opener } {
		# record pure acks for the forward direction
		backward_pure_ack $field(time) $field(tcpackno)
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
	global ackchan packchan segchan dropchan ctrlchan emptysegchan

        set ackstmp $outfile.acks ; # data-full acks
        set segstmp $outfile.p; # segments
	set esegstmp $outfile.es; # empty segments
        set dropstmp $outfile.d; # drops
        set packstmp $outfile.packs; # pure acks
	set ctltmp $outfile.ctrl ; # SYNs + FINs
        exec rm -f $ackstmp $segstmp $esegstmp $dropstmp $packstmp $ctltmp

	set ackchan [open $ackstmp w]
	set segchan [open $segstmp w]
	set emptysegchan [open $esegstmp w]
	set dropchan [open $dropstmp w]
	set packchan [open $packstmp w]
	set ctrlchan [open $ctltmp w]

	set inf [open $infile r]

	parse_file $inf

	close $ackchan
	close $segchan
	close $emptysegchan
	close $dropchan
	close $packchan
	close $ctrlchan
}

if { $argc < 2 || $argc > 3 } {
	puts stderr "usage: tclsh tcpfull-summarize.tcl tracefile outprefix [reverse]"
	exit 1
} elseif { $argc == 3 } {
	if { [lindex $argv 2] == "reverse" } {
		global reverse
		set reverse 1
	}
}
dofile [lindex $argv 0] [lindex $argv 1]
exit 0

proc mvar args {
	upvar self _s
	uplevel $_s instvar $args
}

proc bw_parse { bspec } {
	if { [scan $bspec "%f%s" b unit] != 2 } {
		puts stderr "error: bw_parse: can't scan `$bspec'."
		exit 1
	}
	switch $unit {
	bps  { return $b }
	kb/s - kbps { return [expr $b*1000] }
        Mb/s - Mbps { return [expr $b*1000000] }
	Gb/s - Gbps { return [expr $b*1000000000] }
	default { 
		  puts: "error unknown unit `$unit'" 
		  exit 1
		}
	}
}

Session/RTP instproc init {} {
	$self next 
	mvar dchan_ cchan_
	set cchan_ [new agent/rtcp]
	set dchan_ [new agent/cbr/rtp]
	$dchan_ set packet-size 1024

	$dchan_ session $self
	$cchan_ session $self

	$self set rtcp_timer_ [new RTCPTimer $self]
}

Session/RTP instproc start {} {
	mvar group_
	if ![info exists group_] {
		puts "error: can't transmit before joining group!"
		exit 1
	}

	mvar cchan_ 
	$cchan_ start 
}

Session/RTP instproc transmit {} {
	mvar group_
	if ![info exists group_] {
		puts "error: can't transmit before joining group!"
		exit 1
	}

	mvar dchan_
	$dchan_ start 
}


Session/RTP instproc report-interval { i } {
	mvar cchan_
	$cchan_ set interval_ $i
}

Session/RTP instproc bye {} {
	mvar cchan_ dchan_
	$dchan_ stop
	$cchan_ bye
}

Session/RTP instproc attach-to { node } {
	mvar dchan_ cchan_
	$node attach $dchan_
	$node attach $cchan_

	$self set node_ $node
}

Session/RTP instproc join-group { g } {
	set g [expr $g]
	
	$self set group_ $g

	mvar node_ dchan_ cchan_ 

	$dchan_ set dst $g
	$node_ join-group $dchan_ $g

	incr g

	$cchan_ set dst $g
	$node_ join-group $cchan_ $g
}

Session/RTP instproc leave-group { } {
	mvar group_ node_ cchan_
	$node_ leave-group $dchan_ $group_
	$node_ leave-group $cchan_ [expr $group_+1]
	
	$self unset group_
}

Session/RTP instproc tx-bw { bspec } {
	
	set b [bw_parse $bspec]

	$self set txBW_ $b

	mvar rtcp_timer_
	$rtcp_timer_ session-bw $b

	mvar dchan_
	set ps [$dchan_ set packet-size]

	$dchan_ set interval_ [expr 8.*$ps/$b]
}


Session/RTP instproc sample-size { cc } {
	mvar rtcp_timer_
	$rtcp_timer_ sample-size $cc
}

Session/RTP instproc adapt-timer { nsrc nrr we_sent } {
	mvar rtcp_timer_
	$rtcp_timer_ adapt $nsrc $nrr $we_sent
}

Class RTCPTimer 

RTCPTimer instproc init { session } {
	$self next
	
	# Could make most of these class instvars.
	
	mvar session_bw_fraction_ min_rpt_time_ inv_sender_bw_fraction_
	mvar inv_rcvr_bw_fraction_ size_gain_ avg_size_ inv_bw_

	set session_bw_fraction_ 0.05

	# XXX just so we see some reports in short sim's...
	set min_rpt_time_ 1.   
	
	set sender_bw_fraction 0.25
	set rcvr_bw_fraction [expr 1. - $sender_bw_fraction]
	
	set inv_sender_bw_fraction_ [expr 1. / $sender_bw_fraction]
	set inv_rcvr_bw_fraction_ [expr 1. / $rcvr_bw_fraction]
	
	set size_gain_ 0.125	

	set avg_size_ 128.
	set inv_bw_ 0.

	mvar session_
	set session_ $session
	
        # Schedule a timer for our first report using half the
        # min ctrl interval.  This gives us some time before
        # our first report to learn about other sources so our
        # next report interval will account for them.  The avg
        # ctrl size was initialized to 128 bytes which is
        # conservative (it assumes everyone else is generating
        # SRs instead of RRs).
	
	mvar min_rtp_time_ avg_size_ inv_bw_
	set rint [expr $avg_size_ * $inv_bw_]
	
	set t [expr $min_rpt_time_ / 2.]

	if { $rint < $t } {
		set rint $t
	}
	
	$session_ report-interval $rint
}
	
RTCPTimer instproc sample-size { cc } {
	mvar avg_size_ size_gain_

	set avg_size_ [expr $avg_size_ + $size_gain_ * ($cc + 28 - $avg_size_)]
}

RTCPTimer instproc adapt { nsrc nrr we_sent } {
	mvar inv_bw_ avg_size_ min_rpt_time_
	mvar inv_sender_bw_fraction_ inv_rcvr_bw_fraction_
	
	 # Compute the time to the next report.  we do this here
	 # because we need to know if there were any active sources
	 # during the last report period (nrr above) & if we were
	 # a source.  The bandwidth limit for ctrl traffic was set
	 # on startup from the session bandwidth.  It is the inverse
	 # of bandwidth (ie., ms/byte) to avoid a divide below.

	if { $nrr > 0 } {
		if { $we_sent } {
			set inv_bw_ [expr $inv_bw_ * $inv_sender_bw_fraction_]
			set nsrc $nrr
		} else {
			set inv_bw_ [expr $inv_bw_ * $inv_rcvr_bw_fraction_]
			incr nsrc -$nrr
		}
	}
	
	set rint [expr $avg_size_ * $nsrc * $inv_bw_]
	if { $rint < $min_rpt_time_ } {
		set rint $min_rpt_time_
	}
	
	mvar session_
	$session_ report-interval $rint
}
	
RTCPTimer instproc session-bw { b } {
	$self set inv_bw_ [expr 1. / $b ]
}


agent/rtcp set interval_ 0.
agent/rtcp set random_ 0
agent/rtcp set cls 32

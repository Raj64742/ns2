#
# RTMechanisms: routines for the router mechanisms paper
#
# conventions:
# 	procs of the form "do_..."  are run periodically
#	instvars starting with Capitals are constants
#
Class RtMechanisms

RTMechanisms instproc tcp_ref_bw { mtu rtt droprate } {
	set guideline [expr 1.22 * $mtu / ($rtt*sqrt($droprate)) ]
}

RTMechanisms instproc notokbandwidth { ref_bw flow_bw } {
	$self instvar safety_factor
	if { $flow_bw > ($safety_factor * $ref_bw) } {
		return "true"
	}
	return "false"
}

RTMechanisms instproc droprate { arrs drops } {
	set droprate [expr double($drops) / $arrs]
}

RTMechanisms isntproc maxmetric flows {
	$self instvar okboxfm_
	set bdrops [$okboxfm_ set bdrops] ; # total bytes dropped
	set pdrops [$okboxfm_ set pdrops] ; # total pkts dropped
	set ebdrops [$okboxfm_ set ebdrops] ; # bytes dropped early (unforced)
	set epdrops [$okboxfm_ set epdrops] ; # pkts drooped early (unforced)
	set fpdrops [expr $pdrops - $epdrops] ; # pkts dropped (forced)
	set fbdrops [expr $bdrops - $ebdrops] ; # bytes dropped (forced)

	set maxmetric -1.0
	set maxflow "none"

	foreach f $flows {
		# need to catch for div by zero
		set unforced_frac [expr [$f set epdrops] / $pdrops]
		set forced_frac [expr ([$f set pdrops] - [$f set epdrops]) / \
			$pdrops]
		set forced_metric [expr ([$f set bdrops] - [$f set ebdrops]) / \
			$fbdrops
		set unforced_metric [expr [$set epdrops] / $epdrops]
		set metric [expr $forced_frac * $forced_metric + \
			$unforced_frac * $unforced_metric]
		if { $metric > $maxmetric } {
			set maxmetric $metric
			set maxflow $f
		}
	}
	return "$maxflow $maxmetric"
}

RTMechanisms instproc print_good_and_bad { label } {
	$self instvar ns_
	$self instvar okboxfm_ pboxfm_
	$self instvar badclass_

	set now [$ns_ now]
	set ballot [$badclass_ allot]

	puts [format "time %5.1f $label goodflows: [$okboxfm_ flows], badflows: [$pboxfm_ flows] (allot %4.2f, droprate: ??)" $now $ballot]
}

RTMechanisms instproc print_allot_change { oallot nallot } {
	puts [format "pbox allotment changed from %6.5f to %6.5f" $oallot $nallot]
}

#
# move a flow to the bad box
# ie penalize a flow
#
RTMechanisms instproc penalize { badflow guideline_bw } {
	$self instvar npenalty_ badslot_ badhead_ cbq_
	$self instvar okboxfm_ pboxfm_
	$self do_reward

	incr npenalty_

	set classifier

	#
	# add the bad flow to the cbq/mechanisms classifier
	#
	set slot [$classifier installNext $badhead_]
	set src [$badflow src]
	set dst [$badflow dst]
	set fid [$badflow fid]
	$classifier set-hash auto $src $dst $fid $slot

	#
	# move flow manager state over
	#
	$okboxfm_ remove $badflow
	$pboxfm_ add $badflow

	#
	# reallocate allotment
	#

	set new_cbw [expr 0.5 * $guideline_bw * $npenalty_]
	if { $new_cbw > $max_cbq } {
		set $new_cbw $max_cbw
	}
	set cbqlink [$cbq_ link]
	set bw [[$cbqlink link] set bandwidth_]
	set oallot [$badclass_ set allot_]
	set cbw [expr $oallot * $bw]
	set nallot [expr $new_cbw / $bw]

	$badclass_ newallot $nallot
	$goodclass_ newallot [expr .98 - $nallot]
}

#
# move a flow to the good box
# ie stop penalizing a flow
#
RTMechanisms instproc unpenalize { goodflow guideline_bw } {
	$self instvar npenalty_ badslot_ badhead_ cbq_
	$self instvar okboxfm_ pboxfm_
	$self do_reward

	incr npenalty_ -1

	set classifier

	#
	# delete the bad flow from the cbq/mechanisms classifier
	# this flow will return to the "default" case in the classifier
	#
	set src [$goodflow src]
	set dst [$goodflow dst]
	set fid [$goodflow fid]
	set slot [$classifier del-hash auto $src $dst $fid]
	$classifier clear $slot

	#
	# move flow manager state over
	#
	$pboxfm_ remove $goodflow
	$okboxfm_ add $goodflow

	#
	# reallocate allotment
	#

	set bw [expr [[$cbqlink link] set bandwidth_] / 8.0]
	set oallot [$badclass_ set allot_]
	set cbw [expr $oallot * $bw]
	set new_cbw [expr $npenalty_ * $cbw / ($npenalty_ + 1)]
	set nallot [expr $new_cbw / $bw]
	$badclass_ newallot $nallot
	$goodclass_ newallot [expr .98 - $nallot]
}

# Check if bandwidth in penalty box should be adjusted.
RTMechanisms instproc checkbandwidth guideline_bw {
	$self instvar badclass_
	$self instvar npenalty_ cbq_
	set new_cbw [expr 0.5 * $guideline_bw * $npenalty_]
	set link_bw [expr [[$cbqlink link] set bandwidth_] / 8.0]
	set old_allot [$badclass_ allot]
	set class_bw [expr $old_allot * $link_bw]
	if { $new_cbw < $class_bw } {
		set new_allot [expr $new_cbw / $link_bw]
		return $new_allot
	}
	return "ok"
}

#
# main routine to determine if there are bad flows to penalize
#
RTMechanisms instproc do_detect {} {
	$self instvar ns_
	$self instvar last_detect_
	$self instvar Mintime_
	$self instvar okboxfm_

	set now [$ns_ now]
	set elapsed [expr $now - $last_detect_]
	if { $elapsed < $Mintime_ } {
		puts "ERROR: do_detect: elapsed: $elapsed, min: $Mintime_"
		exit 1
	}

	set barrivals [$okboxfm_ set barrivals]
	set ndrops [$okboxfm_ set pdrops] ; # drops == (total drops, incl epd)
	set droprateG [$self $narrivals $ndrops]
	set M [$self maxmetric [$okboxfm_ flows]]
	set badflow [lindex $M 0]
	set maxmetric [lindex $M 1]

	# estimate the bw's arrival rate without knowing it directly
	set flow_bw_est [expr $maxmetric * .01 * $barrivals / $elapsed]

	if { [$self test_friendly $badflow] != "ok" } {
		# didn't pass friendly test
		$self penalize $badflow
	} else if { [info exists $unresponsive($badflow)] } {
		# was unresponsive once already
		if { [$self test_unresponsive $badflow] != "ok" } {
			# is still unresponsive
			$self penalize $badflow
		}
	} else {
		if { [$self test_unresponsive $badflow] != "ok" } {
			$self add-history $badflow
			set unresponsive($badflow) $now
		} else if { [$self test_high $badflow] != "ok" } {
			$self penalize $badflow
		} else {
			$self checkbandwidth $guideline
			$self checkbandwidth $flow_bw_est
		}
	}
	$self flowdump
}

#
# RTMechanisms: routines for the router mechanisms paper
#
# this file contains primarily the support routines
# to manage the flows.  The policies defined by the
# tests are found in rtm_tests.tcl
#
# conventions:
# 	procs of the form "do_..."  are run periodically
#	instvars starting with Capitals are constants
#
Class RTMechanisms

source rtm_tests.tcl
source rtm_link.tcl

RTMechanisms instproc tcp_ref_bw { mtu rtt droprate } {
	set guideline [expr 1.22 * $mtu / ($rtt*sqrt($droprate)) ]
}

RTMechanisms instproc droprate { arrs drops } {
	if { $arrs == 0 } {
		return 0.0
	}
	return [expr double($drops) / $arrs]
}

#
# mmetric: maxmetric or minmetric in the ok box
#	op: one of "max" or "min"
#	flows: list of flows
#
RTMechanisms instproc mmetric { op flows } {
	$self instvar okboxfm_

	set bdrops [$okboxfm_ set bdrops_] ; # total bytes dropped
	set pdrops [$okboxfm_ set pdrops_] ; # total pkts dropped
	set ebdrops [$okboxfm_ set ebdrops_] ; # bytes dropped early (unforced)
	set epdrops [$okboxfm_ set epdrops_] ; # pkts drooped early (unforced)
	set fpdrops [expr $pdrops - $epdrops] ; # pkts dropped (forced)
	set fbdrops [expr $bdrops - $ebdrops] ; # bytes dropped (forced)

	if { $op == "max" } {
		set op ">"
		set metric -1.0
	} else if { $op == "min" } {
		set op "<"
		set metric 1000000
	}

	set flow ""

	foreach f $flows {
		# need to catch for div by zero
		set unforced_frac [expr [$f set epdrops_] / $pdrops]
		set forced_frac [expr ([$f set pdrops_] - [$f set epdrops_]) / \
			$pdrops]
		set forced_metric [expr ([$f set bdrops_] - [$f set ebdrops_]) / \
			$fbdrops
		set unforced_metric [expr [$f set epdrops_] / $epdrops]
		set metric [expr $forced_frac * $forced_metric + \
			$unforced_frac * $unforced_metric]
		if { $metric $op $metric } {
			set metric $metric
			set flow $f
		}
	}
	return "$flow $metric"
}

RTMechanisms instproc setstate { flow reason bandwidth droprate } { 
	$self instvar state_

	set state_($flow,reason) $reason
	set state_($flow,bandwidth) $bandwidth
	set state_($flow,droprate) $droprate
}

# set new allotment in pbox
RTMechanisms instproc pallot allotment {
	$self instvar badclass_
	$self instvar Maxallot_

	$badclass_ newallot $allotment
	$goodclass_ newallot [expr $Maxallot_ - $allotment]
}

# add a flow to the history buffer (for unresponsive test)
RTMechanisms instproc fhist-add { flow droprate bandwidth } {
	$self instvar hist_next_ Hist_max_
	$self instvar flowhist_

	# circular history buffer
	incr hist_next_
	if { $hist_next_ >= Hist_max_ } {
		set hist_next_ 0
	}
	set flowhist_($hist_next_,name) $flow
	set flowhist_($hist_next_,droprate) $droprate
	set flowhist_($hist_next_,bandwidth) $bandwidth
	return $hist_next_
}

# find entry in hist buffer with lowest droprate, return its index
# used for unresponsive test
RTMechanisms instproc fhist-mindroprate flow {
	$self instvar Hist_max_
	set dr 100000000
	set idx -1
	for { set i 0 } { $i < $Hist_max_ } { incr i } {
		if { flowhist_($i,name) == $flow &&
		     flowhist_($i,droprate) < $dr } {
			set dr $flowhist_($i,droprate)
			set idx $i
		}
	}
	return $idx
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
	$self instvar npenalty_ badslot_ badhead_ cbqlink_
	$self instvar okboxfm_ pboxfm_
	$self instvar Max_cbw_
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
	if { $new_cbw > $Max_cbw_ } {
		set $new_cbw $Max_cbw_
	}
	set bw [[$cbqlink_ link] set bandwidth_]
	set oallot [$badclass_ set allot_]
	set cbw [expr $oallot * $bw]
	set nallot [expr $new_cbw / $bw]

	$self pallot $nallot
}

#
# move a flow to the good box
# ie stop penalizing a flow
#
RTMechanisms instproc unpenalize goodflow {
	$self instvar npenalty_ badslot_ badhead_ cbqlink_
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

	set bw [expr [[$cbqlink_ link] set bandwidth_] / 8.0]
	set oallot [$badclass_ set allot_]
	set cbw [expr $oallot * $bw]
	set new_cbw [expr $npenalty_ * $cbw / ($npenalty_ + 1)]
	set nallot [expr $new_cbw / $bw]

	$self pallot $nallot
}

# Check if bandwidth in penalty box should be adjusted.
# basen on some change in npenalty_
RTMechanisms instproc checkbw_fair guideline_bw {
	$self instvar badclass_
	$self instvar npenalty_ cbqlink_
	set new_cbw [expr 0.5 * $guideline_bw * $npenalty_]
	set link_bw [expr [[$cbqlink_ link] set bandwidth_] / 8.0]
	set old_allot [$badclass_ allot]
	set class_bw [expr $old_allot * $link_bw]
	if { $new_cbw < $class_bw } {
		set new_allot [expr $new_cbw / $link_bw]
		return $new_allot
	}
	return "ok"
}

# Check if bandwidth in penalty box should be adjusted.
# basen on drop rate diffs between good and bad boxes
RTMechanisms instproc checkbw_droprate { droprateB droprateG } {
	$self instvar badclass_
	$self instvar npenalty_ cbqlink_
	if { $droprateB < 2 * $droprateG } {
		set link_bw [expr [[$cbqlink_ link] set bandwidth_] / 8.0]
		set old_allot [$badclass_ allot]
		set class_bw [expr $old_allot * $link_bw]
		set new_cbw [expr 0.5 * $class_bw]
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
	$self instvar Mintime_ Rtt_ Mtu_
	$self instvar okboxfm_
	$self instvar state_

	set now [$ns_ now]
	set elapsed [expr $now - $last_detect_]
	if { $elapsed < $Mintime_ } {
		puts "ERROR: do_detect: elapsed: $elapsed, min: $Mintime_"
		exit 1
	}

	set barrivals [$okboxfm_ set barrivals_]
	set parrivals [$okboxfm_ set parrivals_]
	set ndrops [$okboxfm_ set pdrops_] ; # drops == (total drops, incl epd)
	set droprateG [$self droprate $parrivals $ndrops]

	set M [$self mmetric max [$okboxfm_ flows]]
	set badflow [lindex $M 0]
	set maxmetric [lindex $M 1]

	if { $badflow == "" } {
		# nobody
		return
	}

	# estimate the bw's arrival rate without knowing it directly
	set flow_bw_est [expr $maxmetric * .01 * $barrivals / $elapsed]
	set guideline_bw  [$self tcp_ref_bw $Mtu_ $Rtt_ $droprateG]

	set friendly [$self test_friendly $badflow]
	if { [$self test_friendly $flow_bw_est $guideline] } {
		# didn't pass friendly test
		$self setstate $badflow "UNFRIENDLY" $flow_bw_est $droprateG
		$self penalize $badflow
	} else if { $state_($badflow,reason) == "UNRESPONSIVE" } {
		# was unresponsive once already
		$self instvar PUFrac_
		set u [$self test_unresponsive_again
		    $badflow $flow_bw_est $droprateG $PUFrac_ $PUFrac_]
		if { $u != "ok" } {
			# is still unresponsive
			$self setstate $badflow "UNRESPONSIVE2"
			    $flow_bw_est $droprateG
			$self penalize $badflow
		}
	} else {
		set nxt [$self fhist-add $badflow $droprateG $flow_bw_est]
		set u [$self test_unresponsive_initial
		    $badflow $flow_bw_est $droprateG]
		if { $u != "ok" } {
			$self setstate $badflow "UNRESPONSIVE"
			    $flow_bw_est $droprateG
		} else if { [$self test_high $flow_bw_est $droprateG $elapsed] != "ok" } {
			$self setstate $badflow "HIGH"
			    $flow_bw_est $droprateG
			$self penalize $badflow
		} else {
			set ck1 [$self checkbw_fair $guideline_bw]
			set ck2 [$self checkbw_fair $flow_bw_est]
			if { $ck1 != "ok" || $ck2 != "ok" } {
				if { $ck1 == "ok" } {
					set nallot $ck2
				} else if { $ck2 == "ok" } {
					set nallot $ck1
				} else {
					set nallot $ck2
					if { $ck1 < $ck2 } {
						set nallot $ck1
					}
				}
				$self pallot $nallot
			}
		}
	}
	$self flowdump
}


#
# main routine to determine if there are restricted flows
# that are now behaving better
#
RTMechanisms instproc do_reward {} {
	$self instvar ns_
	$self instvar last_reward_
	$self instvar Mintime_
	$self instvar state_
	$self instvar pboxfm_ okboxfm_

	set now [$ns_ now]
	set elapsed [expr $now - $last_reward_]
	if { $elapsed > $Mintime_ / 2 } {
		set parrivals [$pboxfm_ set parrivals_]
		set pdrops [$pboxfm_ set pdrops_]
		set barrivals [$pboxfm_ set barrivals_]
		set badBps [expr $barrivals / $elapsed]
		set pgoodarrivals [$okboxfm_ set parrivals_]
		set pflows [$pboxfm_ flows] ; # all penalized flows
		if { $parrivals == 0 } {
			# nothing!, everybody becomes good
			foreach f $pflows {
				$self unpenalize $f
			}
			return
		}
		set droprateB [$self droprate $pdrops $parrivals]
		set M [$self mmetric min $pflows]
		set goodflow [lindex $M 0]
		set goodmetric [lindex $M 1]
		if { $goodflow == "" } {
			#none
			return
		}
		set flow_bw_est [expr $goodmetric * .01 * $barrivals / $elapsed]
		#
		# if it was unfriendly and is now friendly, reward
		# if it was unresp and is now resp + friendly, reward
		# if it was high and is now !high + friendly, reward
		#
		switch $state_($goodflow,reason) {
			"UNFRIENDLY" {
				set fr [$self test_friendly $flow_bw_est \
				    [$self tcp_ref_bw $Mtu_ $Rtt_ $droprateB]]
				if { $fr == "ok" } {
					$self setstate $goodflow "OK" $flow_bw_est $droprateB
					$self reward $goodflow
				}
			}

			"UNRESPONSIVE" {
				$self instvar RUBFrac_
				$self instvar RUDFrac_
				set unr [$self test_unresponsive_again $goodflow $RUBFrac_ $RUDFrac_]
				if { $unr == "ok" } {
				    set fr [$self test_friendly $flow_bw_est \
				      [$self tcp_ref_bw $Mtu_ $Rtt_ $droprateB]]
				    if { $fr == "ok" } {
					$self setstate $goodflow "OK" $flow_bw_est $droprateB
					$self reward $goodflow
				    }
				}
			}

			"HIGH" {
				set h [$self test_high $goodflow]
				if { $h == "ok" } {
				    set fr [$self test_friendly $flow_bw_est \
				      [$self tcp_ref_bw $Mtu_ $Rtt_ $droprateB]]
				    if { $fr == "ok" } {
					$self setstate $goodflow "OK" $flow_bw_est $droprateB
					$self reward $goodflow
				    }
				}
			}
		}
		$fm_ dump
		if { $npenalty_ > 0 } {
			$self checkbw_droprate $droprateB $droprateG
		}
	}
}

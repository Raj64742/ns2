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
	if { $rtt == 0 || $droprate == 0 } {
		return "none"
	}
	$self vprint 9 "mtu: $mtu rtt: $rtt droprate: $droprate"
	set result [expr 1.22 * $mtu / ($rtt*sqrt($droprate))]
	set sqrt [expr sqrt($droprate) ]
	$self vprint 9 "sqrt: $sqrt result $result"
	return [expr 1.22 * $mtu / ($rtt*sqrt($droprate))]
}

RTMechanisms instproc frac { num denom } {
	if { $denom == 0 } {
		return 0.0
	}
	return [expr double($num) / $denom]
}

RTMechanisms instproc vprint args {
	$self instvar verbose_
	set level [lindex $args 0]
	set a [lrange $args 1 end]
	if { $level <= $verbose_ } {
		$self instvar ns_
		puts "[$ns_ now] $a"
		flush stdout
	}
}

#
# mmetric: maxmetric or minmetric in the ok box
#	op: one of "max" or "min"
#	flows: list of flows
#
RTMechanisms instproc mmetric { op flows } {
	$self instvar okboxfm_

	set tot_bdrops [$okboxfm_ set bdrops_] ; # total bytes dropped
	set tot_pdrops [$okboxfm_ set pdrops_] ; # total pkts dropped
	set tot_ebdrops [$okboxfm_ set ebdrops_] ; # bytes dropped early (unforced)
	set tot_epdrops [$okboxfm_ set epdrops_] ; # pkts drooped early (unforced)
	set tot_fpdrops [expr $tot_pdrops - $tot_epdrops] ; 
						# pkts dropped (forced)
	set tot_fbdrops [expr $tot_bdrops - $tot_ebdrops] ; 
						# bytes dropped (forced)

	if { $op == "max" } {
		set op ">"
		set metric -1.0
	} elseif { $op == "min" } {
		set op "<"
		set metric 1000000
	}

	set flow "none"

	set unforced_frac [$self frac $tot_epdrops $tot_pdrops]
	set forced_frac [expr 1 - $unforced_frac ]
	foreach f $flows {
		set fepdrops [$f set epdrops_]
		set fpdrops [$f set pdrops_]

		set fbdrops [$f set bdrops_]
		set febdrops [$f set ebdrops_]
			
		set forced_metric [$self frac [expr $fbdrops - $febdrops] $tot_fbdrops]
		set unforced_metric [$self frac $fepdrops $tot_epdrops]
		set fmetric [expr $forced_frac * $forced_metric + \
			$unforced_frac * $unforced_metric]
		if { [expr $fmetric $op $metric] } {
			set metric $fmetric
			set flow $f
		}
	}
	return "$flow $metric"
}

RTMechanisms instproc setstate { flow reason bandwidth droprate } { 
	$self instvar state_ ns_

	$self vprint 1 "SETSTATE: flow: $flow NEWSTATE (reason:$reason, bw: $bandwidth, droprate: $droprate)"

	set state_($flow,reason) $reason
	set state_($flow,bandwidth) $bandwidth
	set state_($flow,droprate) $droprate
	set state_($flow,ctime) [$ns_ now]
}

# set new allotment in pbox
RTMechanisms instproc pallot allotment {
	$self instvar badclass_ goodclass_
	$self instvar Maxallot_

	$self vprint 0 "PALLOT: Allots: pbox: $allotment, okbox: [expr $Maxallot_ - $allotment]"
	$badclass_ newallot $allotment
	$goodclass_ newallot [expr $Maxallot_ - $allotment]
}

# add a flow to the flow history array (for unresponsive test)
RTMechanisms instproc fhist-add { flow droprate bandwidth } {
	$self instvar hist_next_ Hist_max_
	$self instvar flowhist_

	# circular history buffer
	incr hist_next_
	if { $hist_next_ >= $Hist_max_ } {
		set hist_next_ 0
	}
	set flowhist_($hist_next_,name) $flow
	set flowhist_($hist_next_,droprate) $droprate
	set flowhist_($hist_next_,bandwidth) $bandwidth

	$self vprint 1 "HISTORY ADDITION: flow: $flow, droprate: $droprate, bw: $bandwidth"
	return $hist_next_
}

# find entry in hist buffer with lowest droprate, return its index
# used for unresponsive test
RTMechanisms instproc fhist-mindroprate flow {
	$self instvar Hist_max_
	$self instvar flowhist_
	set dr 100000000
	set idx -1

	for { set i 0 } { $i < $Hist_max_ } { incr i } {
		if { [info exists flowhist_($i,name)] &&
		     $flowhist_($i,name) == $flow &&
		     $flowhist_($i,droprate) < $dr } {
			set dr $flowhist_($i,droprate)
			set idx $i
		}
	}
	$self vprint 1 "HISTORY MINDR SEARCH (flow: $flow): hmax: $Hist_max_, index: $idx"
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
	$self instvar npenalty_ badslot_ cbqlink_
	$self instvar badclass_
	$self instvar okboxfm_ pboxfm_
	$self instvar Max_cbw_

	$self vprint 1 "penalizing flow $badflow, guideline bw: $guideline_bw"

	incr npenalty_
	set classifier [$cbqlink_ classifier]

	#
	# add the bad flow to the cbq/mechanisms classifier
	# the underlying object (badcl_) is already in $badslot_
	#
	set src [$badflow set src_]
	set dst [$badflow set dst_]
	set fid [$badflow set flowid_]
	$classifier set-hash auto $src $dst $fid $badslot_

	#
	# remove flow record from ok fmon
	# add it to pbox f mon
	#
	set okcl [$okboxfm_ classifier]
	set okslot [$okcl del-hash $src $dst $fid]
	$okcl clear $okslot
	set bcl [$pboxfm_ classifier]
	set bslot [$bcl installNext $badflow]
	$badflow reset
	$bcl set-hash auto $src $dst $fid $bslot

	#
	# reallocate allotment
	#

	set new_pbw [expr 0.5 * $guideline_bw * $npenalty_ ]
	$self vprint 1 "npenalty $npenalty_ guideline_bw $guideline_bw"
	if { $new_pbw > $Max_cbw_ } {
		set $new_pbw $Max_cbw_
	}
	$self instvar badclass_
	# link bw is in bits/sec
	set bw [expr [[$cbqlink_ link] set bandwidth_] / 8.0]
	$self vprint 1 "new_pbw $new_pbw bw $bw" 
	set nallot [expr $new_pbw / $bw]

	$self pallot $nallot
	$self vprint 2 "penalize done.."
}

#
# move a flow to the good box
# ie stop penalizing a flow
#
RTMechanisms instproc unpenalize goodflow {
	$self instvar npenalty_ badslot_ badhead_ cbqlink_
	$self instvar okboxfm_ pboxfm_
	$self instvar badclass_

	incr npenalty_ -1

	set classifier [$cbqlink_ classifier]
	$self vprint 0 "UNPENALIZE flow $goodflow"

	#
	# delete the bad flow from the cbq/mechanisms classifier
	# this flow will return to the "default" case in the classifier
	# do not "clear" the entry, as that would lose the reference
	# to $badclass_ in the CBQ classifier
	#
	set src [$goodflow set src_]
	set dst [$goodflow set dst_]
	set fid [$goodflow set flowid_]
	$classifier del-hash $src $dst $fid

	#
	# remove flow record from pbox fmon
	# add it to okbox box fmon
	#
	set pcl [$pboxfm_ classifier]
	set pslot [$pcl del-hash $src $dst $fid]
	$pcl clear $pslot
	set gcl [$okboxfm_ classifier]
	set gslot [$gcl installNext $goodflow]
	$goodflow reset
	$gcl set-hash auto $src $dst $fid $gslot

	#
	# reallocate allotment
	#

	set bw [expr [[$cbqlink_ link] set bandwidth_] / 8.0]
	set oallot [$badclass_ allot]
	set cbw [expr $oallot * $bw]
	set new_cbw [expr $npenalty_ * $cbw / ($npenalty_ + 1)]
	set nallot [expr $new_cbw / $bw]

	$self pallot $nallot
	$self vprint 2 "unpenalize done..."
}

# Check if bandwidth in penalty box should be adjusted.
# basen on some change in npenalty_
RTMechanisms instproc checkbw_fair guideline_bw {
	$self instvar badclass_
	$self instvar npenalty_ cbqlink_

	if { $guideline_bw == "none" } {
		return "ok"
	}

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

	set link_bw [expr [[$cbqlink_ link] set bandwidth_] / 8.0]
	set old_allot [$badclass_ allot]
	if { $droprateB < 2 * $droprateG } {
		set class_bw [expr $old_allot * $link_bw]
		set new_cbw [expr 0.5 * $class_bw]
		set new_allot [expr $new_cbw / $link_bw]
		$self vprint 0 "Penalty box: was $old_allot now $new_allot"
		return $new_allot
	}
	$self vprint 1 "Penalty box: $old_allot"
	return "ok"
}

#
# main routine to determine if there are bad flows to penalize
#
RTMechanisms instproc sched-detect {} {
	$self instvar Detect_interval_ detect_pending_
	$self instvar ns_
	if { $detect_pending_ == "true" } {
		$self vprint 2 "SCHEDULING DETECT (NO, ALREADY PENDING)"
		return
	}
	set now [$ns_ now]
	set then [expr $now + $Detect_interval_]
	set detect_pending_ true
	$ns_ at $then "$self do_detect"
	$self vprint 2 "SCHEDULING DETECT for $then"
}
	
RTMechanisms instproc do_detect {} {
	$self instvar ns_
	$self instvar last_detect_
	$self instvar Mintime_ Rtt_ Mtu_
	$self instvar okboxfm_
	$self instvar state_
	$self instvar detect_pending_

	$okboxfm_ dump

	set detect_pending_ false
	set now [$ns_ now]
	$self vprint 2 "DO_DETECT started at time $now, last: $last_detect_"
	set elapsed [expr $now - $last_detect_]
	set last_detect_ $now
	if { $elapsed < $Mintime_ } {
		puts "ERROR: do_detect: elapsed: $elapsed, min: $Mintime_"
		exit 1
	}

	set barrivals [$okboxfm_ set barrivals_]
	set parrivals [$okboxfm_ set parrivals_]
	set ndrops [$okboxfm_ set pdrops_] ; # drops == (total drops, incl epd)
	set droprateG [$self frac $ndrops $parrivals]

	set M [$self mmetric max "[$okboxfm_ flows]"]
	set badflow [lindex $M 0]
	set maxmetric [lindex $M 1]

	$self vprint 2 "DO_DETECT: droprateG: $droprateG (drops:$ndrops, arrs:$parrivals"
	if { $badflow == "none" } {
		$self vprint 1 "DO_DETECT: no candidate bad flows... returning"
		$self sched-detect
		# nobody
		return
	}
	$self vprint 2 "DO_DETECT: possible bad flow: $badflow ([$badflow set src_], [$badflow set dst_], [$badflow set flowid_]), maxmetric:$maxmetric"

	set known false
	if { [info exists state_($badflow,ctime)] } {
		set known true
		set flowage [expr $now - $state_($badflow,ctime)]
		if { $flowage < $Mintime_ } {
			$self vprint 1 "DO_DETECT: flow $badflow too young ($flowage)"
			$self sched-detect
			return
		}
	}

	# estimate the bw's arrival rate without knowing it directly
	#	note: in ns-1 maxmetric was a %age, here it is a frac
	set flow_bw_est [expr $maxmetric * $barrivals / $elapsed]
	$self vprint 9 "maxmetric $maxmetric barrivals $barrivals elapsed $elapsed"
	set guideline_bw  [$self tcp_ref_bw $Mtu_ $Rtt_ $droprateG]

	set friendly [$self test_friendly $badflow $flow_bw_est $guideline_bw]
	if { $friendly == "fail" } {
		# didn't pass friendly test
		$self setstate $badflow "UNFRIENDLY" $flow_bw_est $droprateG
		$self penalize $badflow $guideline_bw
		$self sched-reward
	} elseif { $known == "true" && $state_($badflow,reason) == "UNRESPONSIVE" } {
		# was unresponsive once already
		$self vprint 1 "WAS unresponsive once already"
		$self instvar PUFrac_
		set u [$self test_unresponsive_again \
		    $badflow $flow_bw_est $droprateG $PUFrac_ $PUFrac_]
		if { $u == "fail" } {
			# is still unresponsive
			$self setstate $badflow "UNRESPONSIVE2" \
			    $flow_bw_est $droprateG
			$self penalize $badflow $flow_bw_est
			$self sched-reward
		}
	} else {
		set nxt [$self fhist-add $badflow $droprateG $flow_bw_est]
		set u [$self test_unresponsive_initial \
		    $badflow $flow_bw_est $droprateG $nxt]
		if { $u == "fail" } {
			$self vprint 1 "FIRST TIME unresponsive"
			$self setstate $badflow "UNRESPONSIVE" \
			    $flow_bw_est $droprateG
		} elseif { [$self test_high $badflow $flow_bw_est $droprateG $elapsed] == "fail" } {
			$self setstate $badflow "HIGH" \
			    $flow_bw_est $droprateG
			$self penalize $badflow $flow_bw_est
			$self sched-reward
		} else {
			set ck1 [$self checkbw_fair $guideline_bw]
			set ck2 [$self checkbw_fair $flow_bw_est]
			if { $ck1 == "fail" || $ck2 == "fail" } {
				if { $ck1 == "ok" } {
					set nallot $ck2
				} elseif { $ck2 == "ok" } {
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
	$self sched-detect
	foreach f [$okboxfm_ flows] {
		$f reset
	}
	$okboxfm_ reset
	$self vprint 2 "do_detect complete..."
}


#
# main routine to determine if there are restricted flows
# that are now behaving better
#

RTMechanisms instproc sched-reward {} {
	$self instvar Reward_interval_ reward_pending_ 
	$self instvar ns_
	if { $reward_pending_ == "true" } {
		$self vprint 2 "SCHEDULING REWARD (NO, ALREADY PENDING)"
		return
	}
	set now [$ns_ now]
	set then [expr $now + $Reward_interval_]
	set reward_pending_ true
	$ns_ at $then "$self do_reward"
	$self vprint 1 "SCHEDULING REWARD for $then"
}
RTMechanisms instproc do_reward {} {
	$self instvar ns_
	$self instvar last_reward_ reward_pending_
	$self instvar Mintime_
	$self instvar state_
	$self instvar pboxfm_ okboxfm_
	$self instvar npenalty_
	$self instvar Mtu_ Rtt_

	$pboxfm_ dump

	set reward_pending_ false
	set now [$ns_ now]
	$self vprint 2 "DO_REWARD starting at $now, last: $last_reward_"
### Is this wrong? - question from Ion Stoica.
### When is $last_reward_ set when a flow is first penalized?
### What if the "penalize" and "reward" cycles are not in sync?
### So that "$now - $last_reward_" is long, but that flow has
### not been categorized in this box for that period of time?
	set elapsed [expr $now - $last_reward_]

	set last_reward_ $now

	if { $npenalty_ == 0 } {
		return
	}

	set parrivals [$pboxfm_ set parrivals_]
	set pdepartures [$pboxfm_ set pdepartures_]
	set pdrops [$pboxfm_ set pdrops_]
	set barrivals [$pboxfm_ set barrivals_]
	set badBps [expr $barrivals / $elapsed]
	set pflows [$pboxfm_ flows] ; # all penalized flows


	$self vprint 1 "DO_REWARD: droprateB: [$self frac $pdrops $parrivals] (pdrops: $pdrops, parr: $parrivals pdep: $pdepartures)"
	$self vprint 1 "DO_REWARD: badbox pool of flows: $pflows"

	if { $parrivals == 0 && $elapsed > $Mintime_ } {
		# nothing!, everybody becomes good
		$self vprint 1 "do_reward: no bad flows, reward all"
		foreach f $pflows {
			$self unpenalize $f
		}
		set npenalty_ 0
		return
	}

	set droprateB [$self frac $pdrops $parrivals]
	set M [$self mmetric min "$pflows"]
	set goodflow [lindex $M 0]
	set goodmetric [lindex $M 1]
	if { $goodflow == "none" } {
		#none
		$self sched-reward
		return
	}
	set flowage [expr $now - $state_($goodflow,ctime)]
	$self vprint 2 "found flow $goodflow as potential good-guy (age: $flowage)"
	if { $flowage < $Mintime_ } {
		$self vprint 1 "DO_REWARD: flow $goodflow too young ($flowage) to be rewarded"
		$self sched-reward
		return
	}


	set pgoodarrivals [$okboxfm_ set parrivals_]
	set ngdrops [$okboxfm_ set pdrops_]
	set droprateG [$self frac $ngdrops $pgoodarrivals]


	# assume we have per-flow arrival stats in bad box
	#set flow_bw_est [expr $goodmetric * $barrivals / $elapsed]
	set flow_bw_est [expr [$goodflow set barrivals_] / $elapsed]
	
	# if it was unfriendly and is now friendly, reward
	# if it was unresp and is now resp + friendly, reward
	# if it was high and is now !high + friendly, reward
	#
	switch $state_($goodflow,reason) {
		"UNFRIENDLY" {
			set fr [$self test_friendly $goodflow $flow_bw_est \
			    [$self tcp_ref_bw $Mtu_ $Rtt_ $droprateB]]
			if { $fr == "ok" } {
				$self setstate $goodflow "OK" $flow_bw_est $droprateB
				$self unpenalize $goodflow
			}
		}

		"UNRESPONSIVE" {
			$self instvar RUBFrac_
			$self instvar RUDFrac_
			set unr [$self test_unresponsive_again $goodflow $RUBFrac_ $RUDFrac_]
			if { $unr == "ok" } {
			    set fr [$self test_friendly $goodflow $flow_bw_est \
			      [$self tcp_ref_bw $Mtu_ $Rtt_ $droprateB]]
			    if { $fr == "ok" } {
				$self setstate $goodflow "OK" $flow_bw_est $droprateB
				$self unpenalize $goodflow
			    }
			}
		}

		"HIGH" {
			set h [$self test_high $goodflow $flow_bw_est $droprateB $elapsed]
			if { $h == "ok" } {
			    set fr [$self test_friendly $goodflow $flow_bw_est \
			      [$self tcp_ref_bw $Mtu_ $Rtt_ $droprateB]]
			    if { $fr == "ok" } {
				$self setstate $goodflow "OK" $flow_bw_est $droprateB
				$self unpenalize $goodflow
			    }
			}
		}
	}
	if { $npenalty_ > 0 } {
		$self checkbw_droprate $droprateB $droprateG
	}
	$self sched-reward
	foreach f [$pboxfm_ flows] {
		$f reset
	}
	$pboxfm_ reset
	$self vprint 2 "do_reward complete..."
}

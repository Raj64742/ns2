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

RTMechanisms instproc droprate { drops arrs } {
	set droprate [expr double($drops) / $arrs]
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
RTMechanisms instproc checkbandwidth { droprateB droprateG } {
	$self instvar badclass_
        if { $droprateB < 2 * $droprateG } {
		set link_bw [expr [[$cbqlink link] set bandwidth_] / 8.0]
                set old_allot [$badclass_ allot]
                set class_bw [expr $old_allot * $link_bw]
                set new_cbw [expr $class_bw / 2]
                set new_allot [expr $new_cbw / $link_bw]
		return $new_allot
        } 
	return "ok"
}

#
# here are the main tests for detecting
#	unfriendly, unresponsive and high-bw flows
#

RTMechanisms instproc init { ns cbqlink rtt mtu enable } { 
        $self instvar Safety_factor_
        $self instvar Max_cbw_  
        $self instvar Maxallot_
        $self instvar Mintime_
        $self instvar npenalty_     
	$self instvar cbqlink_
	$self instvar last_reward_ last_detect_
	$self instvar Reward_interval_ reward_pending_
	$self instvar Detect_interval_ detect_pending_
	$self instvar ns_
	$self instvar Mtu_ Rtt_
	$self instvar verbose_
	$self instvar Hist_max_ hist_next_
	$self instvar High_const_
	$self instvar Unresp_droprate_factor_
	$self instvar Unresp_flowbw_factor_
	$self instvar PUFrac_

	set verbose_ 0	; #-1 means no messages
	set cbqlink_ $cbqlink
	set Rtt_ $rtt
	set Mtu_ $mtu
	set ns_ $ns

	set detect_pending_ false
	set reward_pending_ false
        set npenalty_ 0 
	set last_reward_ 0.0
	set last_detect_ 0.0
	set hist_next_ 0

	set Reward_interval_  5.0
	#
	# Set Detect_interval_ to INFINITY to turn off penalty box.
	#
	set Detect_interval_  5.0
#	set Detect_interval_  500.0
	set Hist_max_ 10
        set Safety_factor_ 1.2
        set Max_cbw_ 46750
        set Maxallot_ 0.98          
        set Mintime_ 0.5
	#set Unresp_droprate_factor_ 3
	set Unresp_droprate_factor_ 2
	set Unresp_flowbw_factor_ 0.8
	set PUFrac_ 0.98
	#
	# Set High_const_ to INFINITY to turn off HIGH-BANDWIDTH test.
	#
        set High_const_ 12000   
#       set High_const_ 1200000

	# don't schedule reward initially;  nobody in pbox yet
	if { $enable == "true" || $enable == 1 } {
		$self sched-detect
	} else {
		puts stderr "(router mechanisms disabled)"
	}
}      

RTMechanisms instproc test_friendly { flow flow_bw ref_bw } {
        $self instvar Safety_factor_
        if { $ref_bw != "none" && $flow_bw  > ($Safety_factor_ * $ref_bw) } {
		$self vprint 0 "FRIENDLY-TEST: FAILED (flow: $flow, ([$flow set flowid_]) fbw: $flow_bw, refbw: $ref_bw)"
                return "fail"
        }       
	$self vprint 1 "FRIENDLY-TEST: OK (flow: $flow ([$flow set flowid_]), fbw: $flow_bw, refbw: $ref_bw)"
        return "ok"
}    

RTMechanisms instproc test_unresponsive_initial { flow flow_bw droprate lastidx } {
	$self instvar Unresp_droprate_factor_
	$self instvar Unresp_flowbw_factor_
	$self instvar flowhist_

	set idx [$self fhist-mindroprate $flow]
	if { $idx >= 0 && $idx != $lastidx } {
		$self vprint 1 "UNRES-INIT: droprate: $droprate, past drop rate: $flowhist_($idx,droprate)"
		$self vprint 1 "UNRES-INIT: flow_bw: $flow_bw, past bw: $flowhist_($idx,bandwidth)"
		if { $droprate > [expr $Unresp_droprate_factor_ * \
		    $flowhist_($idx,droprate)] &&
		$flow_bw > [expr $Unresp_flowbw_factor_ * \
		    $flowhist_($idx,bandwidth)] } {

			if { $flowhist_($idx,name) != $flow } {
				error "unresp_init: flow wrong!"
			}
			$self vprint 0 "UNRESPONSIVE-TEST: FAILED (flow: $flow ([$flow set flowid_]) fbw: $flow_bw, droprate: $droprate"
			return "fail"
		}
	}
	$self vprint 1 "UNRESPONSIVE-TEST: OK (flow: $flow ([$flow set flowid_]) fbw: $flow_bw, droprate: $droprate"
	return "ok"
}

# is a flow unresponsive for a 2nd time
RTMechanisms instproc test_unresponsive_again { flow flow_bw droprate bwfrac drfrac } {
	$self instvar flowhist_ state_
	if { $flow_bw == "0" } {
		return "ok"
	}
	if { $flow_bw >= $bwfrac * $state_($flow,bandwidth) &&
		($droprate >= $drfrac * $state_($flow,droprate)) } {
		$self vprint 0 "UNRESP-AGAIN-TEST: FAILED (flow: $flow ([$flow set flowid_]) fbw: $flow_bw, droprate: $droprate"
		return "fail"
	}
	$self vprint 1 "UNRESP-AGAIN-TEST: OK (flow: $flow ([$flow set flowid_]) fbw: $flow_bw, droprate: $droprate"
	return "ok"
}


RTMechanisms instproc test_high { flow flow_bw droprate etime } {
	$self instvar okboxfm_
	$self instvar High_const_

	set numflows [llength [$okboxfm_ flows]]
	set gbarrivals [$okboxfm_ set barrivals_]
	set goodBps [expr $gbarrivals / $etime]
	set fBps [expr $goodBps / $numflows]
	if { $flow_bw > log(3*$numflows) * $fBps  &&
		$flow_bw * sqrt($droprate) > $High_const_ } {
		$self vprint 0 "HIGH-TEST: FAILED (flow: $flow ([$flow set flowid_]), fbw: $flow_bw, droprate: $droprate, etime: $etime)"
		return "fail"
	}
	$self vprint 1 "HIGH-TEST: OK (fbw: $flow_bw, droprate: $droprate, etime: $etime)"
	return "ok"
}

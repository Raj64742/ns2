#
# here are the main tests for detecting
#	unfriendly, unresponsive and high-bw flows
#
RTMechanisms instproc test_friendly { flow_bw ref_bw } {
        $self instvar Safety_factor_
        if { $flow_bw  > ($Safety_factor_ * $ref_bw) } {
                return "fail"
        }       
        return "ok"
}    

RTMechanisms instproc test_unresponsive_initial { flow flow_bw droprate lastidx } {
	$self instvar Unresp_droprate_factor_
	$self instvar Unresp_flowbw_factor_
	$self instvar flowhist_

	set idx [$self fhist-mindroprate $flow]
	if { $idx >= 0 && $idx != $lastidx &&
		$droprate > [expr $Unresp_droprate_factor_ *
		    $flowhist_($idx,droprate)] &&
		$flow_bw > [expr $Unresp_flowbw_factor_ *
		    $flowhist_($idx,bandwidth)] } {

		if { $flowhist_($idx,name) != $flow } {
			error "unresp_init: flow wrong!"
		}
		return "fail"
	}
	return "ok"
}

# is a flow unresponsive for a 2nd time
RTMechanisms instproc test_unresponsive_again { flow flow_bw droprate bwfrac drfrac } {
	$self instvar flowhist_
	if { $flow_bw == "0" } {
		return "ok"
	}
	if { $flow_bw >= $bwfrac * $state_($flow,bandwidth) &&
		($droprate >= $drfrac * $state_($flow,droprate)) } {
		return "fail"
	}
	return "ok"
}


RTMechanisms instproc test_high { flow_bw droprate etime } {
	$self instvar okboxfm_
	$self instvar High_const_
	$self instvar okmon_

	set numflows [$okboxfm_ fwdrops]
	set gbarrivals [$okmon_ barrivals]
	set goodBps [expr $gbarrivals / $etime]
	set fBps [expr $goodBps / $numflows]
	if { $flow_bw > log(3*$numflows) * $fBps  &&
		$flow_bw * sqrt($droprate) > $High_const_ } {
		return "fail"
	}
	return "ok"
}

#
# rtm_link.tcl
#	all the instproc's needed to set up the link structure
#	itself
#

# set up link classifier's slot table with 2 entries:
#	entry 0 : good box	[default]
#	entry 1 : penalty box
RTMechanisms instproc bindboxes {} {
	$self instvar cbqlink_
	$self instvar goodclass_ badclass_
	$self instvar goodslot_ badslot_

        set classifier [$cbqlink_ classifier]
	#$classifier dump
        set goodslot_ [$classifier installNext $goodclass_]
	set badslot_ [$classifier installNext $badclass_]
        $classifier set default_ $goodslot_
	$self vprint 2 "bindboxes: cbq classifier: $classifier, gslot: $goodslot_, bslot: $badslot_, defslot: $goodslot_"
}

RTMechanisms instproc set_red_params { redq psize qlim bytes wait } {
        $redq set mean_pktsize_ $psize
        $redq set limit_ $qlim
        $redq set bytes_ $bytes
        $redq set wait_ $wait
}    

#
# create the CBQ classes for the good box and penalty box
# insert them into the rtm link
#
RTMechanisms instproc makeboxes { okboxfm pboxfm qsz psz } {
	$self instvar cbqlink_
	$self instvar goodclass_ badclass_
	$self instvar okboxfm_ pboxfm_
	$self instvar ns_


        set cbq [$cbqlink_ queue]
        set rootcl [new CBQClass]

        set badclass_ [new CBQClass]
        set goodclass_ [new CBQClass]

	$self vprint 2 "makeboxes: bclass:$badclass_, gclass:$goodclass_"

        set badq [new Queue/RED]
        $badq link [$cbqlink_ link]

        set goodq [new Queue/RED]
        $goodq link [$cbqlink_ link]

        $self set_red_params $badq $psz $qsz true false
        $self set_red_params $goodq $psz $qsz true false

        $badclass_ install-queue $badq
        $goodclass_ install-queue $goodq

	$goodclass_ setparams $rootcl true 0.98 0.004 1 1 0
	$badclass_ setparams $rootcl true 0.0 0.004 1 1 0
# WHY DO NO PACKETS GET SENT WHEN BADCLASS has borrowing set to FALSE??? 
# ../../ns rtm_reclass.tcl three net3

        $rootcl setparams none true 0.98 0.004 1 1 0

	set okboxfm_ $okboxfm
	set pboxfm_ $pboxfm

        $cbqlink_ insert $rootcl
        $cbqlink_ insert $badclass_ $pboxfm_
        $cbqlink_ insert $goodclass_ $okboxfm_

	# put in the edrop stuff
	$cbqlink_ instvar drophead_

	set edsnoop [new SnoopQueue/EDrop]
	$edsnoop set-monitor $pboxfm_
	$edsnoop target $drophead_
	$badq early-drop-target $edsnoop

	set edsnoop [new SnoopQueue/EDrop]
	$edsnoop set-monitor $okboxfm_
	$edsnoop target $drophead_
	$goodq early-drop-target $edsnoop

	$self vprint 2 "makeboxes completing: okfm: $okboxfm, okfmcl: [$okboxfm classifier], okredQ: $goodq; pboxfm: $pboxfm, pboxfmcl: [$pboxfm classifier], pboxredQ: $badq"
}

#
# create a flow monitor
RTMechanisms instproc makeflowmon {} {
	$self instvar ns_ okboxfm_ pboxfm_

	set flowmon [new QueueMonitor/ED/Flowmon]
        set cl [new Classifier/Hash/SrcDestFid 33]

        set pbody {
		set ns [$rtm_ set ns_]
		set okcl [[$rtm_ set okboxfm_] classifier]
                ## puts "here, okcl: $okcl self: $self"
		if { $okcl == $self } {
			# see if this flow moved to the pbox
			set pboxcl [[$rtm_ set pboxfm_] classifier]
			set moved [$pboxcl lookup auto $src $dst $fid]
		} else {
			# see if this flow moved to the okbox
			set okboxcl [[$rtm_ set okboxfm_] classifier]
			set moved [$okboxcl lookup auto $src $dst $fid]
		}
		if { $moved != "" } {
			# residual packet belonging to a moved flow
			return
		}
                set fdesc [new QueueMonitor/ED/Flow]
                set slot [$self installNext $fdesc]
		$rtm_ vprint 2 "(self:$self) installing flow $fdesc (s:$src,d:$dst,f:$fid) in buck: ?, slot >$slot<"
                $self set-hash auto $src $dst $fid $slot
		$rtm_ vprint 2 "(self: $self) unknown-flow done"
flush stdout
        }
	set pbody "set rtm_ $self ; $pbody"

        $cl proc unknown-flow { src dst fid } $pbody
	$cl proc no-slot slotnum {
		#
		# note: we can wind up here when a packet passes
		# through either an Out or a Drop Snoop Queue for
		# a queue that the flow doesn't belong to anymore.
		# For exampe, if a flow is penalized, future packets
		# are directed to a "penalty box" queue, but there
		# may be previos packets still in the good box queue
		# which are allowed to either depart or are dropped.
		# Since there is no longer hash state in the good box's
		# hash classifier, we get a -1 return value for the
		# hash classifier's classify() function, and there
		# is no node at slot_[-1].  What to do about this?
		# Well, we are talking about flows that have already
		# been moved and so should rightly have their stats
		# zero'd anyhow, so for now just ignore this case..
		# puts "classifier $self, no-slot for slotnum $slotnum"
	}
        $flowmon classifier $cl
        return $flowmon
}

RTMechanisms instproc monitor-link {} {
	$self instvar ns_ cbqlink_
	set flowmon [new QueueMonitor/ED/Flowmon]
	set cl [new Classifier/Hash/SrcDestFid 33]
	$flowmon classifier $cl
	$cl proc unknown-flow { src dst fid } {
		set nflow [new QueueMonitor/ED/Flow]
		set slot [$self installNext $nflow]
		## puts "here1"
		$self set-hash auto $src $dst $fid $slot
	}
	$cl proc no-slot slotnum {
		puts stderr "classifier $self, no-slot for slotnum $slotnum"
	}
	$cbqlink_ attach-monitors [new SnoopQueue/In] [new SnoopQueue/Out] \
		[new SnoopQueue/Drop] $flowmon
	return $flowmon
}

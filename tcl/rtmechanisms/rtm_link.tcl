#
# rtm_link.tcl
#	all the instproc's needed to set up the link structure
#	itself
#

RTMechanisms instproc bindboxes {} {
	$self instvar cbqlink_
	$self instvar goodclass_ badclass_
	$self instvar goodslot_ badslot_

        set classifier [$cbqlink_ classifier]
        set goodslot_ [$classifier installNext $goodclass_]
	set badslot_ [$classifier installNext $badclass_]
        $classifier set default_ $goodslot_
	$self vprint "bindboxes: cbq classifier: $classifier, gslot: $goodslot_, bslot: $badslot_, defslot: $goodslot_"
}

RTMechanisms instproc set_red_params { redq psize qlim bytes wait } {
        $redq set mean_pktsize_ $psize
        $redq set limit_ $qlim
        $redq set bytes_ $bytes
        $redq set wait_ $wait
}    

RTMechanisms instproc makeboxes { okboxfm pboxfm qsz psz } {
	$self instvar cbqlink_
	$self instvar goodclass_ badclass_
	$self instvar okboxfm_ pboxfm_

	$self vprint "makeboxes: okfm: $okboxfm okfmcl: [$okboxfm classifier], pboxfm: $pboxfm, pboxfmcl: [$pboxfm classifier]"

        set cbq [$cbqlink_ queue]
        set rootcl [new CBQClass]

        set badclass_ [new CBQClass]
        set goodclass_ [new CBQClass]

	$self vprint "makeboxes: bclass:$badclass_, gclass:$goodclass_"

        set badq [new Queue/RED]
        $badq link [$cbqlink_ link]

        set goodq [new Queue/RED]
        $goodq link [$cbqlink_ link]

        $self set_red_params $badq $psz $qsz true false
        $self set_red_params $goodq $psz $qsz true false

        $badclass_ install-queue $badq
        $goodclass_ install-queue $goodq

        $badclass_ setparams $rootcl true 0.0 0.004 1 1 0
        $goodclass_ setparams $rootcl true 0.98 0.004 1 1 0
        $rootcl setparams none true 0.98 0.004 1 1 0

	set okboxfm_ $okboxfm
	set pboxfm_ $pboxfm

        $cbqlink_ insert $rootcl
        $cbqlink_ insert $badclass_ $pboxfm_
        $cbqlink_ insert $goodclass_ $okboxfm_

	# put in the edrop stuff
	$cbqlink_ instvar snoopDrop_
	if { [info exists snoopDrop_] } {
		set ldrop $snoopDrop_
	} else {
		set ldrop [$ns_ set nullAgent_]
	}
	set edsnoop [new SnoopQueue/EDrop]
	$edsnoop set-monitor $pboxfm_
	$edsnoop target $ldrop
	$badq early-drop-target $edsnoop

	set edsnoop [new SnoopQueue/EDrop]
	$edsnoop set-monitor $okboxfm_
	$edsnoop target $ldrop
	$goodq early-drop-target $edsnoop
}

RTMechanisms instproc makeflowmon {} {
	$self instvar ns_ okboxfm_ pboxfm_

	set flowmon [new QueueMonitor/ED/Flowmon]
        set cl [new Classifier/Hash/SrcDestFid 33]

        set pbody {
		set ns [$rtm_ set ns_]
		set okcl [[$rtm_ set okboxfm_] classifier]
		if { $okcl == $self } {
			# see if this flow moved to the pbox
			set pboxcl [[$rtm_ set pboxfm_] classifier]
			set moved [$pboxcl lookup $hashbucket $src $dst $fid]
		} else {
			# see if this flow moved to the okbox
			set okboxcl [[$rtm_ set okboxfm_] classifier]
			set moved [$okboxcl lookup $hashbucket $src $dst $fid]
		}
		if { $moved != "" } {
			# residual packet belonging to a moved flow
			return
		}
                set fdesc [new QueueMonitor/ED/Flow]
                set slot [$self installNext $fdesc]
puts "[$ns now] (self:$self) installing flow $fdesc (s:$src,d:$dst,f:$fid) in buck: $hashbucket, slot >$slot<"
                $self set-hash $hashbucket $src $dst $fid $slot
puts "[$ns now] (self: $self) unknown-flow done"
flush stdout
        }
#	set pbody \
#	    "set ns_ $ns_ ; set pboxfm_ $pboxfm_ ; set okboxfm_ $okboxfm_ $pbody"
	set pbody "set rtm_ $self ; $pbody"

        $cl proc unknown-flow { src dst fid hashbucket } $pbody
	$cl proc no-slot slotnum {
	}
        $flowmon classifier $cl
        return $flowmon
}

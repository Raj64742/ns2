#
# rtm_link.tcl
#	all the instproc's needed to set up the link structure
#	itself
#

RTMechanisms instproc bindboxes {} {
	$self instvar cbqlink_
	$self instvar goodclass_ badclass_

        set classifier [$cbqlink_ classifier]
        set goodslot [$classifier installNext $goodclass_]
	$classifier installNext $badclass_
        $classifier set default_ $goodslot
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

        set cbq [$cbqlink_ queue]
        set rootcl [new CBQClass]

        set badclass_ [new CBQClass]
        set goodclass_ [new CBQClass]

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
}

RTMechanisms instproc makeflowmon {} {
	set flowmon [new QueueMonitor/ED/Flowmon]
        set cl [new Classifier/Hash/SrcDestFid 33]
        $cl proc unknown-flow { src dst fid hashbucket } {
                global ns
                set fdesc [new QueueMonitor/ED/Flow]
                set slot [$self installNext $fdesc]
puts "[$ns now]: (self:$self) installing flow $fdesc (s:$src,d:$dst,f:$fid) in b
uck: $hashbucket, slot >$slot<"
                $self set-hash $hashbucket $src $dst $fid $slot
        }

        $flowmon classifier $cl
        return $flowmon
}

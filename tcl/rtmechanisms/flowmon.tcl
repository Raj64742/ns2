# make a flow monitor
proc makeflowmon {} {
    global ns
    set flowmon [new QueueMonitor/ED/Flowmon]
    set cl [new Classifier/Hash/SrcDestFid 33]
    
    $cl proc unknown-flow { src dst fid } {
	set fdesc [new QueueMonitor/ED/Flow]
	set slot [$self installNext $fdesc]
	$self set-hash auto $src $dst $fid $slot
    }
    $cl proc no-slot slotnum {
    }
    $flowmon classifier $cl
    return $flowmon
}

# attach a flow monitor to a link
proc attach-fmon {lnk fm} {
    global ns
    set isnoop [new SnoopQueue/In]
    set osnoop [new SnoopQueue/Out]
    set dsnoop [new SnoopQueue/Drop]
    $lnk attach-monitors $isnoop $osnoop $dsnoop $fm
    set edsnoop [new SnoopQueue/EDrop]
    $edsnoop set-monitor $fm
    [$lnk queue] early-drop-target $edsnoop
    $edsnoop target [$ns set nullAgent_]
}

source ../../../ex/asym/util.tcl

Class WebCS

proc createTransfer {ns src dst tcpargs tcptrace sinkargs session sessionargs doneargs xfersz} {
	set tcp [eval createTcpSource $tcpargs]
	$tcp proc done {} " $doneargs"
	if {$xfersz < 1} {
		$tcp set packetSize_ [expr int($xfersz*1000)]
	}
	set sink [eval createTcpSink $sinkargs]
	set ftp [createFtp $ns $src $tcp $dst $sink]
	if {$session} {
		eval setupTcpSession $tcp $sessionargs
	}
	setupTcpTracing $tcp $tcptrace $session
	return $ftp
}

proc numberOfImages {} {
	return 4
}

proc htmlRequestSize {} {
	return 0.3
}

proc htmlReplySize {} {
	return 10
}

proc imageRequestSize {} {
	return 0.3
}

proc imageResponseSize {} {
	return 10
}

WebCS instproc init {ns client server tcpargs tcptrace sinkargs {phttp false} {session false} {sessionargs ""}} {
	$self instvar ns_ 
	$self instvar ftpCS1_ ftpCS2_ 
	$self instvar ftpSC1_ ftpSC2_
	$self instvar numImg_ htmlReqsz_ htmlReplsz_ imgReqsz_ imgReplsz_
	$self instvar numImgRecd_ numImgRepl_
	$self instvar starttime_ endtime_
	
	set ns_ $ns
	# compute transfer sizes
	set numImg_ [numberOfImages]
	set htmlReqsz_ [htmlRequestSize]
	set htmlReplsz_ [htmlReplySize]
	set imgReqsz_ [imageRequestSize]
	set imgReplsz_ [imageResponseSize]

	# create "ftp's" for each transfer
	set ftpCS1_ [createTransfer $ns $client $server "TCP/Newreno" $tcptrace $sinkargs $session $sessionargs "$self htmlReqDone" $htmlReqsz_]
	set ftpSC1_ [createTransfer $ns $server $client $tcpargs $tcptrace $sinkargs $session $sessionargs "$self htmlReplDone" $htmlReplsz_]
	for {set i 0} {$i < $numImg_} {incr i 1} {
		set ftpCS2_($i) [createTransfer $ns $client $server "TCP/Newreno" $tcptrace $sinkargs $session $sessionargs "$self imgReqDone" $imgReqsz_]
	}
	for {set i 0} {$i < $numImg_} {incr i 1} {
		if {$phttp} {
			if {$i == 0} {
				set ftpSC2_($i) [createTransfer $ns $server $client $tcpargs $tcptrace $sinkargs $session $sessionargs "$self imgReplDone" $imgReplsz_]
			} else {
				set ftp_sc2_($i) $ftp_sc2_(0)
			}
		} else {
			set ftpSC2_($i) [createTransfer $ns $server $client $tcpargs $tcptrace $sinkargs $session $sessionargs "$self imgReplDone" $imgReplsz_]
		}
	}
}

WebCS instproc start {} {
	$self instvar ns_ 
	$self instvar ftpCS1_ ftpCS2_ 
	$self instvar ftpSC1_ ftpSC2_
	$self instvar numImg_ htmlReqsz_ htmlReplsz_ imgReqsz_ imgReplsz_
	$self instvar numImgRecd_ numImgRepl_
	$self instvar starttime_ endtime_

	set starttime_ [$ns_ now]
	set numImgRecd_ 0
	set numImgRepl_ 0
	if {$htmlReqsz_ < 1} {
		$ftpCS1_ producemore 1
	} else {
		$ftpCS1_ producemore $htmlReqsz_
	}
}

WebCS instproc htmlReqDone {} {
	$self instvar ns_ 
	$self instvar ftpCS1_ ftpCS2_ 
	$self instvar ftpSC1_ ftpSC2_
	$self instvar numImg_ htmlReqsz_ htmlReplsz_ imgReqsz_ imgReplsz_
	$self instvar numImgRecd_ numImgRepl_
	$self instvar starttime_ endtime_

	$ftpSC1_ producemore $htmlReplsz_
}

WebCS instproc htmlReplDone {} {
	$self instvar ns_ 
	$self instvar ftpCS1_ ftpCS2_ 
	$self instvar ftpSC1_ ftpSC2_
	$self instvar numImg_ htmlReqsz_ htmlReplsz_ imgReqsz_ imgReplsz_
	$self instvar numImgRecd_ numImgRepl_
	$self instvar starttime_ endtime_

	for {set i 0} {$i < $numImg_} {incr i 1} {
		if {$imgReqsz_ < 1} {
			$ftpCS2_($i) producemore 1
		} else {
			$ftpCS2_($i) producemore $imgReqsz_
		}
	}
}

WebCS instproc imgReqDone {} {
	$self instvar ns_ 
	$self instvar ftpCS1_ ftpCS2_ 
	$self instvar ftpSC1_ ftpSC2_
	$self instvar numImg_ htmlReqsz_ htmlReplsz_ imgReqsz_ imgReplsz_
	$self instvar numImgRecd_ numImgRepl_
	$self instvar starttime_ endtime_

	$ftpSC2_($numImgRepl_) producemore $imgReplsz_
	incr numImgRepl_
}
	
	
WebCS instproc imgReplDone {} {
	$self instvar ns_ 
	$self instvar ftpCS1_ ftpCS2_ 
	$self instvar ftpSC1_ ftpSC2_
	$self instvar numImg_ htmlReqsz_ htmlReplsz_ imgReqsz_ imgReplsz_
	$self instvar numImgRecd_ numImgRepl_
	$self instvar starttime_ endtime_

	incr numImgRecd_
	if {$numImgRecd_ == $numImg_} {
		$self end
	}
}

WebCS instproc end {} {
	$self instvar ns_ 
	$self instvar ftpCS1_ ftpCS2_ 
	$self instvar ftpSC1_ ftpSC2_
	$self instvar numImg_ htmlReqsz_ htmlReplsz_ imgReqsz_ imgReplsz_
	$self instvar numImgRecd_ numImgRepl_
	$self instvar starttime_ endtime_

	set endtime_ [$ns_ now]
	puts -nonewline " [format " %.3f" [expr $endtime_ - $starttime_]]"
	$ns_ at [expr [$ns_ now] + 3] "$self start"
}


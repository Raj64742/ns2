TestSuite instproc June01defaults {} {
	 Agent/TCP set windowInit_ 2
	 Agent/TCP set singledup_ 1
	 Agent/TCP set minrto_ 1
	 Agent/TCP set syn_ true
	 Agent/TCP set delay_growth_ true
}

TestSuite instproc dropPkts { link fid pkts } {
    set emod [new ErrorModule Fid]
    set errmodel1 [new ErrorModel/List]
    $errmodel1 droplist $pkts
    $link errormodule $emod
    $emod insert $errmodel1
    $emod bind $errmodel1 $fid
}


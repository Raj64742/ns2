Class Traffic/Mix -superclass TestSuite
TestSuite instproc trafficMix { { notused 0 } } {
    $self instvar node_
    
    set fid 1
    for {set i 0} {$i< 3} {incr i} {
	for {set j 0} {$j< 3} {incr j} {
	    #$self new_Tcp $startTime $src $dst $rwnd $fid $dump $pktSize $type $maxPkts 
	    $self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 1000 sack 0
	    incr fid
# 	    $self new_Tcp [$self rnd 10] $node_(s$i) $node_(s$j) 50 $fid 1 1000 sack 0
# 	    incr fid
# 	    $self new_Tcp [$self rnd 10] $node_(s$i) $node_(s$j) 50 $fid 1 1000 sack 0
# 	    incr fid
	    #reverse
#	    $self new_Tcp [$self rnd 10] $node_(s$j) $node_(s$i) 50 $fid 0 1000 sack 0
#	    incr fid
	}
    }

    #50% of the link bandwidth
#    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .0016 $fid 0   
#    incr fid

#    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 500 .0008 $fid 0   
#    incr fid
	
    #45% of the link
#    $self new_Cbr [$self rnd 10] $node_(s1) $node_(d1) 1000 .0018 $fid 0   
#    incr fid

    #40% of the link
#    $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 1000 .0021 $fid 0   
#    incr fid

    #35% of the link bandwidth
#    $self new_Cbr [$self rnd 10] $node_(s2) $node_(s5) 1000 .0023 $fid 0   
#    incr fid 

    #30% of the link bandwidth
#    $self new_Cbr [$self rnd 10] $node_(s1) $node_(d1) 1000  .0026 $fid 0   
#    incr fid 

    #20% of the link bandwidth
#     $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 1000 .004 $fid 0   
#     incr fid
    
#     $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 1000 .004 $fid 0   
#     incr fid
    
#     $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 500 .002 $fid 0   
#     incr fid

#     $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 500 .002 $fid 0   
#     incr fid
   
    #15% of the link bandwidth
#     $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 1000 .0052 $fid 0   
#     incr fid
#     $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 1000 .0052 $fid 0   
#     incr fid
#    $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 1000 .0052 $fid 0   
#    incr fid
  
  
    #10% of the link bandwidth
#     $self new_Cbr [$self rnd 10] $node_(s2) $node_(d2) 1000 .008 $fid 0   
#     incr fid
#     $self new_Cbr [$self rnd 10] $node_(s3) $node_(s6) 1000 .008 $fid 0   
#     incr fid
#     $self new_Cbr [$self rnd 10] $node_(s3) $node_(s6) 1000 .008 $fid 0   
#     incr fid
#     $self new_Cbr [$self rnd 10] $node_(s3) $node_(s6) 1000 .008 $fid 0   
#     incr fid
#     $self new_Cbr [$self rnd 10] $node_(s3) $node_(s6) 1000 .008 $fid 0   
#     incr fid
#     $self new_Cbr [$self rnd 10] $node_(s3) $node_(s6) 1000 .008 $fid 0   
#     incr fid
    
    #2% of link bandwidth
#    $self new_Cbr [$self rnd 10] $node_(s3) $node_(s6) 1000 .04 $fid 0   
#    incr fid
    
    #4% of link bandwidth
#    $self new_Cbr [$self rnd 10] $node_(s3) $node_(s6) 1000 .02 $fid 0   
#    incr fid

    #varying cbr 0.25*f(R,p) -> 4*f(R,p)
     $self new_VaryingCbr [$self rnd 10] $node_(s2) $node_(d2) 1000 0.02262742 $fid 0   
	
}

Class Traffic/AllTCP -superclass TestSuite
TestSuite instproc trafficAllTCP { { notused 0 } } {
    $self instvar node_
    
    set fid 1
    for {set i 0} {$i< 4} {incr i} {
	for {set j 0} {$j< 2} {incr j} {
	    #$self new_Tcp $startTime $src $dst $rwnd $fid $dump $pktSize $type $maxPkts 
	    $self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 1000 sack 0
	    incr fid
	    if {$i == 3 } {
		$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 1000 sack 0
		incr fid
		$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 1000 sack 0
		incr fid
		$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 1000 sack 0
		incr fid
# 		$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 1000 sack 0
# 		incr fid
# 		$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 1000 sack 0
# 		incr fid
	    }
#	    $self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 1000 sack 0
#	    incr fid
# 	    $self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 1000 sack 0
# 	    incr fid
# 	    $self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$j) 50 $fid 1 1000 sack 0
# 	    incr fid
	}
    }
}

Class Traffic/AllUDP -superclass TestSuite
TestSuite instproc trafficAllUDP { { notused 0 } } {

    $self instvar node_ 
    
    set fid 1
    
    #1% of the link 
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .08 $fid 0   
    incr fid 	

    #5% of the link 
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .016 $fid 0   
    incr fid 
    
    #10% of the link bandwidth
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .008 $fid 0   
    incr fid 
    
    #15% of the link bandwidth
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .0053 $fid 0   
    incr fid

    #20% of the link bandwidth
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .004 $fid 0   
    incr fid 
    
    #25% of the link bandwidth
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .0032 $fid 0   
    incr fid 
    
    #30% of the link bandwidth
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .0026 $fid 0   
    incr fid 
    
    #35% of the link bandwidth
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .0023 $fid 0   
    incr fid 
	
    #40% of the link
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .002 $fid 0   
    incr fid

    #45% of the link
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .0018 $fid 0   
    incr fid

    #50% of the link
    $self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .0016 $fid 0   
    incr fid
}

# 5 TCP flows. 
Class Traffic/TestIdent -superclass TestSuite
TestSuite instproc trafficTestIdent { { notused 0 } } {

    $self instvar node_ 
    
    set fid 1
    for {set i 0} {$i< 5} {incr i} {
	#$self new_Tcp $startTime $src $dst $rwnd $fid $dump $pktSize $type $maxPkts 
	$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d0) 50 $fid 1 1000 sack 0
	incr fid

	#sending rate about equal to frp
#	$self new_Cbr [$self rnd 10] $node_(s0) $node_(d0) 1000 .007 $fid 0   
#	incr fid

    }
}

Class Traffic/Web -superclass TestSuite
TestSuite instproc trafficWeb { { notused 0 } } {

    $self instvar node_
    
    Agent/TCP set packetSize_ 500
    Agent/UDP set packetSize_ 500
    
    PagePool/WebTraf set TCPTYPE_ Sack1
    PagePool/WebTraf set TCPSINKTYPE_ TCPSink/Sack1
    PagePool/WebTraf set FID_ASSIGNING_MODE_ 1

    set fid 1
    
    for {set i 0} {$i<5} {incr i} {
	#$self new_Tcp $startTime $src $dst $rwnd $fid $dump $pktSize $type $maxPkts
	$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d$i) 50 $fid 1 500 sack 0
	incr fid
	
	$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d[expr ($i+1)%5]) 50 $fid 1 500 sack 0
	incr fid
	
	#reverse
	# $self new_Tcp [$self rnd 10] $node_(s$j) $node_(s$i) 50 $fid 0 1000 sack 0
	# incr fid
    }
	
    #20% of the link bandwidth
    $self new_Cbr [$self rnd 10] $node_(s3) $node_(d3) 500 .002 $fid 0   
    incr fid 
    
    set pool [new PagePool/WebTraf]
    $pool instvar maxFid_
	
    set maxFid_ $fid

    #something like this should have been there 
    # instead of the ugly hack I put in place in tcl/webcache/webtraf.tcl
    # WebPage set LASTOBJ_ 10

    $pool set-num-client 5
    $pool set-num-server 5

    for {set i 0} {$i < 5} {incr i} {
	$pool set-server $i $node_(s$i)
    }

    for {set i 0} {$i < 5} {incr i} {
	$pool set-client $i $node_(d$i)
    }

    set numSession 50
    $pool set-num-session $numSession
    
    set interSession [new RandomVariable/Exponential]
    $interSession set avg_ 1
    
    set sessionSize [new RandomVariable/Constant]
    $sessionSize set val_ 100
    
    set launchTime 0
    for {set i 0} {$i < $numSession} {incr i} {
	
	    #number of pages in a session
	    set numPage [$sessionSize value]
	    
	    #time between requesting various pages
	    set interPage [new RandomVariable/Exponential]
	    $interPage set avg_ 10
	    #set interPage [new RandomVariable/Constant]
	    #$interPage set val_ 10
	    
	    # number of objects in a page
	    set pageSize [new RandomVariable/Exponential]
	    $pageSize set avg_ 10
	    #set pageSize [new RandomVariable/Constant]
	    #$pageSize set val_ 10
	    
	    # the time between requesting various objects
	    set interObj [new RandomVariable/Exponential]
	    $interObj set avg_ 0.01
	    #set interObj [new RandomVariable/Constant]
	    #$interObj set val_ 0.01
	    
	    #the size of objects being requested
	    set objSize [new RandomVariable/ParetoII]
	    $objSize set avg_ 24
	    $objSize set shape_ 1.2
	    #set objSize [new RandomVariable/Constant]
	    #$objSize set val_ 24
	    
	    
	    $pool create-session $i $numPage [expr $launchTime + 0.1] \
		$interPage $pageSize $interObj $objSize
	    set launchTime [expr $launchTime + [$interSession value]]
	    
    }


}

Class Traffic/Multi -superclass TestSuite
TestSuite instproc trafficMulti {noLinks_} {
	$self instvar node_ 
	
	set fid 1

	for {set link 0} {$link < $noLinks_} {incr link} {
	    for {set i 0} {$i < 2} {incr i} {
		
		$self new_Tcp [$self rnd 10] $node_(s$link) $node_(d$link) 50 $fid 1 1000 sack 0
		incr fid
		
 		$self new_Tcp [$self rnd 10] $node_(s1$link) $node_(d$link) 50 $fid 1 1000 sack 0
 		incr fid

 		$self new_Tcp [$self rnd 10] $node_(s2$link) $node_(d$link) 50 $fid 1 1000 sack 0
 		incr fid

		$self new_Tcp [$self rnd 10] $node_(s3$link) $node_(d$link) 50 $fid 1 1000 sack 0
		incr fid
#		$self new_Tcp [$self rnd 10] $node_(s3$link) $node_(d$link) 50 $fid 1 1000 sack 0
#		incr fid
		
#		$self new_Cbr [$self rnd 10] $node_(s$src) $node_(s$dst) 1000 .08 $fid 0   
#		incr fid
	    }
			
 	    for {set i 0} {$i < 2} {incr i} {
		#5.0 Mbps
 		$self new_Cbr [$self rnd 10] $node_(s$link) $node_(d$link) 1000 .0016 $fid 0   
 		incr fid
 	    }
	}
	
	$self new_Cbr [$self rnd 10] $node_(source) $node_(sink) 1000 .0088 $fid 0   ;#0.909Mbps
#	$self new_Tcp [$self rnd 10] $node_(source) $node_(sink) 50 $fid 1 1000 sack 0
	incr fid
}

Class Traffic/TFRC -superclass TestSuite
TestSuite instproc trafficTFRC { { numFlowPairs 2 } } {
    $self instvar node_

    set fid 1
    for {set i 0} {$i<$numFlowPairs} {incr i} {
	set k [ expr $i % 2 ]
	for {set j 0} {$j<1} {incr j} {
	    #new_TFRC startTime source dest fid pktSize
	    $self new_TFRC [$self rnd 10] $node_(s$k) $node_(d$j) $fid 1000 
	    incr fid
	
	    $self new_Tcp [$self rnd 10] $node_(s$k) $node_(d$j) 50 $fid 1 1000 sack 0
	    incr fid

	}
    }
}

Class Traffic/TestFRp -superclass TestSuite
TestSuite instproc trafficTestFRp { { notused 0 } } {

    $self instvar node_ topo_
    $topo_ instvar redpdq_

    set fid 1

#     for {set i 0} {$i<6} {incr i} {
# 	$self new_Tcp [$self rnd 10] $node_(s$i) $node_(d0) 50 $fid 1 1000 sack 0
# 	incr fid
#     }

    set p [$redpdq_ set P_testFRp_]
    set R 0.040
    set frp [expr sqrt(1.5)/($R*sqrt($p))]
    set rate [expr {1.0/(3.5*$frp)}]

    $self new_Cbr [$self rnd 1] $node_(s0) $node_(d0) 1000 $rate $fid 0   
    incr fid

    #varying cbr 30->15->7.5->15->30. changes every 50s starting at 30s.
#     $self new_VaryingCbr [$self rnd 10] $node_(s0) $node_(d0) 1000  0.01306395 $fid 0 
#     incr fid

#     $self new_VaryingCbr [$self rnd 10] $node_(s0) $node_(d0) 1000 0.02612789 $fid 0 
#     incr fid
}

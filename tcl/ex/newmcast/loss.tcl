# the following complements the TraceErrorModel class in C++
MrouteErrorModel instproc lossFile { fname } {
	$self instvar filename fhandle
	set filename $fname
	set fhandle [open $filename r]
	$self read
}

MrouteErrorModel instproc read { } {
	$self instvar fhandle good_ loss_
	if { [set line [gets $fhandle]] != -1 && $line != "" } {
		set loss_ [lindex $line 0]
		set good_ [lindex $line 1]
	}
	$self instvar filename
}


Class LossMgr 

LossMgr instproc init { sim mlink } {
	$self next
	$self instvar ns
	set ns $sim
	$self lossylan $mlink
}

LossMgr instproc lossylan { mlink } {
	$self instvar ns loss nodes
	set nodes [$mlink set nodes_]
# XXX assume traceAllFile in ns already exists for now.. !
	# set dropFile [$ns set traceAllFile]
	set dropFile [$ns set namtraceAllFile_]
	foreach n1 $nodes {
	    set did [$n1 id]
	    foreach n2 $nodes {
		set sid [$n2 id]
		if { $sid != $did } {
		    set link [$ns getlink $sid $did]

		    set loss($sid:$did) [new MrouteErrorModel]
# see ns-mlink.tcl DummyLink addloss
		    $link addloss $loss($sid:$did)

		    # set enqT [$ns create-trace enque $dropFile $n2 $n1]
		    # set drpT [$ns create-trace Drop $dropFile $n2 $n1]
		    set drpT [$ns create-trace Drop $dropFile $mlink $n1 nam]
		    $loss($sid:$did) drop-target $drpT
		    # $enqT target $drpT
	            $drpT target [$ns set nullAgent_]
		}
	    }
	}
}

LossMgr instproc assignLoss { node1 node2 filename cls } {
	$self instvar loss ns
	set id1 [$node1 id]
	set id2 [$node2 id]

	$loss($id1:$id2) lossFile $filename
# instead of class... set the message type... 
#	$loss($id1:$id2) set class_ $cls
# hack XXX set the message type accdg to mapping... yuck !!
# the mapping list for detailedDM XXX
#	prune 30, graft 31, graft-ack 32, join 33, assert 34
	switch $cls {
		30 { set msg prune }
		31 { set msg graft }
		32 { set msg graft-ack }
		33 { set msg join }
		34 { set msg assert }
		default { puts "unknown type $cls"; return 0 }
	}
	$loss($id1:$id2) drop-packet $msg
# note that the pim-sm stuff may not work anymore... also it
# may not work with the new MrouteErrorModel that checks
# the prune header type field !!
}

LossMgr instproc exhaustive-loss {} {
	$self instvar nodes
	set num [llength $nodes]
	$self exhaust-loss [expr $num - 1]
}

LossMgr instproc exhaust-loss { Num } {
   for { set i 0 } { $i < $Num } { incr i } {
	set loss($i) 0
	set good($i) 0
   }
   set n [expr pow(2,$Num)]
   for { set index 0 } { $index < $Num } { incr index } {
   	for { set i 0 } { $i < $n } { incr i } {
	   if { [expr 1 << $index] & $i } {
		if $loss($index) {
			lappend lossList($index) $loss($index)
			set loss($index) 0
		}
		incr good($index)
	   } else {
		if $good($index) {
			lappend lossList($index) $good($index)
			set good($index) 0
		}
		incr loss($index)
	   }
	}
	lappend lossList($index) $good($index)
   }
   for { set index 0 } { $index < $Num } { incr index } {
	set f$index [open loss-trace-$index w]
	set lossflg 1
	for { set cnt 0 } { $cnt < [llength $lossList($index)] } { incr cnt } {
	  eval "puts -nonewline \$f$index [lindex $lossList($index) $cnt]"
	  eval "puts -nonewline \$f$index \" \""
	  if $lossflg { 
		set lossflg 0 
	  } else {
	    	set lossflg 1
		eval "puts -nonewline \$f$index \"\\n\""
		eval "puts -nonewline \"\\n\""
	  }
	}
	eval "flush \$f$index"
	eval "close \$f$index"
   }
}

LossMgr instproc programLoss { fl cls } {
	$self instvar nodes
	set fout "types.out"
	if { [file exists $fout] } {
		exec rm $fout
	}
# XXX make sure you have this script.. !!
	exec awk -f types.awk $fl > $fout
	set f [open $fout r]
	set line 0
	set doneList ""

	set power [expr [llength $nodes] - 1]
	set factor [expr pow(2,$power)]

	set totalNum 0
	set scenarioNum 0

	while { [set line [gets $f]] != -1 && $line != "" } {	  
	  set id1 [lindex $line 0]
	  # XXX assume only lan now
	  if { [$self belongsTo $id1 $doneList] } {
		continue
	  }
	  set id2 [lindex $line 1]
	  set class_ [lindex $line 2]
	  set num [lindex $line 3]
	  if { $cls != $class_ } {
		continue
	  }
	  # check if the nodes belong to the LAN,... a nam hack to put
	  # a very large lan id as dst
	  if { [$self belongsTo $id1 $nodes] &&
		([$self belongsTo $id2 $nodes] || $id2 > 1000000) } {
		set index 0

		set totalNum [expr $totalNum + $num]

		set n1 [$self getNode $id1]
		lappend doneList $n1
		foreach n $nodes {
		   if { $n != $n1 } {
		      set fname "loss-trace-$index"
		      set suffix "$id1-[$n id]"
		      if { [file exists $fname-$suffix] } {
		          exec rm $fname-$suffix
		      }
		      if { $num == 1 } {
			  set fname2 [$self extendScenario $fname $scenarioNum $suffix]
			  $self assignLoss $n1 $n $fname2 $cls
		      } else {
		       for { set i 0 } { $i < $num } { incr i } {
			set before $i
			set after [expr $num - $before - 1]
			set fname2 [$self extendLoss $fname $scenarioNum $before $after $suffix]
		       }
		       $self assignLoss $n1 $n $fname2 $cls
		      }
		      incr index
		   }
		}
		set scenarioNum [expr $num * $factor + $scenarioNum]
	  }
	}

	set totalNum [expr $totalNum * $factor]
	return $totalNum
}

LossMgr instproc extendScenario { fname scenario index } {
	if { ! $scenario } {
		return $fname
	}
	set f [open $fname r]
	set fileout "$fname-$index"
	set fout [open $fileout w]
	puts $fout "0 $scenario"
        while { [set line [gets $f]] != -1 && $line != "" } {
		puts $fout "$line"
	}
        close $f
        close $fout
        return $fileout
}

LossMgr instproc extendLoss { fname scenario before after index } {
	set f [open $fname r]
	set fileout "$fname-$index"
	set fout [open $fileout a]
	if $scenario {
		puts $fout "0 $scenario"
	}
	while { [set line [gets $f]] != -1 && $line != "" } {
		set loss [lindex $line 0]
		set good [lindex $line 1]
		for { set j 0 } { $j < $loss } { incr j } {
			if $before {
				puts $fout "0 $before"
			}
			puts $fout "1 $after"
		}
		set sum [expr $before + $after + 1]
		set good [expr {($good == "") ? 1 : $good}]
		set goods [expr $good * $sum]
		puts $fout "0 $goods"
	}
	close $f 
	close $fout
	return $fileout
}

LossMgr instproc getNode { id0 } {
	$self instvar nodes
	foreach n $nodes {
		if { [$n id] == $id0 } { break }
	}
	return $n
}

LossMgr instproc belongsTo { id nodes } {
	set nodelist ""
	foreach n $nodes {
		lappend nodelist [$n id]
	}
	set k [lsearch -exact $nodelist $id]
	return [expr {($k >= 0) ? 1 : 0}]
}


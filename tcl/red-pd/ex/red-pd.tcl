#
# Main test file for red-pd simulations
#
Class TestSuite
source ../monitoring.tcl
source ../helper.tcl
source sources.tcl
source plot.tcl
source traffic.tcl
source topology.tcl

TestSuite instproc init {} {
    $self instvar ns_  test_ net_ traffic_ enable_ 
    $self instvar topo_ node_ testName_
    $self instvar scheduler_ random_
    
    set ns_ [new Simulator]
    
    set random_ [new RNG]
    $random_ seed 0

    #$ns_ use-scheduler Heap
    set scheduler_ [$ns_ set scheduler_]
    
    set topo_ [new Topology/$net_ $ns_]
    foreach i [$topo_ array names node_] {
	set node_($i) [$topo_ node? $i]
    }
    
    set testName_ "$test_.$net_.$traffic_.$enable_"
}

TestSuite instproc  rnd {n} {
	$self instvar random_
	return [$random_ uniform 0 $n]
}


TestSuite instproc config { name } {
    $self instvar linkflowfile_ linkgraphfile_
    $self instvar flowfile_ graphfile_
    $self instvar redqfile_
    $self instvar label_ post_
    
    set label_ $name
    
    set linkflowfile_ $name.tr
    set linkgraphfile_ $name.xgr
    
    set flowfile_ $name.f.tr
    set graphfile_ $name.f.xgr
    
    set redqfile_ $name.q
    
    set post_ [new PostProcess $label_ $linkflowfile_ $linkgraphfile_ \
		   $flowfile_ $graphfile_ $redqfile_]
    
    $post_ set format_ "xgraph"
}

#
# prints "time: $time class: $class bytes: $bytes" for the link.
#
TestSuite instproc linkDumpFlows { linkmon interval stoptime } {
    $self instvar ns_ linkflowfile_
    set f [open $linkflowfile_ w]
    puts "linkDumpFlows: opening file $linkflowfile_, fdesc: $f"
    TestSuite instproc dump1 { file linkmon interval } {
	$self instvar ns_ linkmon_
	$ns_ at [expr [$ns_ now] + $interval] \
	    "$self dump1 $file $linkmon $interval"
	foreach flow [$linkmon flows] {
	    set bytes [$flow set bdepartures_]
	    if {$bytes > 0} {
		puts $file \
		    "time: [$ns_ now] class: [$flow set flowid_] bytes: $bytes $interval"     
	    }
	}
	# now put in the total
	set bytes [$linkmon set bdepartures_]
	puts $file  "time: [$ns_ now] class: 0 bytes: $bytes $interval"   
    }
    
    $ns_ at $interval "$self dump1 $f $linkmon $interval"
    $ns_ at $stoptime "flush $f"
}

#
# called on exit
#
TestSuite instproc finish {} {
    $self instvar post_ scheduler_
    $self instvar linkflowfile_
    $self instvar tcpRateFileDesc_ tcpRateFile_
    $scheduler_ halt
    
    #bandwidth in kbps of congested link. this should be the bandwidth of the congested link.
    #BAD way of doing this, should be taken from the topology.
    set bandwidth 10000   
    
    $post_ plot_bytes $bandwidth   ;#plots the bandwidth consumed by various flows
    $post_ plot_queue             ;#plots red's average and instantaneous queue size
    close $tcpRateFileDesc_       
    $post_ plot_tcpRate $tcpRateFile_  ;#plots TCP goodput
}


#
# for tests with a single congested link
#
Class Test/one -superclass TestSuite
Test/one instproc init { name topo traffic enable numFlows } {
    $self instvar net_ test_ traffic_ enable_ numFlows_
    set test_ $name
    set net_ $topo   
    set traffic_ $traffic
    set enable_ $enable
    set numFlows_ numFlows
    $self next
    $self config $name.$topo.traffic.$enable
}

Test/one instproc run {} {
    $self instvar ns_ net_ topo_ enable_ stoptime_ test_ traffic_ label_
    $topo_ instvar node_ rtt_ redpdq_ redpdflowmon_ redpdlink_ 
    $self instvar tcpRateFileDesc_ tcpRateFile_ numFlows_
    
    #set stoptime_ 100.0
    set stoptime_ 500.0

    set redpdsim [new REDPDSim $ns_ $redpdq_ $redpdflowmon_ $redpdlink_ 0 $enable_]
    set fmon [$redpdsim monitor-link]
    
    $self instvar flowfile_
    set flowf [open $flowfile_ w]
    $fmon attach $flowf
    
    $self linkDumpFlows $fmon 10.0 $stoptime_
    
    $self instvar redqfile_
    set redqf [open $redqfile_ w]
    $redpdq_ trace curq_
    $redpdq_ trace ave_
    $redpdq_ attach $redqf
    
    set tcpRateFile_ $label_.tcp
    set tcpRateFileDesc_ [open $tcpRateFile_ w]
    puts $tcpRateFileDesc_ "TitleText: $test_"
    puts $tcpRateFileDesc_ "Device: Postscript"
    
    
    $self traffic$traffic_ $numFlows_

    $ns_ at $stoptime_ "$self finish"
    
    ns-random 0
    $ns_ run
}

#
#multiple congested link tests
#
Class Test/multi -superclass TestSuite
Test/multi instproc init { name topo traffic enable {unused 0}} {
    $self instvar net_ test_ traffic_ enable_
    set test_ $name
    set net_ $topo   
    set traffic_ $traffic
    set enable_ $enable
    $self next
    $self config $name.$topo.traffic.$enable
}

Test/multi instproc run {} {
    $self instvar ns_ net_ topo_ enable_ stoptime_ test_ label_
    $topo_ instvar node_ rtt_ redpdq_ redpdflowmon_ redpdlink_ noLinks_
    $self instvar tcpRateFileDesc_ tcpRateFile_
    
    set stoptime_ 300.0
    
    for {set i 0} {$i < $noLinks_} {incr i} {
	set redpdsim($i) [new REDPDSim $ns_ $redpdq_($i) $redpdflowmon_($i) $redpdlink_($i) $i $enable_]
    }
    
    #monitor the last congested link
    set link [expr {$noLinks_ - 1}]
    
    set fmon [$redpdsim($link) monitor-link]
    $self instvar flowfile_
    set flowf [open $flowfile_ w]
    $fmon attach $flowf
    $self linkDumpFlows $fmon 10.0 $stoptime_
    
    $self instvar redqfile_
    set redqf [open $redqfile_ w]
    $redpdq_($link) trace curq_
    $redpdq_($link) trace ave_
    $redpdq_($link) attach $redqf

    set tcpRateFile_ $label_.tcp
    set tcpRateFileDesc_ [open $tcpRateFile_ w]
    puts $tcpRateFileDesc_ "TitleText: $test_"
    puts $tcpRateFileDesc_ "Device: Postscript"
    
    $self trafficMulti $noLinks_
    
    $ns_ at $stoptime_ "$self finish"
    ns-random 0
    $ns_ run
}


#-----------------------------------------

TestSuite proc usage {} {
        global argv0
    puts stderr "usage: ns $argv0 <test> <topology> <traffic> \[disable\]"
    puts stderr "Valid tests are:\t[$self get-subclasses TestSuite Test/]"
    puts stderr "Valid Topologies are:\t[$self get-subclasses Topology Topology/]"
    puts stderr "Valid Traffic are: \t[$self get-subclasses TestSuite Traffic/]"
    exit 1
}

TestSuite proc isProc? {cls prc} {
        if [catch "Object info subclass $cls/$prc" r] {
                global argv0
                puts stderr "$argv0: no such $cls: $prc"
                $self usage
        }
}

TestSuite proc get-subclasses {cls pfx} {
        set ret ""
        set l [string length $pfx]

        set c $cls
        while {[llength $c] > 0} {
                set t [lindex $c 0]
                set c [lrange $c 1 end]
                if [string match ${pfx}* $t] {
                        lappend ret [string range $t $l end]
                }
                eval lappend c [$t info subclass]
        }
        set ret
}

TestSuite proc runTest {} {
    global argc argv
    set enable true
    switch $argc {
	3 {
	    set test [lindex $argv 0]
	    $self isProc? Test $test
	    
	    set topo [lindex $argv 1]
	    $self isProc? Topology $topo
	    
	    set traffic [lindex $argv 2]
	    $self isProc? Traffic $traffic
	    
	    set enable true
	    set numFlows 2
	}
	4 {
	    set test [lindex $argv 0]
	    $self isProc? Test $test
	    
	    set topo [lindex $argv 1]
	    $self isProc? Topology $topo
	    
	    set traffic [lindex $argv 2]
	    $self isProc? Traffic $traffic
	    

	    set enable true
	    set enable [lindex $argv 3]
	    if { $enable == "disable" } {
		set enable false
	    } elseif { $enable == "enable" } {
		set enable true
  	    } else {
		$self usage
	    }

	    set numFlows 2
	}
	5 {
	    set test [lindex $argv 0]
	    $self isProc? Test $test
	    
	    set topo [lindex $argv 1]
	    $self isProc? Topology $topo
	    
	    set traffic [lindex $argv 2]
	    $self isProc? Traffic $traffic
	    
	    set enable true
	    set enable [lindex $argv 3]
	    if { $enable == "disable" } {
		set enable false
	    } elseif { $enable == "enable" } {
		set enable true
  	    } else {
		$self usage
	    }

	    set numFlows [lindex $argv 4]
	}
	default {
	    $self usage
	}
    }
    set t [new Test/$test $test $topo $traffic $enable $numFlows]
    $t run
}

TestSuite runTest

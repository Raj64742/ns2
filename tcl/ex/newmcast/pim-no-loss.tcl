## exhaustive loss on a lan

source loss.tcl
#source tcl/lib/ns-lib-diff.tcl

set ns [new Simulator]
Simulator set EnableMcast_ 1

for {set i 0 } { $i <= 4 } { incr i } {
        set n$i [$ns node]
}

set scenarioInvestigated 0
set traceIndex $scenarioInvestigated
set traceSuffix ".nam"
set traceFile "out-pimloss"
set file0 $traceFile-$traceIndex$traceSuffix
if { [file exists $file0] } {
	exec rm $file0
}
set f [open $file0 w]
$ns namtrace-all $f

set nodes [list $n0 $n1 $n2 $n3]

Simulator set NumberInterfaces_ 1
set lan [$ns multi-link-of-interfaces $nodes 1.50Mb 10ms DropTail]

#	|4|
#      /  \
#    |3|  |2|
#   ----------
#    |0|  |1|
$lan nodePos $n0 down
$lan nodePos $n1 down
$lan nodePos $n2 up
$lan nodePos $n3 up

$ns duplex-link $n3 $n4 1.5Mb 10ms DropTail
$ns duplex-link-op $n3 $n4 orient up

$ns duplex-link $n4 $n2 1.5Mb 10ms DropTail
$ns duplex-link-op $n4 $n2 orient down

#Prune/Iface/Timer set timeout 0.3
#Deletion/Iface/Timer set timeout 0.1
set mproto detailedDM
$ns mrtproto $mproto {}
$ns rtproto Session  

set group0 [expr 0x8003]

Agent/CBR set interval_ 25ms
set sender4 [new Agent/CBR]
$sender4 set dst_ $group0
$sender4 set class_ 2
$ns attach-agent $n4 $sender4

for {set i 0 } { $i <= 3 } { incr i } {
        set rcvr$i [new Agent/LossMonitor]
        eval "\$ns attach-agent \$n$i \$rcvr$i"
}

for { set i 0 } { $i <= 1 } { incr i } {
	set J$i "[eval "set n$i"] join-group [eval "set rcvr$i"] $group0"
	set L$i "[eval "set n$i"] leave-group [eval "set rcvr$i"] $group0" 
}

# init and cleanup
lappend initList "\$ns clear-mcast"
lappend initList "\$ns run-mcast"

set simulationDuration 0

proc generate-scenarios { ev1 ev2 ev3 ev4 initList } {
  global ns traceFile traceSuffix traceIndex loss0 \
		simulationDuration scenarioInvestigated

  set scenarioDone 0

  set x [expr 0. - 0.0001]
  set scenarioNum 0
  for { set j 0 } {$j < 3} {incr j} {
    set event($j) $ev1
    for { set k 0} {$k < 3} {incr k} {
        if { $j == $k } {continue}
        set event($k) $ev2
        for { set l [expr $j + 1]} {$l < 4} {incr l} {
          if { $k == $l } { continue }
          set event($l) $ev3
          for { set m [expr $k + 1]} {$m < 4} {incr m} {
                if { $j == $m || $l == $m } {
                        continue
                }
                set event($m) $ev4

                if { $scenarioDone == 1 } {
                        set j 3
                        set k 3
                        set l 4
                        set m 4
                        continue
                }
                        
                if { $scenarioNum != $scenarioInvestigated } {
                        incr scenarioNum
                        continue
                }
  
                set scenarioDone 1

		    set cnt 0
		    foreach ev $initList {
			incr cnt
			$ns at [expr $x + 0.0002 * $cnt] $ev
		    }
                    $ns at [expr $x + .1] $event(0)
                    $ns at [expr $x + .2] $event(1)
                    $ns at [expr $x + .3] $event(2)
                    $ns at [expr $x + .4] $event(3)
                    set x [expr $x + .5]  
          }
        }
    }
  }
  set simulationDuration [expr $x + 0.1]
  puts "!!! simulationDuration $simulationDuration";
}

#lappend messages [PIM set ASSERT]
# the join for pimdm is 33
lappend messages 33
#lappend messages [PIM set JOIN]
#lappend messages [PIM set PRUNE] 

generate-scenarios $J0 $J1 $L0 $L1 $initList 

$ns at 0.0001 "$sender4 start"
#set send_interval 0.025

#set pktCnt 0
#set duration 0
   
#while { $duration < $simulationDuration } {
#        incr pktCnt
#        set duration [expr 0 + $send_interval * $pktCnt]
#        $ns at $duration "$sender4 transmit \"$pktCnt\""
#}               

$ns at $simulationDuration "finish"

proc finish {} {
  global ns 
#traceFileList
  $ns flush-trace
#  set file [lindex $traceFileList 0]
#  set namTraceFile "tcl/ex/out-loss-7-24-6-nam.tr"
#  if { [file exists $namTraceFile] } {
#	  exec rm $namTraceFile
#  }
#  exec awk -f tcl/nam-demo/nstonam.awk $file > $namTraceFile

  puts "running nam..."
#  exec ~/nam/nam-0.8a/nam tcl/ex/out-loss-7-24-6-nam &
  exit 0
}

$ns run

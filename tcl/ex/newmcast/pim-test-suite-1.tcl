## STRESS: basic representative scenarios simulation (without loss)
#	|4|
#      /  \
#    |3|  |2|
#   ----------
#    |0|  |1|

# set pimpath "tcl/pim/"
# set pimfile "pim-all-proc.tcl"
# set procTraceFile "proc-trace.out"
#XXX awk -f all-proc.awk $pimpath$pimfile > $pimpath$procTraceFile
# or exec cd tcl/pim; exec profile.sh; cd ../../

set ns [new MultiSim]

for {set i 0 } { $i <= 4 } { incr i } {
        set n$i [$ns node]
}

set traceIndex 0
set traceSuffix ".tr"
set traceFile "pim-test-suite-1-out"
set file0 $traceFile-$traceIndex$traceSuffix
set f [open $file0 w]
$ns trace-all $f

lappend traceFileList $file0

set nodes [list $n0 $n1 $n2 $n3]
set ml0 [$ns multi-link-of-interfaces $nodes 1.5Mb 10ms DropTail]

$ns duplex-link-of-interfaces $n3 $n4 1.5Mb 10ms DropTail
$ns duplex-link-of-interfaces $n2 $n4 1.5Mb 10ms DropTail

for {set i 0 } { $i <= 4 } { incr i } {
        eval "set pim$i \[new PIM \$ns \$n$i \[list 1\]\]"
}

PIM config $ns

set agentTrace [open agent-trace-pim-test-suite-1.out w]

set group0 [expr 0x8003]

set sender4 [new Agent/Message/trace]
$sender4 set dst_ $group0
$sender4 set class_ 2
$ns attach-agent $n4 $sender4
$sender4 trace $agentTrace
$sender4 set-TxPrefix "S"

for {set i 0 } { $i <= 3 } { incr i } {
        set rcvr$i [new Agent/Message/trace]
}

for {set i 0 } { $i <= 3 } { incr i } {
        eval "\$ns attach-agent \$n$i \$rcvr$i"
	eval "\$rcvr$i trace \$agentTrace"
        eval "\$rcvr$i set-RxPrefix \"R\""
}

PIM trace $agentTrace

for { set i 0 } { $i <= 1 } { incr i } {
	set J$i "[eval "set rcvr$i"] join-group $group0"
	set L$i "[eval "set rcvr$i"] leave-group $group0"
}

# init and cleanup
lappend initList "\$ns clear-mcast"
lappend initList "\$ns run-mcast"
lappend initList "\$pim0 force-nextHop \$group0 3"
lappend initList "\$pim1 force-nextHop \$group0 2"

set simulationDuration 0

proc generate-scenarios { ev1 ev2 ev3 ev4 initList} {
  global ns traceFile traceSuffix traceIndex traceFileList simulationDuration
  set x [expr 0. - 0.0001]
  set scenarioNum $traceIndex
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

		incr scenarioNum
		set newTraceFile $traceFile-$scenarioNum$traceSuffix
		lappend traceFileList $newTraceFile
		set newFile [open $newTraceFile w]
		$ns at $x "$ns change-all-traces $newFile"
          }
        }
    }
  }
  set simulationDuration [expr $x + 0.1]
}

generate-scenarios $J0 $J1 $L0 $L1 $initList

set send_interval 0.025

set pktCnt 0
set duration 0

while { $duration < $simulationDuration } {
	incr pktCnt
	set duration [expr 0 + $send_interval * $pktCnt]
        $ns at $duration "$sender4 transmit \"$pktCnt\""
}

$ns at $simulationDuration "finish"

proc finish {} {
  global ns traceFileList
  $ns flush-trace
  exit 0
}

$ns run

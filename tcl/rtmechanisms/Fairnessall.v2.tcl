#
# To run this:  "../../ns Fairnessall.v2.tcl fifo 1 3 Fairness"
#
set datafile Fairness.data
source Fairnessall1.v2.tcl
set scheduling [lindex $argv 0]
set cbrs [lindex $argv 1]
set tcps [lindex $argv 2]
set type [lindex $argv 3]
if {$type == "Collapse"} {
  set bandwidth 128Kb
} else {
  set bandwidth 10Mb
}
#
set singlefile temp.tr
set label $type.$scheduling.$cbrs.$tcps
set psfile Fairness.ps

# Run a single simulation.
proc run_sim {bandwidth scheduling cbrs tcps singlefile datafile i} {
  set interval [expr $cbrs * 0.0$i]
  puts "../../ns Collapse.v2.tcl simple $interval $bandwidth $scheduling $cbrs $tcps"
  exec ../../ns Collapse.v2.tcl simple $interval $bandwidth $scheduling $cbrs $tcps
  append $singlefile $datafile $cbrs $tcps
}

exec rm -f $datafile
# for interval=0.00008, CBR arrival rate is 10 Mbps
# for interval=0.0008, CBR arrival rate is 1 Mbps
# intervals 0.0001 to 0.0009
for {set i 4} {$i <= 9} {incr i 1} {
    run_sim $bandwidth $scheduling $cbrs $tcps $singlefile $datafile 00$i
}
# intervals 0.001 to 0.009
for {set i 1} {$i <= 9} {incr i 1} {
    run_sim $bandwidth $scheduling $cbrs $tcps $singlefile $datafile 0$i
}
# intervals 0.01 to 0.09
for {set i 1} {$i <= 9} {incr i 1} {
    run_sim $bandwidth $scheduling $cbrs $tcps $singlefile $datafile $i
}
finish

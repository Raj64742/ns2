#
# To run this:  "./ns Fairnessall.tcl"
#
set datafile Fairness.data
source Fairnessall1.tcl
#
set bandwidth 10Mb
set infile Fairness.tr
set psfile Fairness.ps

exec rm -f $datafile
# for interval=0.00008, CBR arrival rate is 10 Mbps
# for interval=0.0008, CBR arrival rate is 1 Mbps
# intervals 0.0001 to 0.0009
for {set i 4} {$i <= 9} {incr i 1} {
    puts "./ns Collapse.tcl simple 0.000$i $bandwidth"
# need catch for those "warning: no class variable Queue/RED::fracminthresh_"
    catch {exec ./ns Collapse.tcl simple 0.000$i $bandwidth} result
#    puts stderr $result
    append $infile $datafile
}
# intervals 0.001 to 0.009
for {set i 1} {$i <= 9} {incr i 1} {
    puts "./ns Collapse.tcl simple 0.00$i $bandwidth"
    catch {exec ./ns Collapse.tcl simple 0.00$i $bandwidth} result
#    puts stderr $result
    append $infile $datafile
}
# intervals 0.01 to 0.09
for {set i 1} {$i <= 9} {incr i 1} {
    puts "./ns Collapse.tcl simple 0.0$i $bandwidth"
    catch {exec ./ns Collapse.tcl simple 0.0$i $bandwidth} result
#    puts stderr $result
    append $infile $datafile
}
finish

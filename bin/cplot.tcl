#
# cmerge -- "cooked" plot
#	merge multiple cooked trace files together, eventually
#	to produce a final plot:
#
# Usage: cplot package graph-title cfile1 cname1 [cfile2 cname 2] ...
#
# this will take cooked trace files cfile{1,2,...}
# and merge them into a combined graph of the type defined in
# "package".  for now, package is either xgraph or gnuplot
#

set labelproc(xgraph) xgraph_label
set labelproc(gnuplot) gnuplot_label
set headerproc(xgraph) xgraph_header
set headerproc(gnuplot) gnuplot_header
set filext(xgraph) xgr
set filext(gnuplot) plt

set package default; # graphics package to use

if { $argc < 4 || [expr $argc & 1] } {
	puts stderr "Usage: tclsh cplot graphics-package graph-title cfile1 cname1 \[cfile2 cname2\] ..."
	exit 1
}

proc init {} {
	global tmpchan tmpfile
	set tmpfile /tmp/[pid].tmp
	set tmpchan [open $tmpfile w+]
}

proc cleanup {} {
	global tmpchan tmpfile package filext
	seek $tmpchan 0 start
	exec cat <@ $tmpchan >@ stdout
	close $tmpchan
	exec rm -f $tmpfile
}

proc run {} {
	global labelproc headerproc package argv tmpchan
	init
	set package [lindex $argv 0]
	set title [lindex $argv 1]
	if { ![info exists labelproc($package)] } {
		puts stderr "cplot: invalid output package $package, known packages: [array names labelproc]"
		exit 1
	}
	set ifile 2
	set iname 3
	$headerproc($package) $tmpchan $title
	while {1} {
		set fname [lindex $argv $ifile]
		set label [lindex $argv $iname]
		if { $fname == "" || $label == "" } {
			break
		}
		do_file $fname $label $package $tmpchan
		incr ifile 2
		incr iname 2
	}
	cleanup
}

proc xgraph_header { tmpchan title } {
        puts $tmpchan "TitleText: $title"
        puts $tmpchan "Device: Postscript"
	puts $tmpchan "BoundBox: true"
	puts $tmpchan "Ticks: true"
	puts $tmpchan "Markers: true"
	puts $tmpchan "NoLines: true"
	puts $tmpchan "XUnitText: time"
	puts $tmpchan "YUnitText: sequence/ack number"
}

proc xgraph_label { tmpchan label } {
	puts $tmpchan \n\"$label
}

proc gnuplot_label { tmpchan label } {
	puts $tmpchan $label
}

proc do_file { fname label graphtype tmpchan } {
	global labelproc
	$labelproc($graphtype) $tmpchan $label
	exec cat $fname >@ $tmpchan
}
run

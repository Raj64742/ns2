#!/usr/bin/tclsh

if { $argc != 1} {
   puts "usage: bw.tcl <tcpdump file in ASCII format> "
   exit
} else {
  set arg [split $argv " " ]
  set tfile [lindex $arg 0]
}

set fi [open $tfile r ]
set fo [open $tfile.bw w ]
while {[gets $fi line] >= 0} {

  set startTime  [lindex $line 0]

  set seqno [lindex $line 5]
  if { $seqno !="ack"} {
     set s [split $seqno ":()"]
     set size [lindex $s 2]
     set size [expr $size + 40]
     puts $fo "$startTime $size"
    
  } else {
    puts $fo "$startTime 40"
  }
}
close $fo 
close $fi 

# Create many persistent TCP connections.
# Start time between startinterval and endinterval.
# End time between startinterval and endinterval.
# Number of connections: num
proc create_tcps {num src dst src_type dest_type win class size startinterval \
        endinterval} {
    set maxrval [expr pow(2,31)]
    set tdiff [expr $endinterval - $startinterval]

    for { set j 0 } { $j < $num } { incr j } {
        set randomval [ns-random]
        set rtmp [expr $randomval / $maxrval]
        set start_time [expr $rtmp * $tdiff]
	set enddiff [expr $endinterval - $start_time]
	set randomval [ns-random]
	set rtmp [expr $randomval / $maxrval]
	set stop_time [expr $start_time + ($rtmp * $enddiff)]
        set numpkts 0
        new_Tcp $start_time $stop_time $src $dst $win $class $size $src_type $dest_type $numpkts
        incr class
    }
}

# Create many persistent cbr connections.  
# Start time between startinterval and endinterval.
# Number of connections: num.
proc create_cbrs {num src dst class size interval startinterval \
        endinterval} {
    set maxrval [expr pow(2,31)]
    set tdiff [expr $endinterval - $startinterval]

    for { set j 0 } { $j < $num } { incr j } {
        set randomval [ns-random]
        set rtmp [expr $randomval / $maxrval]
        set start_time [expr $rtmp * $tdiff]
        
        new_cbr $start_time $src $dst $size $interval $class 
    }
}

#
# create and schedule a tcp source/dst 
# new_Tcp 304.0 350.0 $s2 $s4 100 31 512 Sack1 Sack1 0
#
proc new_Tcp { startTime stopTime source dest window class size src_type dest_type
maxpkts } {
  global ns

  set tcp [$ns create-connection TCP/$src_type $source TCPSink/$dest_type $dest $class]
  $tcp set window_ $window
  if {$size > 0}  {
    $tcp set packetSize_ $size
  }
  if {$maxpkts > 0} {
    $tcp set maxpkts_ $maxpkts
  }
  set ftp [$tcp attach-source FTP]
  $ns at $startTime "$ftp start"
  if {$stopTime > $startTime} {
   $ns at $stopTime "$ftp stop"
  }
}

# Create many short TCP connections.  
# Number of packets between minpkts and maxpkts.
# Start time between startinterval and endinterval.
# Number of connections: nmice.
proc create_mice {nmice src dst src_type dest_type win classbase startinterval \
        endinterval} {
    set maxrval [expr pow(2,31)]
    set minpkts 2
    set maxpkts [expr $minpkts + 5]
    set tdiff [expr $endinterval - $startinterval]
    
    set classnum $classbase

    for { set j 0 } { $j < $nmice } { incr j } {
        set randomval [ns-random]
        set rtmp [expr $randomval / $maxrval]
        set start_time [expr $rtmp * $tdiff]
        set randomval [ns-random]
        set rtmp [expr $randomval / $maxrval]
        if { $rtmp > 0.5 } {
            set psize 512
        } else {
            set psize 1460
        }
        set randomval [ns-random]
        set rtmp [expr $randomval / $maxrval]
        set numpkts [expr round($rtmp * $maxpkts) + $minpkts]
        
        new_Tcp $start_time $start_time $src $dst $win $classnum $psize $src_type $dest_type $numpkts
    }
}

global pageIdx
global server
global nSvr
global client
global nClnt
global sessionIdx
global PageThreshold
global SessionThreshold
global outp
source util.tcl

set tfile "demo.dump"

set PageThreshold 1
set SessionThreshold 15

set timezero 0.0
set sessionIdx 0
set isrc ""
set idst ""
set t 0
set nSvr 0
set nClnt 0

catch { exec /bin/rm -rf SESSION*} res
catch { exec tcpdump -S -n -r  $tfile src port 80 and tcp | awk -f util1.awk  > www1.dmp} res

set outp [open "config.log" w ]


set fi [open "www1.dmp" r ]
while {[gets $fi line] >= 0} {

set startTime  [tcpdtimetosecs [lindex $line 0]]

set src [lindex $line 1]
set dst [lindex $line 2]
set flag [lindex $line 3]
set seqno [lindex $line 4]
if { $seqno =="ack"} {
 set ack  [lindex $line 5]
 set win  [lindex $line 7]
 set seqno ""
} else {
 set ack  [lindex $line 6]
 set win  [lindex $line 8]
}

   #calculate the number of server
   set p 0
   for {set i 0} {$i< $nSvr} {incr i} {
      if { $src == $server($i)} {
         set p 1
      }
   }
   if { $p == 0 } {
     set server($nSvr) $src
     set nSvr [expr $nSvr + 1]
   }

   #calculate the number of server
   set s [split $dst "."]
   set c [format "%s.%s.%s.%s" [lindex $s 0] [lindex $s 1] [lindex $s 2] [lindex $s 3]]
   set p 0
   for {set i 0} {$i< $nClnt} {incr i} {
      if { $c == $client($i)} {
         set p 1
      }
   }
   if { $p == 0 } {
     set client($nClnt) $c
     set nClnt [expr $nClnt + 1]
   }



set diff [expr $startTime - $t]

if { $diff > $SessionThreshold } {

   puts $outp "INTERSESSION $diff"

   set sessionIdx [expr $sessionIdx + 1]
   set pageIdx($sessionIdx) 0
}

if { $diff > $PageThreshold } {

   if { $diff < $SessionThreshold } {
       puts $outp "INTERPAGE $diff"
   }

   set pageIdx($sessionIdx) [expr $pageIdx($sessionIdx) + 1]
   set sessDir [format "SESSION%04d" $sessionIdx]
   set pageDir [format "PAGE%06d" $pageIdx($sessionIdx)]
   set pageName [format "page%06d" $pageIdx($sessionIdx)]
   catch {exec mkdir $sessDir} res
   catch {exec mkdir $sessDir/$pageDir} res
   if [info exists fo] {
      close $fo
   }
   set fo [open $sessDir/$pageDir/$pageName w ]
}

if { $seqno ==""} {
   puts $fo "$startTime $src $dst $flag ack $ack "
} else {
   set s [split $seqno ":()"]
puts $fo "$startTime $src $dst $flag [lindex $s 0] [lindex $s 2] ack $ack "
}

set t $startTime

}

close $fo


puts $outp "NUMSERVER $nSvr"
puts $outp "NUMCLIENT $nClnt"
puts $outp "NUMSESSION $sessionIdx"

set objf [open object.startTime w]
 
for {set m 1} {$m<=$sessionIdx} {incr m} {

 puts $outp "NUMPAGE $pageIdx($m)"

 for {set j 1} {$j<=$pageIdx($m)} {incr j} {
   set numObjPerPage 0

   set sessDir [format "SESSION%04d" $m]
   set pageDir [format "PAGE%06d" $j]
   set pageName [format "page%06d" $j]
   set pageFile [format "%s" $sessDir/$pageDir/$pageName ]
   set pageFileSorted [format "%s" $sessDir/$pageDir/$pageName.sort ]
   catch { exec cat $pageFile | awk -f util2.awk | sort > $pageFileSorted } res

   if [info exists fi] {
     close $fi
   }

   set fi [open $pageFileSorted r] 


   set isrc ""
   set idst ""
   set k 0

   #number of TCP connection
   while {[gets $fi line] >= 0} {

   set src [lindex $line 0]
   set dst [lindex $line 1]
   set startTime  [lindex $line 2]

   set field3 [lindex $line 3]
   set field4 [lindex $line 4]
   set field5 [lindex $line 5]
   set field6 [lindex $line 6]
   set field7 [lindex $line 7]
   set field8 [lindex $line 8]

   if { $src != $isrc || $dst != $idst } {
      if [info exists outf] {
         close $outf
      }
      set k [expr $k + 1]
      set dumpFile [format "%04d.dmp" $k]
      set outf [open $sessDir/$pageDir/$dumpFile w ]
      set isrc $src
      set idst $dst
     }

   puts $outf "$src $dst $startTime $field3 $field4 $field5 $field6 $field7 $field8"
   }
   puts $outp "NUMCONNECTION $k"
   if [info exists outf] {
      close $outf
   }

   #number of objects requested in each connection
   for {set n 1} {$n<=$k} {incr n} {
      set dumpFile [format "%04d.dmp" $n]
      set inf [open $sessDir/$pageDir/$dumpFile r ]
puts $outp "$sessDir/$pageDir/$dumpFile"
      set oldAck 0
      set numObj 0
      set objSize 0
      while {[gets $inf line] >= 0} {
         set startTime  [lindex $line 2]
         set size [lindex $line 5]
         set ackSeq [lindex $line 7]
         if { $size > 0 } {
           if { $ackSeq > $oldAck } {
             if { $objSize > 0 } {
                 puts $outp "OBJSIZE $objSize"
                 set numObjPerPage [expr $numObjPerPage + 1]
             }
             puts $objf "$sessDir/$pageDir/$dumpFile $startTime"
             set objSize $size
             set numObj [expr $numObj + 1]
           } else {
             if { $ackSeq == $oldAck && $size > 0 } {
                set objSize [expr $objSize + $size]
             }
           }
           set oldAck $ackSeq
         }
      }
      if { $objSize > 0 } {
          set numObjPerPage [expr $numObjPerPage + 1]
          puts $outp "OBJSIZE $objSize"
      }
      if { $numObj > 1 } {
          puts $outp "PERSISTCONN"
      }
   }
puts $outp "NUMOBJPERPAGE $numObjPerPage"
}
}

catch { exec cat object.startTime |  sort > object.start } res
catch { exec cat config.log | grep INTERSESSION | awk -f util3.awk  > session.inter } res
catch { exec cat config.log | grep INTERPAGE | awk -f util3.awk > page.inter } res
catch { exec cat config.log | grep NUMCONNECTION  | awk -f util3.awk > connect.cnt } res
catch { exec cat config.log | grep OBJSIZE  | awk -f util3.awk > object.size  } res
catch { exec cat config.log | grep PERSISTCONN  > persist.cnt  } res
catch { exec cat config.log | grep NUMOBJPERPAGE  | awk -f util3.awk > objPerPage } res

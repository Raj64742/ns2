proc Pre { tfile } { 
    global timezero 
    set f [open "|tcpdump -n -r $tfile src port 80 and tcp" r]
    set line [gets $f]
    catch "close $f"
    set aslist [split $line " "]
    set timezero [tcpdtimetosecs [lindex $aslist 0]]
}

proc tcpdtimetosecs tcpdtime  {
    global timezero 

    # convert time to seconds
    set t [split $tcpdtime ":."]
    regsub {^0*([0-9])} [lindex $t 0] {\1} hours
    regsub {^0*([0-9])} [lindex $t 1] {\1} minutes
    regsub {^0*([0-9])} [lindex $t 2] {\1} seconds
    set micsec [lindex $t 3]
    append mtim "0." $micsec

    set tim [expr 1.0*$hours*3600 + 1.0*$minutes*60 + 1.0*$seconds ]
    set tim [expr $tim + 1.0*$mtim]


    ##  check for time wrap
    ##
    if { $tim < $timezero } { set tim [expr $tim + 86400.0] }
    return [expr $tim - $timezero]
}


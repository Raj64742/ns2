# OUTPUT:
# Fairness.data: CBR arrival rate, CBR goodput, TCP goodput (in KBps)
#
# INPUT:
# temp.tr:
# class 0 packet-size 100
# class 0 total_packets_received 364
# class 3 total_packets_acked 11781
# cbrs: 3 tcps: 1
# class 3 arriving_pkts 1489 dropped_pkts 308
# stop-time 100.0
#
proc append { infile datafile cbrs tcps } {
        set awkCode {
                {
                if ($1=="stop-time") {time = $2;}
                if ($3=="packet-size") {size[$2] = $4;}
                if ($3=="total_packets_acked") {
                  packets[$2] += $4;
                  goodput[1] += (packets[$2]*size[$2]*8)/1000
                }
                if ($3=="total_packets_received") {
                  packets[$2] += $4;
                  goodput[0] += (packets[$2]*size[$2]*8)/1000
                }
                if ($1=="cbrs:"&&$3=="tcps:") {
			if (($2+$4) == 0)
				cbr_fraction = 0;
			else
				cbr_fraction = $2/($2+$4);
                }
                if ($3=="arriving_pkts" && time > 0) {
                        if ($2 < cbrs) {
                          cbrrate += (($4*size[0]*8)/time)/1000
                        }
                        if ($2==cbrs-1) {
                          printf "%8.2f %8.2f %8.2f %8.2f\n", cbrrate, \
                            goodput[0]/time, goodput[1]/time, cbr_fraction
                        }
                }
                }
        }
        exec awk $awkCode cbrs=$cbrs $infile >> $datafile
}

#-------------------------------------------------------------------

proc finish { } {
    global datafile psfile

    set awkCode1 { {printf "%8.2f %8.2f\n", $1, $1} }
    set awkCode2 { {printf "%8.2f %8.2f\n", $1, $2} }
    set awkCode3 { {printf "%8.2f %8.2f\n", $1, $3} }
    # cbr arrival vs. cbr goodput
    exec awk $awkCode2 $datafile | sort > chart
    # cbr arrival vs. tcp goodput
    exec awk $awkCode3 $datafile | sort > chart1
    # cbr arrival vs. cbr arrival
    exec awk $awkCode1 $datafile | sort > chart2
    ##
    set f [open temp.rands w]
    puts $f "TitleText: $psfile"
    puts $f "Device: Postscript" 
    
    puts $f \"UdpArrivals
    flush $f
    exec cat chart2 >@ $f
    flush $f
    
    
    puts $f \n"UdpGoodput
    flush $f
    exec cat chart >@ $f
    flush $f
    
    puts $f \n"TcpGoodput
    flush $f
    exec cat chart1 >@ $f
    flush $f
    
    close $f
    puts "Calling xgraph..."
    exec xgraph -bb -tk -m -x UdpArrivals -y GoodputKbps temp.rands &
    exit 0
}

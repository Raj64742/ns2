#!../../ns
source ../lan/ns-mac.tcl
source ../lan/ns-lan.tcl
set flags(0) 0
set stop 10
set trfile out
set tcpfile tcpstats
set bw 100Kb
set delay 10ms
set ll LL
set ifq Queue/DropTail
set mac Mac/Multihop
set chan Channel

if {$argc == 0} {
	puts "Usage: $argv0 \[-stop $stop\] \[-seed #]"
	puts "\t\[-tr $trfile\] \[-stat $tcpfile\]"
	puts "\t\[-ll lltype\] \[-ifq ifqtype\] \[-mac mactype\]"
	puts "\t\[-bw $bw] \[-delay $delay\]"
	exit 1
}

proc make-trace {ns} {
	global trfile trfd

	if [file exists ${trfile}] {
		exec touch ${trfile}.${trfile}
		eval exec rm $trfile [glob ${trfile}.*]
	}

	set trfd [open $trfile w]
	$ns trace-all $trfd
}

proc get-options {argc argv} {
	global stop bw delay ll ifq mac chan trfile flags 

	# regexp {^(.+)\..*$} $argv0 match ext
	for {set i 0} {$i < $argc} {incr i} {
		set opt [lindex $argv $i]
		if {$opt == "-stop"} {
			set stop [lindex $argv [incr i]]
		} elseif {$opt == "-bw"} {
			set bw [lindex $argv [incr i]]
		} elseif {$opt == "-delay"} {
			set delay [lindex $argv [incr i]]
		} elseif {$opt == "-ll"} {
			set ll LL/[lindex $argv [incr i]]
		} elseif {$opt == "-ifq"} {
			set ifq Queue/[lindex $argv [incr i]]
		} elseif {$opt == "-mac"} {
			set mac Mac/[lindex $argv [incr i]]
		} elseif {$opt == "-dc"} {
			set chan Channel/Duplex
		} elseif {$opt == "-tr"} {
			set trfile [lindex $argv [incr i]]
		} elseif {$opt == "-stat"} {
			set tcpfile [lindex $argv [incr i]]
		} elseif {$opt == "-seed"} {
			ns-random [lindex $argv [incr i]]
		} elseif {[string range $opt 0 0] == "-"} {
			set flags([string range $opt 1 1]) 1
		}
	}
	if {$mac == "Mac/Multihop"}  {
		set flags(mh) 1
	}
}

proc make-tcp-connections {} {
	global tcp ftp tcpfile tcpfd ns n

	set tcp(0) [$ns create-connection TCP/Reno $n(1) TCPSink $n(0) 0]
	set ftp(0) [$tcp(0) attach-source FTP]
	set tcp(1) [$ns create-connection TCP/Reno $n(0) TCPSink $n(1) 0]
	set ftp(1) [$tcp(1) attach-source FTP]
	set tcp(2) [$ns create-connection TCP/Reno $n(1) TCPSink $n(0) 0]
	set ftp(2) [$tcp(2) attach-source FTP]
	set tcp(3) [$ns create-connection TCP/Reno $n(2) TCPSink $n(3) 0]
	set ftp(3) [$tcp(3) attach-source FTP]
	
	set tcpfd(1) [open ${tcpfile}.1 w]
	set tcpfd(2) [open ${tcpfile}.2 w]
	$tcp(1) trace $tcpfd(1)
	$tcp(2) trace $tcpfd(2)
}

proc make-cbr-connections {} {
	global n ns
	set cbr(2:3) [$ns create-connection CBR $n(2) Null $n(3) 0]
}

proc fire-connections {} {
	global ns ftp
	# $ns at 0 "$ftp(0) start"
	$ns at 0 "$ftp(1) start"
	# $ns at 0.02 "$ftp(2) start"
	# $ns at 0.03 "$ftp(3) start"
	# $ns at 5.1 "$cbr(2:3) start"
}

proc monitor-lan {lan} {
	set qfile [open "q.dat" w]
	$lan init-monitor $ns $qfile 1 $n(0)
	$lan init-monitor $ns $qfile 1 $n(1)
	$ns at 0 "$shl queue-sample-timeout"
}

proc create-error {ns nodearray} {
	upvar $nodearray n
	foreach dst {1 2 3} {
		set em(0:$dst) [new ErrorModel]
		[[$ns link $n(0) $n($dst)] link] errormodel $em(0:$dst)
	}
	$em(0:1) set rate_ 0.01
	$em(0:2) set rate_ 0.02
	$em(0:3) set rate_ 0.03

	if [info exists flags(e)] {
		create-error $ns n
	}
}

get-options $argc $argv
set ns [new Simulator]
make-trace $ns

for {set i 0} {$i < 4} {incr i} {
	set n($i) [$ns node]
}
if [info exists flags(mh)] {
	# build topology
	set lan01 [$ns make-lan "$n(0) $n(1)" $bw $delay $ll $ifq $mac $chan]
	puts "LAN: $lan01 $bw $delay $ll $ifq $mac $chan"
#	set lan12 [$ns make-lan "$n(1) $n(2)" $bw $delay $ll $ifq $mac $chan]
#	set lan23 [$ns make-lan "$n(2) $n(3)" $bw $delay $ll $ifq $mac $chan]
	set lan $lan01
} else {
	set nodes [list $n(0) $n(1) $n(2) $n(3)]
	set lan [$ns make-lan $nodes $bw $delay $ll $ifq $mac $chan]
	puts "LAN: $lan $bw $delay $ll $ifq $mac $chan"
	if {$mac == "Mac/802_11"} {
		foreach src "$n(0) $n(1) $n(2) $n(3)" {
			set mac [$lan getmac $src]
			puts "$mac mode RTS_CTS"
			$mac mode RTS_CTS
		}
	}
}
$lan trace $ns $trfd

make-tcp-connections
make-cbr-connections
fire-connections
# monitor-lan $lan

proc finish {} {
	global ns env flags
	global trfd trfile

	$ns flush-trace
	close $trfd

	exec gawk -v trfile=$trfile {
	BEGIN {
		trfile_tcp_1_0 = sprintf("%s.tcp.1.0", trfile);
		trfile_ack_0_1 = sprintf("%s.ack.0.1", trfile);
		trfile_cbr = sprintf("%s.cbr", trfile);
		trfile_cbr_r = sprintf("%s.cbr.r", trfile);
		trfile_tcp_0_1 = sprintf("%s.tcp.0.1", trfile);
		trfile_ack_1_0 = sprintf("%s.ack.1.0", trfile);
		trfile_tcp_0_2 = sprintf("%s.tcp.0.2", trfile);
		trfile_ack_2_0 = sprintf("%s.ack.2.0", trfile);
		trfile_tcp_0_3 = sprintf("%s.tcp.0.3", trfile);
		trfile_ack_3_0 = sprintf("%s.ack.3.0", trfile);
	}
	END {
		fflush("");
		close(trfile_tcp_1_0);
		close(trfile_ack_0_1);
		close(trfile_cbr);
		close(trfile_cbr_r);
		close(trfile_tcp_0_1);
		close(trfile_ack_1_0);
		close(trfile_tcp_0_2);
		close(trfile_ack_2_0);
		close(trfile_tcp_0_3);
		close(trfile_ack_3_0);
	}
	{
		if ($1 == "r") {
			if ($5 == "cbr") {
				print $2, ($11) > trfile_cbr ;
			}
			else if ($5 == "tcp") {
				if (($3 == 1) && ($4 == 0)) {
					print $2, ($11) > trfile_tcp_1_0 ;
				}
				else if (($3 == 0) && ($4 == 1)) {
					print $2, ($11) > trfile_tcp_0_1 ;
				}
				else if (($3 == 0) && ($4 == 2)) {
					print $2, ($11) > trfile_tcp_0_2 ;
				}
				else if (($3 == 0) && ($4 == 3)) {
					print $2, ($11) > trfile_tcp_0_3 ;
				}
			}
			else if ($5 == "ack") {
				if (($3 == 0) && ($4 == 1)) {
					print $2, ($11) > trfile_ack_0_1 ;
				}
				else if (($3 == 1) && ($4 == 0)) {
					print $2, ($11) > trfile_ack_1_0 ;
				}
				else if (($3 == 2) && ($4 == 0)) {
					print $2, ($11) > trfile_ack_2_0 ;
				}
				else if (($3 == 3) && ($4 == 0)) {
					print $2, ($11) > trfile_ack_3_0 ;
				}
			}
		}
		else if ($1 == "r") {
			if ($5 == "cbr") {
				print $2, ($11) > trfile_cbr_r ;
			}
		}
	}
	} $trfile

	exec echo "" >> ${trfile}-bw
	foreach af [lsort [glob ${trfile}.ack.*]] {
		exec tail -1 $af | gawk -v af=$af {
		{
			kbps = 8 * $2 / $1;
			printf("%s\t%f\t%d\t%f\t%f\n", af,$1,$2, $2/$1, kbps);
		}
		} >> ${trfile}-bw
	}
	puts " Files \t\t Time\t   #packets \t pkts/s \t Kbps (1000 byte pkts)"
	exec cat ${trfile}-bw >> /dev/stdout

	if [info exists flags(g)] {
		eval exec xgraph -nl -M -display $env(DISPLAY) \
				[lsort [glob $trfile.*]]
	}

	exit 0
}


$ns at $stop "finish"
$ns run

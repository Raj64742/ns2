#!../../ns

set flags(0) 0
set stop 10
set trfile temp
set tcpfile tcpstats
set bw 100Kb
set delay 10ms
set ll LL/Base
set ifq Queue/DropTail
set mac Mac/Multihop


if {$argc == 0} {
	puts "Usage: $argv0 \[-stop $stop\] \[-seed #]"
	puts "\t\[-tr $trfile\] \[-stat $tcpfile\]"
	puts "\t\[-ll lltype\] \[-ifq ifqtype\] \[-mac mactype\]"
	puts "\t\[-bw $bw] \[-delay $delay\]"
	exit 1
}

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

if [file exist ${trfile}] {
	eval exec rm [glob ${trfile}*]
}


set trfd [open $trfile w]
set ns [new Simulator]
$ns trace-all $trfd


if {$mac == "Mac/Multihop"}  {
	set flags(mh) 1
}
if [info exists flags(mh)] {
	set n(0) [$ns macnode]
	set n(1) [$ns macnode]
	set n(2) [$ns macnode]
	set n(3) [$ns macnode]
	# build topology
	set shl01 [$ns shared-duplex-link "$n(0) $n(1)" $bw $delay DropTail \
			$ll $ifq $mac]
	set shl12 [$ns shared-duplex-link "$n(1) $n(2)" $bw $delay DropTail \
			$ll $ifq $mac]
	set shl23 [$ns shared-duplex-link "$n(2) $n(3)" $bw $delay DropTail \
			$ll $ifq $mac]
	$n(0) linkmacs
	$n(1) linkmacs
	$n(2) linkmacs
	$n(3) linkmacs
	[[$shl01 set channel_] drop-target] attach $trfd

	set tcp(0) [$ns create-connection TCP/Reno $n(1) TCPSink/DelAck $n(0) 0]
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

	# $ns at 0 "$ftp(0) start"
	$ns at 0 "$ftp(1) start"
	$ns at 0.02 "$ftp(2) start"
	# $ns at 0.03 "$ftp(3) start"
} else {
	set n(0) [$ns node]
	set n(1) [$ns node]
	set n(2) [$ns node]
	set n(3) [$ns node]
	set shl [$ns shared-duplex-link "$n(0) $n(1) $n(2) $n(3)" \
			$bw $delay DropTail $ll $ifq $mac]
	[[$shl set channel_] drop-target] attach $trfd

	set tcp(1) [$ns create-connection TCP/Reno $n(0) TCPSink $n(1) 0]
	set ftp(1) [$tcp(1) attach-source FTP]
	set tcp(2) [$ns create-connection TCP/Reno $n(0) TCPSink $n(2) 0]
	set ftp(2) [$tcp(2) attach-source FTP]
	set tcp(3) [$ns create-connection TCP/Reno $n(0) TCPSink $n(3) 0]
	set ftp(3) [$tcp(3) attach-source FTP]
	$ns at 0 "$ftp(1) start"
	$ns at 0.02 "$ftp(2) start"
	$ns at 0.03 "$ftp(3) start"

	set cbr(2:3) [$ns create-connection CBR $n(2) Null $n(3) 0]
	# $ns at 5.1 "$cbr(2:3) start"
}


#set qfile [open "q.dat" w]
#$shl init-monitor $ns $qfile 1 $n(0)
#$shl init-monitor $ns $qfile 1 $n(1)
#$ns at 0 "$shl queue-sample-timeout"


proc create-error {ns nodearray} {
	upvar $nodearray n
	foreach dst {1 2 3} {
		set em(0:$dst) [new ErrorModel]
		[[$ns link $n(0) $n($dst)] link] errormodel $em(0:$dst)
	}
	$em(0:1) set rate_ 0.01
	$em(0:2) set rate_ 0.02
	$em(0:3) set rate_ 0.03
}

if [info exist flags(e)] {
	create-error $ns n
}


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
		if ($1 == "-") {
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

	foreach af [lsort [glob ${trfile}.ack.*]] {
		exec tail -1 $af | gawk -v af=$af {
			{
				printf("%s\t%f\t%d\t%f\n", af, $1, $2, $2/$1);
			}
		} >> ${trfile}-bw
	}
	exec cat ${trfile}-bw >> /dev/stdout

	if [info exist flags(g)] {
		eval exec xgraph -nl -M -display $env(DISPLAY) \
				[lsort [glob $trfile.*]]
	}

	exit 0
}


$ns at $stop "finish"
$ns run

set gflag 0
set stop 100
set trfile temp
set bw 100Kb
set delay 10ms
set ll LL/Base
set ifq Queue/DropTail
set mac Mac/Multihop

if {$argc == 0} {
	puts "Usage: $argv0 \[-stop $stop\] \[-tr $trfile\]"
	puts "\t\[-bw $bw] \[-delay $delay\]"
	puts "\t\[-ll lltype\] \[-ifq ifqtype\] \[-mac mactype\]"
	exit 1
}

regexp {^(.+)\..*$} $argv0 match ext
for {set i 0} {$i < $argc} {incr i} {
	set opt [lindex $argv $i]
	if {$opt == "-stop"} {
		set stop [lindex $argv [incr i]]
	} elseif {$opt == "-g"} {
		set gflag 1
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
		regexp {^(.+)\..*$} $argv0 match ext
		set trfile [lindex $argv [incr i]]
	}
}


set trfd [open $trfile w]

set ns [new Simulator]
$ns trace-all $trfd

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

#$ns duplex-link $n0 $n1 2Mb 32us DropTail
set shl [$ns shared-duplex-link "$n0 $n1 $n2 $n3" $bw $delay DropTail $ll $ifq $mac]

set qfile [open "q.dat" w]
$shl init-monitor $ns $qfile 1 $n0
$shl init-monitor $ns $qfile 1 $n1
$ns at 0 "$shl queue-sample-timeout"


set tcpSource1 [new Agent/TCP/Reno]
$tcpSource1 set packetSize_ 1024
$ns attach-agent $n0 $tcpSource1
set ftp1 [new Source/FTP]
$ftp1 set agent_ $tcpSource1
set tcpSink1 [new Agent/TCPSink]
$ns attach-agent $n1 $tcpSink1
$ns connect $tcpSource1 $tcpSink1
$ns at 0.0 "$ftp1 start"

set tcpSource2 [new Agent/TCP/Reno]
$tcpSource2 set packetSize_ 1024
$ns attach-agent $n1 $tcpSource2
set ftp2 [new Source/FTP]
$ftp2 set agent_ $tcpSource2
set tcpSink2 [new Agent/TCPSink]
$ns attach-agent $n0 $tcpSink2
$ns connect $tcpSource2 $tcpSink2
$ns at 0.0 "$ftp2 start"

set tcp [new Agent/TCP]
$tcp set class_ 2
set sink [new Agent/TCPSink]
$ns attach-agent $n0 $tcp
$ns attach-agent $n1 $sink
$ns connect $tcp $sink
set ftp [new Source/FTP]
$ftp set agent_ $tcp
# $ns at 0.001 "$ftp start"

set tcp [new Agent/TCP]
set sink [new Agent/TCPSink]
$ns attach-agent $n2 $tcp
$ns attach-agent $n3 $sink
$ns connect $tcp $sink
set ftp [new Source/FTP]
$ftp set agent_ $tcp
# $ns at 0.1 "$ftp start"


set cbr1 [new Agent/CBR]
$ns attach-agent $n1 $cbr1
set cbr2 [new Agent/CBR]
$ns attach-agent $n2 $cbr2

set null1 [new Agent/Null]
$ns attach-agent $n0 $null1
set null2 [new Agent/Null]
$ns attach-agent $n0 $null2

$ns connect $cbr1 $null1
$ns connect $cbr2 $null2

#$ns at 5.1 "$cbr1 start"
#$ns at 6.1 "$cbr2 start"


$ns at $stop "finish"

proc finish {} {
	global ns env
	global gflag
	global trfd trfile

	$ns flush-trace
	close $trfd

	exec awk -v trfile=$trfile {
	BEGIN {
		trfile_cbr = sprintf("%s.cbr", trfile);
		trfile_cbr_r = sprintf("%s.cbr.r", trfile);
		trfile_tcp_0_1 = sprintf("%s.tcp.0.1", trfile);
		trfile_tcp_1_0 = sprintf("%s.tcp.1.0", trfile);
		trfile_ack_0_1 = sprintf("%s.ack.0.1", trfile);
		trfile_ack_1_0 = sprintf("%s.ack.1.0", trfile);
	}
	END {
		close(trfile_cbr);
		close(trfile_cbr_r);
		close(trfile_tcp_0_1);
		close(trfile_tcp_1_0);
		close(trfile_ack_0_1);
		close(trfile_ack_1_0);
	}
	{
		if ($1 == "-") {
			if ($5 == "cbr") {
				print $2, ($11) > trfile_cbr ;
			}
			else if ($5 == "tcp") {
				if (($3 == 0) && ($4 == 1)) {
					print $2, ($11) > trfile_tcp_0_1 ;
				}
				else if (($3 == 1) && ($4 == 0)) {
					print $2, ($11) > trfile_tcp_1_0 ;
				}
			}
			else if ($5 == "ack") {
				if (($3 == 0) && ($4 == 1)) {
					print $2, ($11) > trfile_ack_0_1 ;
				}
				else if (($3 == 1) && ($4 == 0)) {
					print $2, ($11) > trfile_ack_1_0 ;
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

	if {$gflag > 0} {
		eval exec xgraph -nl -M -display $env(DISPLAY) [glob $trfile.*]
	}
	exit 0
}

$ns run

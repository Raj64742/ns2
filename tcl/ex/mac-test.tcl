#!/bin/sh
# \
[ -x ns ] && nshome=.
# \
[ -x ../../ns ] && nshome=../..
# \
exec $nshome/ns "$0" "$@"

if [file executable ../../ns] {
	set nshome ../..
} else {
	set nshome .
}
source $nshome/tcl/lan/ns-mac.tcl
source $nshome/tcl/lan/ns-lan.tcl
set env(PATH) "$nshome/bin:$env(PATH)"


set flags(0) 0
set stop 10
set trfile out
set bw 100Kb
set delay 10ms
set ll LL
set ifq Queue/DropTail
set mac Mac/Multihop
set chan Channel

if {$argc == 0} {
	puts "Usage: $argv0 \[-stop $stop\] \[-seed #]"
	puts "\t\[-tr $trfile\]"
	puts "\t\[-ll lltype\] \[-ifq ifqtype\] \[-mac mactype\] \[-ch chan\]"
	puts "\t\[-bw $bw] \[-delay $delay\]"
	exit 1
}

proc get-options {argc argv} {
	global stop flags trfile
	global bw delay ll ifq mac chan
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
		} elseif {$opt == "-ch"} {
			set chan Channel/[lindex $argv [incr i]]
		} elseif {$opt == "-tr"} {
			set trfile [lindex $argv [incr i]]
		} elseif {$opt == "-seed"} {
			ns-random [lindex $argv [incr i]]
		} elseif {[string range $opt 0 0] == "-"} {
			set flags([string range $opt 1 end]) 1
		}
	}
	if {$mac == "Mac/Multihop"}  {
		set flags(mh) 1
	}
}

proc make-trace {} {
	global ns trfile trfd
	if [file exists ${trfile}] {
		exec touch ${trfile}.${trfile}
		eval exec rm $trfile [glob ${trfile}.*]
	}
	set trfd [open $trfile w]
	$ns trace-all $trfd
}

proc make-error {lan} {
	global ns n cbr ftp
	set ifq [$lan getifq $n(0)]
	foreach dst {1 2 3} {
		set em(0:$dst) [new ErrorModel]
		$ifq errormodel $em(0:$dst) [[$lan getmac $n($dst)] set label_]
	}
	$em(0:1) set rate_ 0.01
	$em(0:2) set rate_ 0.02
	$em(0:3) set rate_ 0.03
}

proc make-cbr-connections {} {
	global ns n cbr ftp
	set cbr(2:3) [$ns create-connection CBR $n(2) Null $n(3) 0]
}

proc make-tcp-connections {} {
	global ns n cbr ftp

	set tcp(0) [$ns create-connection TCP/Reno $n(1) TCPSink $n(0) 0]
	set ftp(0) [$tcp(0) attach-source FTP]
	set tcp(1) [$ns create-connection TCP/Reno $n(0) TCPSink $n(1) 0]
	set ftp(1) [$tcp(1) attach-source FTP]
	set tcp(2) [$ns create-connection TCP/Reno $n(0) TCPSink $n(2) 0]
	set ftp(2) [$tcp(2) attach-source FTP]
	set tcp(3) [$ns create-connection TCP/Reno $n(0) TCPSink $n(3) 0]
	set ftp(3) [$tcp(3) attach-source FTP]

	set tcpfd(1) [open ${trfile}-tcpstat.1 w]
	set tcpfd(2) [open ${trfile}-tcpstat.2 w]
	$tcp(1) trace $tcpfd(1)
	$tcp(2) trace $tcpfd(2)
}

proc start-connections {} {
	global ns n cbr ftp
	$ns at 0 "$ftp(0) start"
	$ns at 0.01 "$ftp(1) start"
	$ns at 0.02 "$ftp(2) start"
#	$ns at 0.03 "$ftp(3) start"
#	$ns at 5.1 "$cbr(2:3) start"
}

proc monitor-lan {lan} {
	global ns n cbr ftp
	set qfile [open "q.dat" w]
	$lan init-monitor $ns $qfile 1 $n(0)
	$lan init-monitor $ns $qfile 1 $n(1)
	$ns at 0 "$shl queue-sample-timeout"
}


get-options $argc $argv
set ns [new Simulator]
make-trace

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
			[$lan getmac $src] mode RTS_CTS
		}
	}
}
$lan trace $ns $trfd

if [info exists flags(monitor)] { monitor-lan $lan }
if [info exists flags(e)] { make-error $lan }
make-tcp-connections
make-cbr-connections
start-connections


proc finish {} {
	global env ns flags trfile trfd
	global bw delay ll ifq mac chan

	$ns flush-trace
	close $trfd
	exec trsplit -tt r -pt ack $trfile

	exec echo "	$bw $delay $ll $ifq $mac $chan" >> ${trfile}-bw
	foreach af [lsort [glob ${trfile}.ack.*]] {
		exec tail -1 $af | awk -v af=$af { {
			kbps = 8 * $2 / $1;
			printf("%s\t%f\t%d\t%f\t%f\n", af,$1,$2, $2/$1, kbps);
		} } >> ${trfile}-bw
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

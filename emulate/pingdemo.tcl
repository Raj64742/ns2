#
# pingdemo.tcl
#
# demonstration of the emulation facilities using the icmp
# echo responder (also uses the arp responder)
#
# K Fall, 10/11/98
# kfall@cs.berkeley.edu
#

Class PingDemo

PingDemo instproc init {} {
	$self next
	$self instvar myname myaddr dotrace stoptime owdelay ifname
	$self instvar traceallfile tracenamfile

	set myname "bit.ee.lbl.gov";	# unused ip address
	set myaddr [exec host $myname | sed "s/.*address\ //"]
	set dotrace 1
	set stoptime 200.0
	set owdelay 1000ms
	set ifname fxp0

	set traceallfile em-all.tr
	set tracenamfile em.nam
	puts ""
}

PingDemo instproc syssetup {} {
	puts "turning off IP forwarding and ICMP redirect generation."
	exec sysctl -w net.inet.ip.forwarding=0 net.inet.ip.redirect=0
}

PingDemo instproc emsetup {} {
	$self instvar myname myaddr dotrace stoptime owdelay ifname
	$self instvar traceallfile tracenamfile ns

	puts "I am $myname with IP address $myaddr."

	set ns [new Simulator]

	if { $dotrace } {
		exec rm -f $traceallfile $tracenamfile
		set allchan [open $traceallfile w]
		$ns trace-all $allchan
		set namchan [open $tracenamfile w]
		$ns namtrace-all $namchan
	}

	$ns use-scheduler RealTime

	set bpf2 [new Network/Pcap/Live]; #	used to read IP info
	$bpf2 set promisc_ true
	set dev2 [$bpf2 open readonly $ifname]

	set ipnet [new Network/IP];	 #	used to write IP pkts
	$ipnet open writeonly

	set arpagent [new ArpAgent]
	$arpagent config $ifname
	set myether [$arpagent set myether_]
	puts "Arranging to proxy ARP for $myaddr on my ethernet $myether."
	$arpagent insert $myaddr $myether publish

	#
	# try to filter out unwanted stuff like netbios pkts, dns, etc
	#

	$bpf2 filter "icmp and dst $myaddr"

	set pfa1 [new Agent/Tap]
	set pfa2 [new Agent/Tap]
	set ipa [new Agent/Tap]
	set echoagent [new Agent/PingResponder]

	$pfa2 set fid_ 0
	$ipa set fid_ 1

	$pfa2 network $bpf2
	$ipa network $ipnet

	#
	# topology creation
	#

	set node0 [$ns node]
	set node1 [$ns node]
	set node2 [$ns node]

	$ns simplex-link $node0 $node1 1Mb $owdelay DropTail
	$ns simplex-link $node1 $node0 1Mb $owdelay DropTail
	$ns simplex-link $node0 $node2 1Mb $owdelay DropTail
	$ns simplex-link $node2 $node0 1Mb $owdelay DropTail
	$ns simplex-link $node1 $node2 1Mb $owdelay DropTail
	$ns simplex-link $node2 $node1 1Mb $owdelay DropTail

	#
	# attach-agent winds up calling $node attach $agent which does
	# these things:
	#	append agent to list of agents in the node
	#	sets target in the agent to the entry of the node
	#	sets the node_ field of the agent to the node
	#	if not yet created,
	#		create port demuxer in node (Addr classifier),
	#		put in dmux_
	#		call "$self add-route $id_ $dmux_"
	#			installs id<->dmux mapping in classifier_
	#	allocate a port
	#	set agent's port id and address
	#	install port-agent mapping in dmux_
	#	
	#	
	$ns attach-agent $node0 $pfa2; #	packet filter agent
	$ns attach-agent $node1 $ipa; # ip agent (for sending)
	$ns attach-agent $node2 $echoagent

	$ns simplex-connect $pfa2 $echoagent
	$ns simplex-connect $echoagent $ipa
}

PingDemo instproc run {} {

	$self instvar ns myaddr owdelay ifname
	$self syssetup
	$self emsetup

	puts "listening for pings on addr $myaddr, 1-way link delay: $owdelay\n"

	$ns run
}

PingDemo thisdemo
thisdemo run

# Copyright (C) 1999 by USC/ISI
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 
# Re-tooled version of Polly's web traffic models (tcl/ex/web-traffic.tcl, 
# tcl/ex/large-scale-traffic.tcl) in order to save memory.
#
# The main strategy is to move everything into C++, only leave an OTcl 
# configuration interface. Be very careful as what is configuration and 
# what is functionality.
#
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/webcache/webtraf.tcl,v 1.5 2000/11/21 04:26:44 ratul Exp $

PagePool/WebTraf set debug_ false
PagePool/WebTraf set TCPTYPE_ Reno
PagePool/WebTraf set TCPSINKTYPE_ TCPSink   ;#required for SACK1 Sinks.

#0 for default, fid=id
#1 for increasing along with new agent allocation (as in red-pd). 
#   useful when used with flow monitors (as they tend to run out of space in hash tables)
#   (see red-pd scripts for usage of this)
#2 for assigning the same fid to all connections.
#    useful when want to differentiate between various web traffic generators using flow monitors.
#   (see pushback scripts for usage of this).
PagePool/WebTraf set FID_ASSIGNING_MODE_ 0 

PagePool/WebTraf instproc launch-req { id clnt svr ctcp csnk stcp ssnk size } {
	set ns [Simulator instance]

	$ns attach-agent $svr $stcp
	$ns attach-agent $clnt $ssnk
	$ns connect $stcp $ssnk
	
	$ns attach-agent $clnt $ctcp
	$ns attach-agent $svr $csnk
	$ns connect $ctcp $csnk

	if {[PagePool/WebTraf set FID_ASSIGNING_MODE_] == 0} {
		$stcp set fid_ $id
		$ctcp set fid_ $id
	}

	#modified for web traffic flow trace. 
	#puts "launch request $id: [$clnt id] -> [$svr id] size $size at [$ns now] cfid [$ctcp set fid_] sfid [$stcp set fid_] "
	# puts "  client: $ctcp $csnk"
	# puts "  server: $stcp $ssnk"

	$ctcp proc done {} "$self done-req $id $clnt $svr $ctcp $csnk $stcp $size"
	$stcp proc done {} "$self done-resp $id $clnt $svr $stcp $ssnk $size [$ns now]"
	
	# Send a single packet as a request
	$ctcp advanceby 1
}

PagePool/WebTraf instproc done-req { id clnt svr ctcp csnk stcp size } {
	set ns [Simulator instance]

	# modified to trace web traffic flow
	# puts "done request $id : [$clnt id] -> [$svr id] at [$ns now]"

	# Recycle client-side TCP agents
	$ns detach-agent $clnt $ctcp
	$ns detach-agent $svr $csnk
	$ctcp reset
	$csnk reset
	$self recycle $ctcp $csnk
#	puts "recycled $ctcp $csnk"
	# Advance $size packets
	$stcp advanceby $size
}

PagePool/WebTraf instproc done-resp { id clnt svr stcp ssnk size startTime} {
	set ns [Simulator instance]

	#modified to trace web traffic flow.
	#puts "done response $id : [$clnt id] -> [$svr id] at [$ns now] size $size duration [expr [$ns now] - $startTime]"

	# Recycle server-side TCP agents
	$ns detach-agent $clnt $ssnk
	$ns detach-agent $svr $stcp
	$stcp reset
	$ssnk reset
	$self recycle $stcp $ssnk
#	puts "recycled $stcp $ssnk"
}

# XXX Should allow customizable TCP types. Can be easily done via a 
# class variable
PagePool/WebTraf instproc alloc-tcp {} {
	
	set tcp [new Agent/TCP/[PagePool/WebTraf set TCPTYPE_]]
	
	set fidMode [PagePool/WebTraf set FID_ASSIGNING_MODE_]
	if {$fidMode == 1} {
		$self instvar maxFid_
		$tcp set fid_ $maxFid_
		incr maxFid_
	} elseif  {$fidMode == 2} {
		$self instvar sameFid_
		$tcp set fid_ $sameFid_
	}
    
	return $tcp
}

PagePool/WebTraf instproc alloc-tcp-sink {} {
	return [new Agent/[PagePool/WebTraf set TCPSINKTYPE_]]
}

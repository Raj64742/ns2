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
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/webcache/webtraf.tcl,v 1.3 2000/09/07 19:56:07 haoboy Exp $

PagePool/WebTraf set debug_ false
PagePool/WebTraf set TCPTYPE_ Reno

PagePool/WebTraf instproc launch-req { id clnt svr ctcp csnk stcp ssnk size } {
	set ns [Simulator instance]

#	modified for web traffic flow trace. 
#	puts "launch request $id : [$clnt id] -> [$svr id] size $size at [$ns now]"
#	puts "  client: $ctcp $csnk"
#	puts "  server: $stcp $ssnk"

	$ns attach-agent $clnt $ctcp
	$ns attach-agent $svr $csnk
	$ns connect $ctcp $csnk
	$ctcp set fid_ $id

	$ns attach-agent $svr $stcp
	$ns attach-agent $clnt $ssnk
	$ns connect $stcp $ssnk
	$stcp set fid_ $id

	$ctcp proc done {} "$self done-req $id $clnt $svr $ctcp $csnk $stcp $size"
	$stcp proc done {} "$self done-resp $id $clnt $svr $stcp $ssnk"

	# Send a single packet as a request
	$ctcp advanceby 1
}

PagePool/WebTraf instproc done-req { id clnt svr ctcp csnk stcp size } {
	set ns [Simulator instance]

#	modified to trace web traffic flow
#	puts "done request $id : [$clnt id] -> [$svr id] at [$ns now]"

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

PagePool/WebTraf instproc done-resp { id clnt svr stcp ssnk } {
	set ns [Simulator instance]

#	modified to trace web traffic flow.
#	puts "done response $id : [$clnt id] -> [$svr id] at [$ns now]"

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
	return [new Agent/TCP/[PagePool/WebTraf set TCPTYPE_]]
}

PagePool/WebTraf instproc alloc-tcp-sink {} {
	return [new Agent/TCPSink]
}

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
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/webcache/webtraf.tcl,v 1.1 1999/09/24 23:50:13 haoboy Exp $

PagePool/WebTraf instproc launch-req { id clnt svr ctcp csnk stcp ssnk size } {
#	puts "launch request [$clnt id] [$svr id] size $size"
#	puts "  client: $ctcp $csnk"
#	puts "  server: $stcp $ssnk"

	set ns [Simulator instance]
	$ns attach-agent $clnt $ctcp
	$ns attach-agent $svr $csnk
	$ns connect $ctcp $csnk
	$ctcp set fid_ $id

	$ns attach-agent $svr $stcp
	$ns attach-agent $clnt $ssnk
	$ns connect $stcp $ssnk
	$stcp set fid_ $id

	$ctcp proc done {} "$self done-req $clnt $svr $ctcp $csnk $stcp $size"
	$stcp proc done {} "$self done-resp $clnt $svr $stcp $ssnk"

	# Send a single packet as a request
	$ctcp advanceby 1
}

PagePool/WebTraf instproc done-req { clnt svr ctcp csnk stcp size } {
#	puts "done request [$clnt id] [$svr id]"

	set ns [Simulator instance]
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

PagePool/WebTraf instproc done-resp { clnt svr stcp ssnk } {
#	puts "done response [$clnt id] [$svr id]"

	set ns [Simulator instance]
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
	return [new Agent/TCP/Reno]
}

PagePool/WebTraf instproc alloc-tcp-sink {} {
	return [new Agent/TCPSink]
}

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
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/webcache/webtraf.tcl,v 1.8 2001/08/17 05:22:31 xuanc Exp $

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
PagePool/WebTraf set VERBOSE_ 0

# modified to trace web traffic flows (request and response).
# used to evaluate SFD algorithm.
PagePool/WebTraf set REQ_TRACE_ 0
PagePool/WebTraf set RESP_TRACE_ 0

PagePool/WebTraf instproc launch-req { id pid clnt svr ctcp csnk stcp ssnk size } {
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
    
    $ctcp proc done {} "$self done-req $id $pid $clnt $svr $ctcp $csnk $stcp $size"
    $stcp proc done {} "$self done-resp $id $pid $clnt $svr $stcp $ssnk $size [$ns now] [$stcp set fid_]"
	
    # modified to trace web traffic flows (send request: client==>server).
    if {[PagePool/WebTraf set REQ_TRACE_]} {	
	puts "req + $id $size $pid [$clnt id] [$svr id] [$ns now]"
    }	
    # Send a single packet as a request
    $ctcp advanceby 1
}

PagePool/WebTraf instproc done-req { id pid clnt svr ctcp csnk stcp size } {
    set ns [Simulator instance]
    
    # modified to trace web traffic flows (recv request: client==>server).
    if {[PagePool/WebTraf set REQ_TRACE_]} {	
        	puts "req - $id $size $pid [$clnt id] [$svr id] [$ns now]"
    }
    
    # Recycle client-side TCP agents
    $ns detach-agent $clnt $ctcp
    $ns detach-agent $svr $csnk
    $ctcp reset
    $csnk reset
    $self recycle $ctcp $csnk
    #	puts "recycled $stcp $ssnk"
    
    # modified to trace web traffic flows (send responese: server->client).
    if {[PagePool/WebTraf set RESP_TRACE_]} {
	puts "resp + $id $size $pid [$svr id] [$clnt id] [$ns now]"
    }
    
    # Advance $size packets
    $stcp advanceby $size
}

PagePool/WebTraf instproc done-resp { id pid clnt svr stcp ssnk size {startTime 0} {fid 0}} {
    set ns [Simulator instance]
    
    # modified to trace web traffic flows (recv responese: server->client).
    if {[PagePool/WebTraf set RESP_TRACE_]} {
	puts "resp - $id $size $pid [$svr id] [$clnt id] [$ns now]"
    }
    
    if {[PagePool/WebTraf set VERBOSE_] == 1} {
	puts "done-resp - $id [$svr id] [$clnt id] $size $startTime [$ns now] $fid"
    }
    
    # Recycle server-side TCP agents
    $ns detach-agent $clnt $ssnk
    $ns detach-agent $svr $stcp
    $stcp reset
    $ssnk reset
    $self recycle $stcp $ssnk
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

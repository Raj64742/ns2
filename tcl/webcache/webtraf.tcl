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
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/webcache/webtraf.tcl,v 1.15 2002/03/21 23:21:10 ddutta Exp $

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

# Support the reuse of page level attributes to save
#  memory for large simulations
PagePool/WebTraf set recycle_page_ 1

# To use fullTCP (xuanc), we need to:
# 1. set the flag fulltcp_ to 1
# 2. set TCPTYPE_ FullTcp
PagePool/WebTraf set fulltcp_ 0

# modified to trace web traffic flows (request and response).
# used to evaluate SFD algorithm.
PagePool/WebTraf set REQ_TRACE_ 0
PagePool/WebTraf set RESP_TRACE_ 0

# The threshold to classify short and long flows (in TCP packets, ie 15KB)
PagePool/WebTraf set FLOW_SIZE_TH_ 15
# Option to modify traffic mix:
# 0: original settings without change
# 1: Allow only short flows
# 2: Chop long flows to short ones.
PagePool/WebTraf set FLOW_SIZE_OPS_ 0

PagePool/WebTraf instproc launch-req { id pid clnt svr ctcp csnk stcp ssnk size pobj} {
    set launch_req 1
    set flow_th [PagePool/WebTraf set FLOW_SIZE_TH_]

    if {[PagePool/WebTraf set FLOW_SIZE_OPS_] == 1 && $size > $flow_th} {
	set launch_req 0
    }

    if {$launch_req == 1} {
	set ns [Simulator instance]
	
	$ns attach-agent $svr $stcp
	$ns attach-agent $clnt $ssnk
	$ns connect $stcp $ssnk
	
	$ns attach-agent $clnt $ctcp
	$ns attach-agent $svr $csnk
	$ns connect $ctcp $csnk
	
	# sink needs to listen for fulltcp
	if {[PagePool/WebTraf set fulltcp_] == 1} {
	    $csnk listen
	    $ssnk listen
	}

	if {[PagePool/WebTraf set FID_ASSIGNING_MODE_] == 0} {
	    $stcp set fid_ $id
	    $ctcp set fid_ $id

	    if {[PagePool/WebTraf set fulltcp_] == 1} {
		$csnk set fid_ $id
		$ssnk set fid_ $id
	    }
	}

	if {[PagePool/WebTraf set FLOW_SIZE_OPS_] == 2 && $size > $flow_th} {
	    $ctcp proc done {} "$self done-req $id $pid $clnt $svr $ctcp $csnk $stcp $size $flow_th $pobj"
	    $stcp proc done {} "$self done-resp $id $pid $clnt $svr $stcp $ssnk $size $flow_th $flow_th [$ns now] [$stcp set fid_] $pobj"
	} else {
	    $ctcp proc done {} "$self done-req $id $pid $clnt $svr $ctcp $csnk $stcp $size $size $pobj"
	    $stcp proc done {} "$self done-resp $id $pid $clnt $svr $stcp $ssnk $size $size $flow_th [$ns now] [$stcp set fid_] $pobj"
	}
	
#	$ctcp proc done {} "$self done-req $id $pid $clnt $svr $ctcp $csnk $stcp $size"
#	$stcp proc done {} "$self done-resp $id $pid $clnt $svr $stcp $ssnk $size [$ns now] [$stcp set fid_]"
	
	# modified to trace web traffic flows (send request: client==>server).
	if {[PagePool/WebTraf set REQ_TRACE_]} {	
	    puts "req + $id $pid $size [$clnt id] [$svr id] [$ns now]"
	}	

	# Send a single packet as a request
	$self send-message $ctcp 1
    }
}

PagePool/WebTraf instproc done-req { id pid clnt svr ctcp csnk stcp size sent pobj} {
    set ns [Simulator instance]
    

    # modified to trace web traffic flows (recv request: client==>server).
    if {[PagePool/WebTraf set REQ_TRACE_]} {	
	puts "req - $id $pid $size [$clnt id] [$svr id] [$ns now]"
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
	puts "resp + $id $pid $sent $size [$svr id] [$clnt id] [$ns now]"
    }
    
    # Send $sent packets
    $self send-message $stcp $sent
}

PagePool/WebTraf instproc done-resp { id pid clnt svr stcp ssnk size sent sent_th {startTime 0} {fid 0} pobj} {
    set ns [Simulator instance]
    
    # modified to trace web traffic flows (recv responese: server->client).
    if {[PagePool/WebTraf set RESP_TRACE_]} {
	puts "resp - $id $pid $sent $size [$svr id] [$clnt id] [$ns now]"
    }
    
    if {[PagePool/WebTraf set VERBOSE_] == 1} {
	puts "done-resp - $id [$svr id] [$clnt id] $size $startTime [$ns now] $fid"
    }
    
    # Reset server-side TCP agents
    $stcp reset
    $ssnk reset
    $ns detach-agent $clnt $ssnk
    $ns detach-agent $svr $stcp

    # if there are some packets left, keeps on sending.
    if {$sent < $size} {
	$ns attach-agent $svr $stcp
	$ns attach-agent $clnt $ssnk
	$ns connect $stcp $ssnk

	if {[PagePool/WebTraf set fulltcp_] == 1} {
	    $ssnk listen
	}

	if {[PagePool/WebTraf set FID_ASSIGNING_MODE_] == 0} {
	    $stcp set fid_ $id

	    if {[PagePool/WebTraf set fulltcp_] == 1} {
		$ssnk set fid_ $id
	    }
	}

	set left [expr $size - $sent]
	if {$left <= $sent_th} {
	    # modified to trace web traffic flows (send responese: server->client).
	    if {[PagePool/WebTraf set RESP_TRACE_]} {
		puts "resp + $id $pid $left $size [$svr id] [$clnt id] [$ns now]"
	    }
	    set sent [expr $sent + $left]
	    $stcp proc done {} "$self done-resp $id $pid $clnt $svr $stcp $ssnk $size $sent $sent_th [$ns now] [$stcp set fid_]"

	    $self send-message $stcp $left
	} else {
	    # modified to trace web traffic flows (send responese: server->client).
	    if {[PagePool/WebTraf set RESP_TRACE_]} {
		puts "resp + $id $pid $sent_th $size [$svr id] [$clnt id] [$ns now]"
	    }
	    set sent [expr $sent + $sent_th]
	    $stcp proc done {} "$self done-resp $id $pid $clnt $svr $stcp $ssnk $size $sent $sent_th [$ns now] [$stcp set fid_]"

	    $self send-message $stcp $sent_th
	}
    } else {
	# Recycle server-side TCP agents
	$self recycle $stcp $ssnk	
	$self doneObj $pobj
    }
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

PagePool/WebTraf instproc send-message {tcp num_packet} {
    if {[PagePool/WebTraf set fulltcp_] == 1} {
	# for fulltcp: need to use flag
	$tcp sendmsg [expr $num_packet * 1000] "MSG_EOF"
    } else {
	#Advance $num_packet packets: for one-way tcp
	$tcp advanceby $num_packet
    }
}


# Debo

#PagePool/WebTraf instproc create-session { args } {
#
#    set ns [Simulator instance]
#    set asimflag [$ns set useasim_]
#    #puts "Here"
#    $self use-asim
#    $self cmd create-session $args
#
#} 

PagePool/WebTraf instproc  add2asim { srcid dstid lambda mu } {

    set sf_ [[Simulator instance] set sflows_]
    set nsf_ [[Simulator instance] set nsflows_]
    
    lappend sf_ $srcid:$dstid:$lambda:$mu
    incr nsf_

    [Simulator instance] set sflows_ $sf_
    [Simulator instance] set nsflows_ $nsf_

    #puts "setup short flow .. now sflows_ = $sf_"

}

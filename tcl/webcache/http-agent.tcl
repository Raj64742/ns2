# Copyright (c) Xerox Corporation 1998. All rights reserved.
#
# License is granted to copy, to use, and to make and to use derivative
# works for research and evaluation purposes, provided that Xerox is
# acknowledged in all documentation pertaining to any such copy or
# derivative work. Xerox grants no other licenses expressed or
# implied. The Xerox trade name should not be used in any advertising
# without its written permission. 
#
# XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
# MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
# FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
# express or implied warranty of any kind.
#
# These notices must be retained in any copies of any part of this
# software. 
#
# HTTP agents: server, client, cache
#
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/webcache/http-agent.tcl,v 1.3 1998/09/30 01:26:17 haoboy Exp $

Http set id_ 0	;# required by TclCL
# Type of Tcp agent. Can be SimpleTcp or FullTcp
Http set TRANSPORT_ SimpleTcp
Http set HB_FID_ 40
Http set PINV_FID_ 41

Http/Server set id_ 0
Http/Server/Inval set id_ 0
Http/Server/Inval/Yuc set hb_interval_ 60
Http/Server/Inval/Yuc set enable_upd_ 0
Http/Server/Inval/Yuc set Ca_ 1
Http/Server/Inval/Yuc set Cb_ 4
Http/Server/Inval/Yuc set push_thresh_ 4

Http/Cache set id_ 0
Http/Cache/Inval set id_ 0
Http/Cache/Inval/Mcast set hb_interval_ 60
Http/Cache/Inval/Mcast set upd_interval_ 5
Http/Cache/Inval/Mcast set enable_upd_ 0
Http/Cache/Inval/Mcast set Ca_ 1
Http/Cache/Inval/Mcast set Cb_ 4
Http/Cache/Inval/Mcast set push_thresh_ 4
Http/Cache/Inval/Mcast/Perc set direct_request_ 0

PagePool/CompMath set num_pages_ 1
PagePool/CompMath set main_size_ 1024
PagePool/CompMath set comp_size_ 10240

Http instproc init { ns node } {
	$self next
	$self init-instvar id_
	$self instvar ns_ node_
	set ns_ $ns
	set node_ $node
	$self set id_ [$node_ id]
	$self set-pagepool [new PagePool/Client]
}

Http instproc addr {} {
	$self instvar node_ 
	return [$node_ node-addr]
}

Http set fid_ -1
Http instproc getfid {} {
	$self instvar fid_
	set fid_ [Http set fid_]
	Http set fid_ [incr fid_]
}

# XXX invalidation message size should be proportional to the number of
# invalidations inside the message
Http set INVSize_ 43	;# unicast invalidation
Http set REQSize_ 43	;# Request
Http set REFSize_ 50	;# Refetch request
Http set IMSSize_ 50	;# If-Modified-Since
Http set JOINSize_ 10	;# Server join/leave
Http set HBSize_ 1	;# Used by Http/Server/Inval only
Http set PFSize_ 1	;# Pro forma
Http set NTFSize_ 10	;# Request Notification
Http set MPUSize_ 10	;# Mandatory push request

Http instproc get-mpusize {} {
	return [Http set MPUSize_]
}

Http instproc get-ntfsize {} {
	return [Http set NTFSize_]
}

Http instproc get-pfsize {} {
	return [Http set PFSize_]
}

Http instproc get-hbsize {} {
	return [Http set HBSize_]
}

Http instproc get-imssize {} {
	return [Http set IMSSize_]
}

Http instproc get-invsize {} {
	return [Http set INVSize_]
}

# Generate request packet size. Should be constant because it's small
Http instproc get-reqsize {} {
	return [Http set REQSize_]
}

Http instproc get-refsize {} {
	return [Http set REFSize_]
}

Http instproc get-joinsize {} {
	return [Http set JOINSize_]
}

# At startup, connect to a server, the server may be a cache
Http instproc connect { server } {
	Http instvar TRANSPORT_
	$self instvar ns_ slist_ node_ fid_ id_

	lappend slist_ $server
	set tcp [new Agent/TCP/$TRANSPORT_]
	$tcp set fid_ [$self getfid]
	$ns_ attach-agent $node_ $tcp

	set ret [$server alloc-connection $self $fid_]
	set snk [$ret agent]
	$ns_ connect $tcp $snk
	$tcp set dst_ [$snk set addr_]
	$tcp set window_ 100
	$snk listen

	# Use a wrapper to implement application data transfer
	set wrapper [new Application/TcpApp $tcp]
	$self cmd connect $server $wrapper
	$wrapper connect $ret

	#puts "HttpApp $id_ connected to server [$server id]"
}


Class Http/Client -superclass Http

# Used for mandatory push refreshments
Http/Client set hb_interval_ 60

Http/Client instproc init args {
	eval $self next $args
	$self instvar node_ stat_
	$node_ color "SteelBlue"
	array set stat_ [list req-num 0 stale-num 0 stale-time 0 rep-time 0]
}

Http/Client instproc stat { name } {
	$self instvar stat_
	return $stat_($name)
}

# XXX Should add pending_ handling into disconnect
Http/Client instproc disconnect { server } {
	$self instvar ns_ slist_ 
	set pos [lsearch $slist_ $server]
	if {$pos >= 0} {
		lreplace $slist_ $pos $pos
	} else { 
		error "Http::disconnect: not connected to $server"
	}

	# Clean up all pending requests
	# XXX Assuming a client either connects to a *SINGLE* cache 
	# or a *SINGLE* server, then we remove all pending requests 
	# when we are disconnected. 
	$self instvar pending_
	unset pending_

	set tcp [[$self get-cnc $server] agent]
	$tcp close
	#delete $tcp	;# Should we do a destroy here?
	$self cmd disconnect $server
	$server disconnect $self
}

# Meta-data to be sent in a request
# XXX pageid should always be given from the users, because client may 
# connect to a cache, hence it doesn't know the server name.
Http/Client instproc send-request { server type pageid args } {
	$self instvar ns_ pending_ 	;# unansewered requests

	if ![info exists pending_($pageid)] { 
		# XXX Actually we should use set, because only one request
		# is allowed for a page simultaneously
		lappend pending_($pageid) [$ns_ now]
	} else {
		# If the page is being requested, do not send another request
		return
	}

	set size [$self get-reqsize]
	$self send $server $size \
	    "$server get-request $self $type $pageid size $size [join $args]"
	$self evTrace C GET p $pageid s [$server id] z $size
	$self instvar stat_
	incr stat_(req-num)

	$self mark-request $pageid
}

Http/Client instproc mark-request { pageid } {
	# Nam state coloring
	$self instvar node_ marks_ ns_
	$node_ add-mark $pageid:[$ns_ now] "purple"
	lappend marks_($pageid) $pageid:[$ns_ now]
}

# The reason that "type" is here is for Http/Cache to work. Client doesn't 
# check the reason of the response
Http/Client instproc get-response-GET { server pageid args } {
	$self instvar pending_ id_ ns_ stat_

	if ![info exists pending_($pageid)] {
		error "Client $id_: Unrequested response page $pageid from server [$server id]"
	}

	array set data $args

	# Check stale hits
	set origsvr [lindex [split $pageid :] 0]
	set modtime [$origsvr get-modtime $pageid]
	if {$modtime > $data(modtime)} {
		# Staleness is the time from now to the time it's last modified
		set tmp [$origsvr stale-time $pageid $data(modtime)]
		$self evTrace C STA p $pageid s [$origsvr id] l $tmp
		incr stat_(stale-num)
		set stat_(stale-time) [expr $stat_(stale-time) + $tmp]
	}
	# Assume this response is for the very first request we've sent. 
	# Because we'll average the response time at the end, which 
	# request this response actually corresponds to doesn't matter.
	set pt [lindex $pending_($pageid) 0]
	$self evTrace C RCV p $pageid s [$server id] l \
		[expr [$ns_ now] - $pt] z $data(size)
	set stat_(rep-time) [expr $stat_(rep-time) + [$ns_ now] - $pt]

	set pending_($pageid) [lreplace $pending_($pageid) 0 0]
	if {[llength $pending_($pageid)] == 0} {
		unset pending_($pageid)
	}
	$self mark-response $pageid
}

Http/Client instproc mark-response { pageid } {
	$self instvar node_ marks_ ns_
	set mk [lindex $marks_($pageid) 0]
	$node_ delete-mark $mk
	set marks_($pageid) [lreplace $marks_($pageid) 0 0]
}

Http/Client instproc get-response-REF { server pageid args } {
	eval $self get-response-GET $server $pageid $args
}

Http/Client instproc get-response-IMS { server pageid args } {
	eval $self get-response-GET $server $pageid $args
}

# Assuming everything is setup, this function starts sending requests
# Sending a request to $cache, the original page should come from $server
#
# Populate a cache with all available pages
# XXX how would we distribute pages spatially when we have a hierarchy 
# of caches? Or, how would we distribute client requests spatially?
# It should be used in single client, single cache and single server case
# *ONLY*.
Http/Client instproc start-session { cache server } {
	$self instvar reqRanvar_ ns_ cache_

	set cache_ $cache
	set pageid $server:[$self gen-reqpageid]
	$self send-request $cache GET $pageid
	$ns_ at [expr [$ns_ now] + [$reqRanvar_ value]] \
		"$self start-session $cache $server"
}

Http/Client instproc populate { cache server } {
	$self instvar pgtr_ curpage_ status_ ns_

	if ![info exists status_] {
		set status_ "POPULATE"
		set curpage_ 0
	}

	if [info exists pgtr_] {
		# Populate cache with all pages incrementally
		if {$curpage_ < [$pgtr_ get-poolsize]} {
			$self send-request $cache GET $server:$curpage_
			incr curpage_
			$ns_ at [expr [$ns_ now] + 1] \
				"$self populate $cache $server"
		} else {
			#puts "At [$ns_ now], simulation starts"
			$self start-session $cache $server
		}
	} else {
		# One page only
		$self send-request $cache GET $server:$curpage_
		$ns_ at [expr [$ns_ now] + 1] \
			"$self start-session $cache $server"
	}
}

Http/Client instproc start { cache server } {
	$self instvar cache_
	set cache_ $cache
	$self populate $cache $server
}

# Generate the time when next request will occur
# It's either a TracePagePool or a MathPagePool
#
# XXX both TracePagePool and MathPagePool should share the same C++ 
# interface and OTcl interface
Http/Client instproc set-page-generator { pagepool } {
	$self instvar pgtr_ 	;# Page generator
	set pgtr_ $pagepool
}

# Generate request page id. Should follow Zipf's law here.
Http/Client instproc gen-reqpageid {} {
	$self instvar pgtr_

	if [info exists pgtr_] {
		return [$pgtr_ gen-pageid]
	} else {
		return 0
	}
}

Http/Client instproc set-interval-generator { ranvar } {
	$self instvar reqRanvar_
	set reqRanvar_ $ranvar
}

Http/Client instproc request-mpush { page } {
	$self instvar mpush_refresh_ ns_ cache_
	$self send $cache_ [$self get-mpusize] \
		"$cache_ request-mpush $page"
	Http/Client instvar hb_interval_
	set mpush_refresh_($page) [$ns_ at [expr [$ns_ now] + $hb_interval_] \
		"$self send-refresh-mpush $page"]
}

Http/Client instproc send-refresh-mpush { page } {
	$self instvar mpush_refresh_ ns_ cache_
	$self send $cache_ [$self get-mpusize] "$cache_ refresh-mpush $page"
	#puts "[$ns_ now]: Client [$self id] send mpush refresh"
	Http/Client instvar hb_interval_
	set mpush_refresh_($page) [$ns_ at [expr [$ns_ now] + $hb_interval_] \
		"$self send-refresh-mpush $page"]
}

# XXX We use explicit teardown. 
Http/Client instproc stop-mpush { page } {
	$self instvar mpush_refresh_ ns_ cache_

	if [info exists mpush_refresh_($page)] {
		# Stop sending the periodic refreshment
		$ns_ cancel $mpush_refresh_($page)
		puts "[$ns_ now]: Client [$self id] stops mpush"
	} else {
		error "no mpush to cancel!"
	}
	# Send explicit message up to tear down
	$self send $cache_ [$self get-mpusize] "$cache_ stop-mpush $page"
}


#----------------------------------------------------------------------
# Client which is capable of handling compound pages
#----------------------------------------------------------------------
Class Http/Client/Compound -superclass Http/Client

# XXX We need to override get-response-GET because we need to recompute
#     the RCV time, and the STA time, etc. 
# XXX Allow only *ONE* compound page.
Http/Client/Compound instproc get-response-GET { server pageid args } {
	$self instvar pending_ id_ ns_ num_cpage_ max_stale_ stat_

	if ![info exists pending_($pageid)] {
		error "Client $id_: Unrequested response page $pageid from server/cache [$server id]"
	}

	set origsvr [lindex [split $pageid :] 0]
	set id [lindex [split $pageid :] end]
	if {$id == 0} {
		# Now we get the first page of a compound page, get the rest
		$self instvar pgtr_ cache_
		set num_cpage_ [$pgtr_ set num_pages_]
		for {set i 1} {$i < $num_cpage_} {incr i} {
			$self send-request $cache_ GET $origsvr:$i
		}
	}

	array set data $args

	# Check stale hits and record maximum stale hit time
	set modtime [$origsvr get-modtime $pageid]
	if {$modtime > $data(modtime)} {
		$self instvar ns_
		# Staleness is the time from now to the time it's last modified
		set tmp [$origsvr stale-time $pageid $data(modtime)]
		if ![info exists max_stale_] {
			set max_stale_ $tmp
		} elseif {$max_stale_ < $tmp} {
			set max_stale_ $tmp
		}
	}

	# Check if we have any pending component page.
	incr num_cpage_ -1
	if {$num_cpage_ == 0} {
		# Now we've received all pages
		$self instvar pgtr_
		# Record response time for the whole page
		set pt [lindex $pending_($origsvr:0) 0]
		$self evTrace C RCV p $origsvr:0 s [$origsvr id] l \
				[expr [$ns_ now] - $pt] z $data(size)
		set stat_(rep-time) [expr $stat_(rep-time) + [$ns_ now] - $pt]
		# Delete all pending records
		for {set i 0} {$i < [$pgtr_ set num_pages_]} {incr i} {
			set pid $origsvr:$i
			set pending_($pid) [lreplace $pending_($pid) 0 0]
			if {[llength $pending_($pid)] == 0} {
				unset pending_($pid)
			}
		}
		if [info exists max_stale_] {
			$self evTrace C STA p $origsvr:0 s [$origsvr id] \
				l $max_stale_
			incr stat_(stale-num)
			set stat_(stale-time) [expr $stat_(stale-time) + $max_stale_]
			unset max_stale_
		}
		$self mark-response $origsvr:0
	}
}

Http/Client/Compound instproc mark-request { pageid } {
	set id [lindex [split $pageid :] end]
	if {$id == 0} {
		$self next $pageid
	}
}

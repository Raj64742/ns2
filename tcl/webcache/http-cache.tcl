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
# Implementation of web cache
#
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/webcache/http-cache.tcl,v 1.2 1998/08/20 01:30:26 haoboy Exp $

Http/Cache instproc init args {
	eval $self next $args

	$self instvar node_
	$node_ color "yellow"	;# no page
}

# It's the user's responsibility to connect clients to caches, and caches to
# servers. Note that a cache may connect to many other caches and servers, 
# but it has only one parent cache
Http/Cache instproc connect { server } {
	$self next $server
}

Http/Cache instproc disconnect { http } {
	$self instvar slist_ clist_
	if {[lsearch $slist_ $http] >= 0} {
		$self disconnect-server $http
	} else {
		$self disconnect-client $http
	}
}

# XXX Should add pending_ handling into disconnect
Http/Cache instproc disconnect-server { server } {
	$self instvar ns_ slist_ 
	set pos [lsearch $slist_ $server]
	if {$pos >= 0} {
		lreplace $slist_ $pos $pos
	} else { 
		error "Http::disconnect: not connected to $server"
	}
	set tcp [[$self get-cnc $server] agent]
	$tcp close
	#delete $tcp	;# Should we do a destroy here?
	$self cmd disconnect $server
	$server disconnect $self
}

Http/Cache instproc disconnect-client { client } {
	$self instvar ns_ clist_ 
	set pos [lsearch $clist_ $client]
	if {$pos >= 0} {
		lreplace $clist_ $pos $pos
	} else { 
		error "Http/Server::disconnect: not connected to $server"
	}
	set tcp [[$self get-cnc $client] agent]
	$tcp close
	#delete $tcp	;# Should we do a destroy here?
	$self cmd disconnect $client
}

# Use this function to construct a cache hierarchy
Http/Cache instproc set-parent { server } {
	$self instvar parent_
	set parent_ $server
}


# Copied from Http/Server
# Let the client side to do the actual connection ($ns connect)
Http/Cache instproc alloc-connection { client fid } {
	Http instvar TRANSPORT_
	$self instvar ns_ clist_ node_ id_ fid_

	lappend clist_ $client
	set snk [new Agent/TCP/$TRANSPORT_]
	$snk set fid_ $fid
	$ns_ attach-agent $node_ $snk
	set wrapper [new Application/TcpApp $snk]
	$self cmd connect $client $wrapper
	#puts "Cache $id_ connected to client [$client id]"
	return $wrapper
}

# Parameters different from Http/Client::send-request. This one needs 
# size of the request because it may need to forward a client's request to 
# a server.
Http/Cache instproc send-request { server type pageid size args } {
	$self instvar ns_ pending_	;# pending requests, includes those 
					;# from itself
	$self instvar id_
	set pending_($pageid) [$ns_ now]
	$self send $server $size \
	    "$server get-request $self $type $pageid size $size [join $args]"
}

# By constructing page id as tuple (server name, page id) we build in 
# support for multiple web servers
Http/Cache instproc get-request { cl type pageid args } {
	$self instvar slist_ clist_ ns_ id_ pending_

	array set data $args
	if ![info exists data(size)] {
		error "Http/Cache $id_: client [$cl id] must include request size in its request"
	}

	if [$self exist-page $pageid] {
		$self cache-hit $cl $type $pageid 
	} else {
		$self cache-miss $cl $type $pageid
	}
}

# Cache miss, get it from the server
Http/Cache instproc cache-miss { cl type pageid } {
	$self instvar parent_ pending_ \
		creq_ ;# pending client requests

	# Another client requests for the page
	lappend creq_($pageid) $cl/$type

	# XXX If there's a previous requests going on we won't send another
	# request for the same page.
	if [info exists pending_($pageid)] {
		return
	}

	# Page not found, contact parent and get the page. If parent_ == 0,
	# which means this is a root cache, directly contact the server
	set server [lindex [split $pageid :] 0]
	if [info exists parent_] {
		set server $parent_
	}

	set size [$self get-reqsize]
	$self evTrace E MISS p $pageid c [$cl id] s [$server id] z $size
	$self send-request $server $type $pageid $size
}

# Check if page $pageid is consistent. If not, refetch the page from server.
Http/Cache instproc is-consistent { cl type pageid } {
	return 1
}

Http/Cache instproc refetch-pending { cl type pageid } {
	return 0
}

Http/Cache instproc refetch args {
	# Do nothing
}

Http/Cache instproc cache-hit { cl type pageid } {
	# page found in cache, return it to client
	if ![$self is-consistent $cl $type $pageid] {
		# Page expired and is being refetched, waiting...
		if ![$self refetch-pending $cl $type $pageid] {
			$self refetch $cl $type $pageid
		}
		return
	}
	# Page valid, send it back to client
	set pageinfo [$self get-page $pageid]
	set size [$self get-size $pageid]
	$self send $cl $size "$cl get-response-$type $self $pageid $pageinfo"

	set server [lindex [split $pageid :] 0]
	$self evTrace E HIT p $pageid c [$cl id] s [$server id] z $size
}

# A response may come from: 
# (1) a missed client request,
Http/Cache instproc get-response-GET { server pageid args } {
	array set data $args

	if ![$self exist-page $pageid] {
		# Cache the page if it's not in the pool
		eval $self enter-page $pageid $args
		$self evTrace E ENT p $pageid m $data(modtime) \
			z $data(size) s [$server id]
	} else {
		$self instvar id_
		error "Cache $id_ has requested a page which it already has."
	}
	eval $self answer-pending-requests $pageid $args
	
	$self instvar node_
	$node_ color "blue"	;# valid page
}

Http/Cache instproc answer-pending-requests { pageid args } {
	$self instvar creq_ pending_

	array set data $args
	if [info exists creq_($pageid)] {
		# Forward the new page to every client that has requested it
		foreach clt $creq_($pageid) {
			set tmp [split $clt /]
			set cl [lindex $tmp 0]
			set type [lindex $tmp 1]
			$self answer-request-$type $cl $pageid
		}
		unset creq_($pageid)
		unset pending_($pageid)
	} else {
		unset pending_($pageid)
	}
}

Http/Cache instproc answer-request-GET { cl pageid } {
	# In response to a GET, we should always return
	# our copy of the page.
	set pginfo [$self get-page $pageid]
	set size [$self get-size $pageid]
	$self send $cl $size \
		"$cl get-response-GET $self $pageid $pginfo"
	$self evTrace E SND c [$cl id] p $pageid z $size
}


#----------------------------------------------------------------------
# Cache with consistency protocol based on TTL
#----------------------------------------------------------------------
Class Http/Cache/TTL -superclass Http/Cache

Http/Cache/TTL set updateThreshold_ 0.1

Http/Cache/TTL instproc init args {
	eval $self next $args
	$self instvar thresh_
	# default value
	set thresh_ [Http/Cache/TTL set updateThreshold_]
}

Http/Cache/TTL instproc set-thresh { th } {
	$self instvar thresh_
	set thresh_ $th
}

# XXX we should store modtime of IMS requests somewhere. Then we can check 
# if that modtime matches this cache's newest modtime when it gets an IMS
# response back from the server
Http/Cache/TTL instproc answer-request-IMS { client pageid } {
	set mt [$self get-modtime $pageid]

	if ![$client exist-page $pageid] {
		error "client [$client id] IMS a page which it doesn't have"
	}
	if {$mt < [$client get-modtime $pageid]} {
		error "client [$client id] IMS a newer page"
	}

	if {$mt > [$client get-modtime $pageid]} {
		# We should send back the new page, even if we got a 
		# "not-modified-since"
		set pginfo [$self get-page $pageid]
		set size [$self get-size $pageid]
	} else {
		set size [$self get-invsize]
		set pginfo "size $size modtime $mt time [$self get-cachetime $pageid]"
	}
	$self evTrace E SND c [$client id] t IMS z $size
	$self send $client $size \
		"$client get-response-IMS $self $pageid $pginfo"
}

Http/Cache/TTL instproc get-response-IMS { server pageid args } {
	$self instvar ns_

	# Alex cache
	# Invalidate when:(CurTime-LastCheckTime) > Thresh*(CurTime-CreateTime)
	array set data $args
	if {$data(modtime) > [$self get-modtime $pageid]} {
		# Newer page, cache it
		#if {$data(size) != 1024} {
		#	puts "WRONG!"
		#	debug 1
		#}
		eval $self enter-page $pageid $args
		$self evTrace E ENT p $pageid m [$self get-modtime $pageid] \
		    z [$self get-size $pageid] s [$server id]
		# XXX Set cache entry time to server's entry time so that
		# we would have the same expiration time
		$self set-cachetime $pageid $data(time)
	} else {
		# Update entry last validation time
		$self set-cachetime $pageid [$ns_ now]
	}
	eval $self answer-pending-requests $pageid $args
}


Http/Cache/TTL instproc is-expired { pageid } {
	$self instvar thresh_ ns_
	set cktime [expr [$ns_ now] - [$self get-cachetime $pageid]]
	set age [expr ([$ns_ now] - [$self get-modtime $pageid]) * $thresh_]
	if {$cktime < $age} {
		# Not expired
		return 0
	}
	return 1
}

Http/Cache/TTL instproc is-consistent { cl type pageid } { 
	return ![$self is-expired $pageid]
}

Http/Cache/TTL instproc refetch-pending { cl type pageid } {
	# Page expired, validate it
	$self instvar creq_ 
	if [info exists creq_($pageid)] {
		if [regexp $cl:* $creq_($pageid)] {
			# This page already requestsed by this client
			return 1
		}
		# This page is already requested by other clients. Add 
		# the new client to the requester list, do not request it again
		lappend creq_($pageid) $cl/$type
		return 1
	}
	# Set up a refetch pending state
	lappend creq_($pageid) $cl/$type
	return 0
}

Http/Cache/TTL instproc refetch { cl type pageid } {
	$self instvar parent_

	# Send an If-Modified-Since
	set server [lindex [split $pageid :] 0]
	set size [$self get-imssize]
	if [info exists parent_] {
		set server $parent_
	}
	$self evTrace E IMS p $pageid c [$cl id] s [$server id] z $size \
		t [$self get-cachetime $pageid]
	$self send-request $server IMS $pageid $size \
			modtime [$self get-modtime $pageid]
	return 0
}


# Old style TTL, using a single fixed threshold
Class Http/Cache/TTL/Old -superclass Http/Cache/TTL

Http/Cache/TTL/Old set updateThreshold_ 100

Http/Cache/TTL/Old instproc init { args } {
	eval $self next $args
	$self instvar thresh_
	set thresh_ [[$self info class] set updateThreshold_]
}

Http/Cache/TTL/Old instproc is-expired { pageid } {
	$self instvar ns_ thresh_
	set cktime [expr [$ns_ now] - [$self get-cachetime $pageid]]
	if {$cktime < $thresh_} {
		return 0
	}
	return 1
}


Class Http/Cache/TTL/Omniscient -superclass Http/Cache/TTL

# Assume every cache has exact knowledge of when a page will change
Http/Cache/TTL/Omniscient instproc is-expired { pageid } {
	$self instvar ns_ 

	set nmt [expr [$self get-modtime $pageid] + [$self get-age $pageid]]
	if {[$ns_ now] >= $nmt} {
		return 1
	} 
	return 0
}


#----------------------------------------------------------------------
# Http cache with invalidation -- Base Class
#----------------------------------------------------------------------

Http/Cache/Inval instproc mark-invalid {} {
	$self instvar node_
	$node_ color "red"
}

Http/Cache/Inval instproc mark-valid {} {
	$self instvar node_ 
	$node_ color "blue"
}

Http/Cache/Inval instproc answer-request-REF { cl pageid } {
	# Send my new page back
	set pginfo [$self get-page $pageid]
	set size [$self get-size $pageid]
	$self evTrace E SND c [$cl id] t REF p $pageid z $size
	$self send $cl $size \
		"$cl get-response-REF $self $pageid $pginfo"
}

# Only get the new page cached, do nothing else
Http/Cache/Inval instproc get-response-REF { server pageid args } {
	$self instvar creq_ id_ 

	array set data $args

	if {[$self get-modtime $pageid] > $data(modtime)} {
		error "Cache $self ($id_) refetched an old page $pageid ($data(modtime), new time [$self get-modtime $pageid]) from [$server id]"
	}

	# The page is re-validated by replacing the old entry
	eval $self enter-page $pageid $args
	$self evTrace E UPD p $pageid m [$self get-modtime $pageid] \
			z [$self get-size $pageid] s [$server id]
	eval $self answer-pending-requests $pageid $args

	$self instvar node_ marks_ ns_
	set mk [lindex $marks_($pageid) 0]
	$node_ delete-mark $mk
	set marks_($pageid) [lreplace $marks_($pageid) 0 0]
	$node_ color "blue"
}

# Always consistent?
Http/Cache/Inval instproc is-consistent { cl type pageid } {
	return [$self is-valid $pageid]
}

Http/Cache/Inval instproc refetch-pending { cl type pageid } {
	# Invalid page, prepare a refetch. 
	$self instvar creq_ 
	if [info exists creq_($pageid)] {
		if [regexp $cl:* $creq_($pageid)] {
			# This page already requestsed by this client
			return 1
		}
		# This page already requested by other clients, add ourselves
		# to the returning list and return
		lappend creq_($pageid) $cl/$type
		return 1
	}
	# Setup a refetch pending state
	lappend creq_($pageid) $cl/$type
	return 0
}

# Send a refetch. Forward the request to our parent
Http/Cache/Inval instproc refetch { cl type pageid } {
	$self instvar parent_

	set size [$self get-refsize]
	set server [lindex [split $pageid :] 0]

	if [info exists parent_] {
		set par $parent_
	} else {
		# We are the root cache (TLC), directly contact the 
		# web server
		set par $server
	}

	$self evTrace E REF p $pageid s [$server id] z $size
	$self send-request $par REF $pageid $size

	$self instvar node_ marks_ ns_
	lappend marks_($pageid) $pageid:[$ns_ now]
	$node_ add-mark $pageid:[$ns_ now] "brown"
}


#----------------------------------------------------------------------
# Invalidation cache with multicast heartbeat invalidation
#----------------------------------------------------------------------

Http/Cache/Inval/Mcast instproc init args {
	eval $self next $args
	$self add-to-map
}

# When we enter a new page into cache, we'll have to register the server
# in case we haven't know anything about it. The right place to do it 
# is in get-response-GET, because a cache will only enter a new page 
# after a cache miss, where it issues a GET.
Http/Cache/Inval/Mcast instproc get-response-GET { server pageid args } {
	eval $self next $server $pageid $args

	# XXX Assume once server-neighbor cache relationship is fixed, they
	# never change.
	set sid [[lindex [split $pageid :] 0] id]
	set cid [$server id]
	$self register-server $cid $sid
}

Http/Cache/Inval/Mcast instproc set-parent { parent } {
	$self next $parent
	# Establish a cache entry in state table
	$self cmd set-parent $parent
}

# I'm a listener (child)
Http/Cache/Inval/Mcast instproc join-inval-group { group } {
	$self instvar invalListener_ invListenGroup_ ns_ node_

	if [info exists invalListener_] {
		return
	}
	set invalListener_ [new Agent/HttpInval]
	set invListenGroup_ $group
	$invalListener_ set dst_ $group

	$self add-inval-listener $invalListener_
	$ns_ attach-agent $node_ $invalListener_

	# XXX assuming simulator already started
	$node_ join-group $invalListener_ $group
}

# I'm a sender (parent)
Http/Cache/Inval/Mcast instproc init-inval-group { group } {
	$self instvar invalSender_ invSndGroup_ ns_ node_
	if [info exists invalSender_] {
		return
	}
	set invalSender_ [new Agent/HttpInval]
	set invSndGroup_ $group
	$invalSender_ set dst_ $group

	$self add-inval-sender $invalSender_
	$ns_ attach-agent $node_ $invalSender_
	$node_ join-group $invalSender_ $group

	# XXX We should put this somewhere else... But where???
	$self start-hbtimer
}

# Another "breakdown" version of parent-cache() is in cache-miss()
Http/Cache/Inval/Mcast instproc parent-cache { server } {
	$self instvar parent_
	
	set par [$self cmd parent-cache [$server id]]
	if {$par == ""} {
		# (par == "") means parent cache in the virtual distribution
		# tree is the default, which is parent_
		if [info exists parent_] {
			set par $parent_
		} else {
			# We are the root cache (TLC), directly contact the 
			# web server
			set par $server
		}
	}
	return $par
}

# Send a refetch.
# 
# We should ask our parent in the virtual distribution tree 
# of the corresponding web server, instead of our parent in the 
# cache hierarchy.
Http/Cache/Inval/Mcast instproc refetch { cl type pageid } {
	set size [$self get-refsize]
	set server [lindex [split $pageid :] 0]
	set par [$self parent-cache $server]

	$self evTrace E REF p $pageid s [$server id] z $size
	$self send-request $par REF $pageid $size

	$self instvar node_ marks_ ns_
	lappend marks_($pageid) $pageid:[$ns_ now]
	$node_ add-mark $pageid:[$ns_ now] "brown"
}

# Cache miss, get it from our parent cache in the virtual distribution
# tree of the web server
Http/Cache/Inval/Mcast instproc cache-miss { cl type pageid } {
	$self instvar parent_ pending_ creq_ ;# pending client requests

	lappend creq_($pageid) $cl/$type

	# XXX If there's a previous requests going on we won't send another
	# request for the same page.
	if [info exists pending_($pageid)] {
		return
	}

	# Page not found, contact parent and get the page.
	set size [$self get-reqsize]
	set server [lindex [split $pageid :] 0]
	$self evTrace E MISS p $pageid c [$cl id] s [$server id] z $size

	# We directly query the server map without using TCL's version
	# of parent-cache() to mask details...
	set par [$self cmd parent-cache [$server id]]
	if {$par == ""} {
		if [info exists parent_] {
			# Use default server map, i.e., parent cache
			set par $parent_
		} else {
			# This is a TLC, and the request is for another server
			# in another hierarchy (because we don't have it in our
			# server map, nor do we have a parent cache). Now we 
			# need to find out what's the corresponding TLC of 
			# the web server so as to setup invalidation path.
			#
			# Send a direct request to server to ask about TLC
			$self instvar ns_ id_
			#puts "[$ns_ now]: $id_ send TLC"
			$self send-request $server TLC $pageid $size
			# We'll send another request to the TLC after we get 
			# its addr
			return
		}
	}
	$self send-request $par $type $pageid $size
}

# This allows a server passes invalidation to any cache via unicast
# XXX Whenever a node only wants to do an invalidation, call "cmd recv-inv"
Http/Cache/Inval/Mcast instproc invalidate { pageid modtime } {
	if [$self recv-inv $pageid $modtime] {
		# Unicast invalidation to parent.
		$self instvar parent_ 
		if ![info exists parent_] {
			# This must be a root cache, should we do anything? 
			return
		}
		set size [$self get-invsize]
		$self evTrace E SND t INV c [$parent_ id] p $pageid z $size
		
		# Mark invalidation packet as another fid
		set agent [[$self get-cnc $parent_] agent]
		set fid [$agent set fid_]
		$agent set fid_ [Http set PINV_FID_]
		$self send $parent_ $size \
				"$parent_ invalidate $pageid $modtime"
		$agent set fid_ $fid
	}
}

Http/Cache/Inval/Mcast instproc get-request { cl type pageid args } {
	eval $self next $cl $type $pageid $args
	if {(($type == "GET") || ($type == "REF")) && \
		[$self exist-page $pageid]} {
		$self count-request $pageid
		if [$self is-unread $pageid] {
			$self send-req-notify $pageid
			$self set-read $pageid
		}
	}
}

# Do the same thing as if getting a request
Http/Cache/Inval/Mcast instproc get-req-notify { pageid } {
	$self count-request $pageid
	if [$self is-unread $pageid] {
		# Continue to forward the request only if our page is 
		# also unread
		$self set-read $pageid
		$self send-req-notify $pageid
	}
}

# Request notification goes along a single path in the virtual distribution
# tree towards the web server. It's not multicast to anybody else
Http/Cache/Inval/Mcast instproc send-req-notify { pageid } {
	set server [lindex [split $pageid :] 0]
	set par [$self parent-cache $server]
	$self send $par [$self get-ntfsize] "$par get-req-notify $pageid"
}

# (1) setup an invalidation record is set to invalidate my children; 
# (2) Unicast the new page to my parent;
# (3) Update my own page records
# (4) Setting up a repair group to send out the new page (once and for all)
Http/Cache/Inval/Mcast instproc push-update { pageid args } {
	# Update page, possibly push the page to children
	if [eval $self recv-push $pageid $args] {
		# XXX We should probably check if we have pending request for 
		# this page. If so, we should use this pushed page to answer 
		# those pending requests, and then mark this page as read.

		# unicast push to parent
		$self instvar parent_ 
		if [info exists parent_] {
			# If we are root, don't forward the data packet to
			# anybody. Otherwise unicast the new page to my parent
			set pginfo [$self get-page $pageid]
			set size [$self get-size $pageid]
			$self evTrace E UPD c [$parent_ id] p $pageid z $size
			$self send $parent_ $size \
				"$parent_ push-update $pageid $pginfo"
		}
		$self push-children $pageid
	}
}

Http/Cache/Inval/Mcast instproc init-update-group { group } {
	$self instvar ns_ node_ updSender_ updSendGroup_

	# Allow a cache to have multiple update groups. 
	set snd [new Agent/HttpInval]
	$snd set dst_ $group
	$self add-upd-sender $snd
	$ns_ attach-agent $node_ $snd
	$node_ join-group $snd $group
}

Http/Cache/Inval/Mcast instproc join-update-group { group }  {
	$self instvar updListener_ updListenGroup_ ns_ node_

	set updListenGroup_ $group
	# One cache can only receive from one update group at a time
	if ![info exists updListener_] {
		set updListener_ [new Agent/HttpInval]
		$self add-upd-listener $updListener_
		$updListener_ set dst_ $updListenGroup_
		$ns_ attach-agent $node_ $updListener_
	}
	$node_ join-group $updListener_ $updListenGroup_
#	$node_ add-mark "Updating" "Orange"
}

Http/Cache/Inval/Mcast instproc leave-update-group {} {
	$self instvar updListener_ updListenGroup_ ns_ node_
	if ![info exists updListener_] {
		return
	}
	$node_ leave-group $updListener_ $updListenGroup_
	$node_ delete-mark "Updating"
}

# Set up a unicast heartbeat connection
Http/Cache/Inval/Mcast instproc setup-unicast-hb {} {
	Http instvar TRANSPORT_
	$self instvar node_ ns_

	set snk [new Agent/TCP/$TRANSPORT_]
	$snk set fid_ [Http set HB_FID_]
	$ns_ attach-agent $node_ $snk
	set wrapper [new Application/TcpApp/HttpInval $snk]
	$wrapper set-app $self
	return $wrapper
}

# Establish state for server. Propagate until Top-Level Cache is reached
# Set up heartbeat connection along the way
Http/Cache/Inval/Mcast instproc server-join { server cache } {
	$self cmd join [$server id] $cache

	#puts "Server [$server id] joins cache [$self id]"

	$self instvar parent_
	if ![info exists parent_] {
		return
	}

	$self send $parent_ [$self get-joinsize] \
			"$parent_ server-join $server $self"

	# Establishing heartbeat
	Http instvar TRANSPORT_
	$self instvar ns_ node_
	set tcp [new Agent/TCP/$TRANSPORT_]
	$tcp set fid_ [Http set HB_FID_]
	$ns_ attach-agent $node_ $tcp
	set dst [$parent_ setup-unicast-hb]
	set snk [$dst agent]
	$ns_ connect $tcp $snk
	$tcp set dst_ [$snk set addr_] 
	$tcp set window_ 100
	$snk listen
	set wrapper [new Application/TcpApp/HttpInval $tcp]
	$wrapper connect $dst
	$wrapper set-app $self
	$self set-pinv-agent $wrapper

	# If we haven't started it yet, start it.
	$self start-hbtimer
}

Http/Cache/Inval/Mcast instproc request-mpush { page } {
	$self instvar mpush_refresh_ ns_ hb_interval_
	if [info exists mpush_refresh_($page)] {
		# The page is already set as mandatory push, ignore it
		return
	}
	$self set-mandatory-push $page

	set server [lindex [split $page :] 0]
	set cache [$self parent-cache $server]

	set mpush_refresh_($page) [$ns_ at [expr [$ns_ now] + $hb_interval_] \
		"$self send-refresh-mpush $cache $page"]
	# Forward the push request towards the web server
	$self send $cache [$self get-mpusize] "$cache request-mpush $page"
}

Http/Cache/Inval/Mcast instproc refresh-mpush { page } {
	$self cmd set-mandatory-push $page
}

Http/Cache/Inval/Mcast instproc send-refresh-mpush { cache page } {
	$self instvar mpush_refresh_ ns_ hb_interval_
	$self send $cache [$self get-mpusize] "$cache refresh-mpush $page"
	set mpush_refresh_($page) [$ns_ at [expr [$ns_ now] + $hb_interval_] \
		"$self send-refresh-mpush $cache $page"]
}

# XXX This is used when a mpush is timed out, where we don't need to 
# send explicit teardown, etc. 
Http/Cache/Inval/Mcast instproc cancel-mpush-refresh { page } {
	$self instvar mpush_refresh_ ns_ 
	if [info exists mpush_refresh_($page)] {
		$ns_ cancel $mpush_refresh_($page)
		puts "[$ns_ now]: Cache [$self id] stops mpush"
	} else {
		error "Cache [$self id]: No mpush to stop!"
	}
}

Http/Cache/Inval/Mcast instproc stop-mpush { page } {
	# Cancel refresh messages
	$self cancel-mpush-refresh $page

	# Clear page push status
	$self cmd stop-mpush $page

	# Send explicit message to stop mpush
	set server [lindex [split $page :] 0]
	set cache [$self parent-cache $server]
	$self send $cache [$self get-mpusize] "$cache stop-mpush $page"
}

#
# Support for multiple hierarchies
#
# Top-Level Caches (TLCs) need to exchange invalidations with each other,
# so they are both sender and receiver in this multicast group. 
Http/Cache/Inval/Mcast instproc join-tlc-group { group } {
	$self instvar tlcAgent_ tlcGroup_ ns_ node_

	if [info exists tlcAgent_] {
		return 
	}
	set tlcAgent_ [new Agent/HttpInval]
	set tlcGroup_ $group
	$tlcAgent_ set dst_ $group

	$self add-inval-sender $tlcAgent_
	$self add-inval-listener $tlcAgent_
	$ns_ attach-agent $node_ $tlcAgent_
	$node_ join-group $tlcAgent_ $group
}

Http/Cache/Inval/Mcast instproc get-response-TLC { server pageid tlc } {
	# Continue query...
	$self register-server [$tlc id] [$server id]
	$self instvar ns_ id_
#	puts "[$ns_ now]: Cache $id_ knows server [$server id] -> tlc [$tlc id]"
	$self send-request $tlc GET $pageid [$self get-reqsize]
}


#----------------------------------------------------------------------
# Http/Cache/Inval/Mcast/Perc
# 
# Multicast invalidation + two way liveness message + invalidation 
# filtering. Must be used with Http/Server/Inval/Ucast/Perc
# 
# Requires C++ support. This is why we have this long name. :( 
#
# Procedures: 
# - Server's new page: the server injects it into the cache hierarchy by
#   sending it to its parent cache, which in turn forwards it up the tree.
# - Every cache keeps a cost for each cached page. 
#----------------------------------------------------------------------

# Because we are doing direct request, we'll get a lot of responses 
# directly from the server, and we'll have cid == sid. We don't want to
# register this into our server map, because the server map is used 
# for forwarding pro formas. Therefore, we wrap up register-server to
# direct requests to all our *UNKNOWN* servers to our parent. 
#
# Note this won't disrupt server entries via JOIN, because they are 
# established before any request is sent.
Http/Cache/Inval/Mcast/Perc instproc register-server {cid sid} {
	$self instvar parent_ direct_request_
	if {$direct_request_ && [info exists parent_]} {
		$self cmd register-server [$parent_ id] $sid
	} 
}

# Allows direct request
Http/Cache/Inval/Mcast/Perc instproc cache-miss { cl type pageid } {
	$self instvar direct_request_

	if !$direct_request_ {
		# If not use direct request, fall back to previous method
		$self next $cl $type $pageid
		return
	}

	# If use direct request, send a request to the web server to ask 
	# for the page, and then send a pro forma when get the request 

	$self instvar parent_ pending_ creq_ ;# pending client requests
	$self instvar dreq_ ;# pending direct requests
		
	lappend creq_($pageid) $cl/$type

	# XXX If there's a previous requests going on we won't send another
	# request for the same page.
	if [info exists pending_($pageid)] {
		return
	}

	$self instvar dreq_
	set dreq_($pageid) 1

	# Page not found, directly contact the server and get the page. 
	set server [lindex [split $pageid :] 0]
	set size [$self get-reqsize]
	$self evTrace E MISS p $pageid c [$cl id] s [$server id] z $size
	$self send-request $server $type $pageid $size
}

# Allows direct request
Http/Cache/Inval/Mcast/Perc instproc refetch { cl type pageid } {
	$self instvar direct_request_

	if !$direct_request_ {
		$self next $cl $type $pageid
		return
	}

	$self instvar dreq_
	set dreq_($pageid) 1

	set size [$self get-refsize]
	set server [lindex [split $pageid :] 0]
	$self evTrace E REF p $pageid s [$server id] z $size
	$self send-request $server REF $pageid $size

	$self instvar node_ marks_ ns_
	lappend marks_($pageid) $pageid:[$ns_ now]
	$node_ add-mark $pageid:[$ns_ now] "brown"
}

# Whenever get a request, send a pro forma up
Http/Cache/Inval/Mcast/Perc instproc get-response-GET { server pageid args } {
	# First, answer children's requests, etc.
	eval $self next $server $pageid $args

	# Then send a pro forma if it's a direct request
	$self instvar dreq_ 
	if [info exists dreq_($pageid)] {
		# If this page is result of a direct request, send a pro forma
		eval $self send-proforma $pageid $args
		unset dreq_($pageid)
	}
}

# Same treatment as get-response-GET
Http/Cache/Inval/Mcast/Perc instproc get-response-REF { server pageid args } {
	eval $self next $server $pageid $args
	$self instvar dreq_
	if [info exists dreq_($pageid)] {
		eval $self send-proforma $pageid $args
		unset dreq_($pageid)
	}
}

# XXX We need special handling for multiple hierarchies. If we cannot find 
# the server in our server map, we directly call the server's routine to 
# find out its TLC. This doesn't make the simulation artificial, though, 
# because in our previous direct response from the server, we could have
# easily gotten its TLC. 
Http/Cache/Inval/Mcast/Perc instproc send-proforma { pageid args } {
	set server [lindex [split $pageid :] 0]
	set par [$self parent-cache $server]
	if {$par == $server} {
		# If we are the primary cache, don't send anything
		return
	} elseif {$par == ""} {
		# XXX 
		# We are the TLC, and we don't have a server entry. This 
		# means that the server resides in another hierarchy. 
		# Query the global server-to-TLC map to unicast this 
		# pro forma to that TLC...
		set par [$server get-tlc]
		puts "TLC [$self id] learned about server [$server id] by pro forma"
	}
	$self send $par [$self get-pfsize] \
		"$par recv-proforma $self $pageid [join $args]"
	$self evTrace E SPF p $pageid c [$par id]
}

Http/Cache/Inval/Mcast/Perc instproc get-response-IMS { server pageid args } {
	$self instvar ns_ 

	array set data $args
	if {$data(modtime) <= [$self get-modtime $pageid]} {
		# The page we got from the pro forma is indeed most up-to-date
		return
	}
	# The server has changed the page since the pro forma is sent
	# We need to send invalidations to invalidate the page
	$self invalidate $pageid 
	eval $self enter-page $pageid $args
	$self mark-valid
}

Http/Cache/Inval/Mcast/Perc instproc mark-valid-hdr {} {
	$self instvar node_
	$node_ color "orange"
}

Http/Cache/Inval/Mcast/Perc instproc recv-proforma { cache pageid args } {
	$self evTrace E RPF p $pageid c [$cache id]

	array set data $args
	if ![$self exist-page $pageid] {
		# Page doesn't exists. Create an entry for page header, and
		# forward it towards the web server
		eval $self enter-metadata $pageid $args
		$self mark-valid-hdr

		set server [lindex [split $pageid :] 0]
		set par [$self parent-cache $server]
		if {$par == $server} {
			# If we are the primary cache, validate this 
			# pro forma by sending an IMS
			$self send-request $par IMS $pageid \
				[$self get-imssize] modtime $data(modtime)
		} else {
			eval $self send-proforma $pageid $args
		}
	} elseif [$self is-valid $pageid] {
		# Valid page, check if this is a newer one
		set mt [$self get-modtime $pageid]
		if {$data(modtime) < $mt} {
			# If the pro forma is older, should invalidate our
			# children so that they'll invalidate their stuff
			$self recv-inv $pageid $data(modtime)
			return
		} elseif {$data(modtime) > $mt} {
			# If the pro forma is about a newer page, 
			# first invalidate our page, so that we have an 
			# invalidation record to let our children know the 
			# page is invalid. Then enter the page metadata.
			# 
			# XXX Should check for existence of page content
			$self recv-inv $pageid $data(modtime)
			eval $self enter-metadata $pageid $args
			$self mark-valid-hdr
			eval $self send-proforma $pageid $args
		}
		# Drop the pro forma if it's the same as our page.
		# XXX count the pro forma as a request to this page, and
		# send a request notification towards the web server. 
		# Mark the page as read if it's originally unread.
		$self count-request $pageid
		if [$self is-unread $pageid] {
			$self set-read $pageid
		}
	} else {
		# Invalid page, check if we should set a valid page header
		# so that invalidations will be forwarded.
		array set data $args
		set mt [$self get-modtime $pageid]
		if {$data(modtime) < $mt} {
			# We already have the most up-to-date page, so are
			# our parents. Do nothing
			return
		} 
		# The pro forma is newer, put in the new meta-data and 
		# set the page as valid_header but not valid_page
		# Note if a page is invalid, its modtime is that of the 
		# newest page.
		# 
		# XXX Should test for the existence of page content by 
		# looking at the size of the pro forma.
		eval $self enter-metadata $pageid $args
		$self mark-valid-hdr

		eval $self send-proforma $pageid $args
	}
}

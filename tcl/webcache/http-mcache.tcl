# 
#  Copyright (c) 1997 by the University of Southern California
#  All rights reserved.
# 
#  Permission to use, copy, modify, and distribute this software and its
#  documentation in source and binary forms for non-commercial purposes
#  and without fee is hereby granted, provided that the above copyright
#  notice appear in all copies and that both the copyright notice and
#  this permission notice appear in supporting documentation. and that
#  any documentation, advertising materials, and other materials related
#  to such distribution and use acknowledge that the software was
#  developed by the University of Southern California, Information
#  Sciences Institute.  The name of the University may not be used to
#  endorse or promote products derived from this software without
#  specific prior written permission.
# 
#  THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
#  the suitability of this software for any purpose.  THIS SOFTWARE IS
#  PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
#  INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 
#  Other copyrights might apply to parts of this software and are so
#  noted when applicable.
# 
# Implementation of web cache, client and server which support 
# multimedia objects.
#
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/webcache/http-mcache.tcl,v 1.2 1999/07/02 17:07:14 haoboy Exp $

#
# Multimedia web client
#

# Override this method to change the default max size of the page pool
Http/Client/Media instproc create-pagepool {} {
	set pool [new PagePool/Client/Media]
	$self set-pagepool $pool
	return $pool
}

# Connection procedure:
# (1) Client ---(HTTP_REQUEST)--> Server
# (2) Client <--(HTTP_RESPONSE)-- Server
# (3) Client <----(RAP_DATA)----  Server
Http/Client/Media instproc get-response-GET { server pageid args } {
	eval $self next $server $pageid $args
	
	# XXX Enter the page into page pool, so that we can keep track of 
	# which segment has been received and know where to stop receiving. 
	if [$self exist-page $pageid] {
		error "Http/Client/Media: receives an \"active\" page!"
	}
	eval $self enter-page $pageid $args

	array set data $args
	if {[info exists data(pgtype)] && ($data(pgtype) == "MEDIA")} {
		# Create a multimedia connection to server
		$self media-connect $server $pageid
	}
}

# Don't accept an request if there is one stream still on transit
Http/Client/Media instproc send-request { server type pageid args } {
	$self instvar mmapp_ 
	if [info exists mmapp_($pageid)] {
		return
	}
	eval $self next $server $type $pageid $args
}

# Create a RAP connection to a server
Http/Client/Media instproc media-connect { server pageid } {

	# DEBUG ONLY
	#puts "Client [$self id] media-connect [$server id] $pageid"

	$self instvar mmapp_ ns_ node_ 
	Http instvar MEDIA_TRANSPORT_ MEDIA_APP_
	if [info exists mmapp_($pageid)] {
		error "Media client [$self id] got a request for an existing \
stream"
	}
	set agent [new Agent/$MEDIA_TRANSPORT_]
	$ns_ attach-agent $node_ $agent
	set app [new Application/$MEDIA_APP_ $pageid]
	$app attach-agent $agent
	$app target $self
	$server alloc-mcon $self $pageid $agent
	set mmapp_($pageid) $app
	$app set-layer [$self get-layer $pageid]
}

Http/Client/Media instproc media-disconnect { server pageid } {
	$self instvar mmapp_ ns_ node_

	if {![info exists mmapp_($pageid)]} {
		error "Media client [$self id] disconnect: not connected to \
server [$server id] with page $pageid"
	}
	set app $mmapp_($pageid)
	set agent [$app agent]
	$ns_ detach-agent $node_ $agent

	# Disconnect the agent and app at the server side
	$server media-disconnect $self $pageid

	# DEBUG ONLY
#	puts "Client [$self id] disconnect from server [$server id]"

	delete $agent
	delete $app
	unset mmapp_($pageid)

	$self stream-received $pageid
}


#
# Multimedia web server
#

# Create media pages instead of normal pages
Http/Server/Media instproc gen-page { pageid } {
	$self instvar pgtr_ 
	set pginfo [$self next $pageid]
	if [$pgtr_ is-media-page $pageid] {
		return [lappend pginfo pgtype MEDIA]
	} else {
		return $pginfo
	}
}

Http/Server/Media instproc create-pagepool {} {
	set pool [new PagePool/Client/Media]
	$self set-pagepool $pool
	# Set the pool size to "infinity" (2^31-1)
	$pool set max_size_ 2147483647
	return $pool
}

# Allocate a media connection
Http/Server/Media instproc alloc-mcon { client pageid dst_agent } {
	$self instvar ns_ node_ mmapp_ 
	Http instvar MEDIA_TRANSPORT_ MEDIA_APP_

	set agent [new Agent/$MEDIA_TRANSPORT_]
	$ns_ attach-agent $node_ $agent
	set app [new Application/$MEDIA_APP_ $pageid]
	$app attach-agent $agent
	$app target $self 
	set mmapp_($client/$pageid) $app
	# Set layers
	$app set-layer [$self get-layer $pageid]

	# Associate $app with $client and $pageid
	$self register-client $app $client $pageid

	# DEBUG ONLY
	# Logging MediaApps, only do it for sender-side media apps
#	set lf [$self log]
#	if {$lf != ""} {
#		$app log $lf
#	}
	#puts "Server [$self id] allocated a connection to client [$client id]\
# using agent $agent"

	# Connect two RAP agents and start data transmission
	$ns_ connect $agent $dst_agent
	$agent start
}

Http/Server/Media instproc media-disconnect { client pageid } { 
	$self instvar mmapp_ ns_ node_

	# DEBUG ONLY
#	puts "At [$ns_ now] Server [$self id] disconnected from \
#			client [$client id]"

	if {![info exists mmapp_($client/$pageid)]} {
		error "Media server [$self id] disconnect: not connected to \
client [$client id] with page $pageid"
	}
	set app $mmapp_($client/$pageid)
	set agent [$app agent]
	$ns_ detach-agent $node_ $agent

	$self unregister-client $app $client $pageid

	# DEBUG ONLY 
	#puts "Server [$self id] deleting agent $agent"

	delete $agent
	delete $app
	unset mmapp_($client/$pageid)
}

# Tell the client that a stream has been completed. Assume that there is 
# an open connection between server and client
Http/Server/Media instproc finish-stream { app } {
	$self instvar mmapp_ 
	foreach n [array names mmapp_] {
		if {$mmapp_($n) == $app} {
			set tmp [split $n /]
			set client [lindex $tmp 0]
			set pageid [lindex $tmp 1]
			$self send $client [$self get-reqsize] \
				"$client media-disconnect $self $pageid"
			return
		}
	}
}

# If the page is a media page, set the response size to that of the request
Http/Server/Media instproc handle-request-GET { pageid args } {
	set pginfo [eval $self next $pageid $args]
	if {[$self get-pagetype $pageid] == "MEDIA"} {
		set pginfo [lreplace $pginfo 0 0 [$self get-reqsize]]
	}
	return $pginfo
}

Http/Server/Media instproc gen-pageinfo { pageid } {
	set pginfo [eval $self next $pageid]
	# Create contents for media page
	$self instvar pgtr_
	if [$pgtr_ is-media-page $pageid] {
		return [lappend pginfo pgtype MEDIA layer \
				[$pgtr_ get-layer $pageid]]
	} else {
		return $pginfo
	}
}

# Map a media application to the HTTP application at the other end
#Http/Server/Media instproc map-media-app { app } {
#	$self instvar mmapp_
#	foreach n [array names mmapp_] {
#		if {$mmapp_($n) == $app} {
#			return [lindex [split $n /] 0]
#		}
#	}
#}

# Handle prefetching of designated segments
Http/Server/Media instproc get-request { client type pageid args } {
	if {$type == "PREFSEG"} {
		# XXX allow only one prefetching stream from any single client
		# Records this client as doing prefetching. 
		# FAKE a connection to the client without prior negotiation
		set pagenum [lindex [split $pageid :] 1]
		set layer [lindex $args 0]
		set seglist [lrange $args 1 end]
		eval $self register-prefetch $client $pagenum $layer $seglist
		$client start-prefetch $self $pageid
		$self evTrace S PREF p $pageid l $layer [join $seglist]
		# DEBUG only
		#$self instvar ns_
		#puts "At [$ns_ now] server [$self id] prefetches $args"
	} elseif {$type == "STOPPREF"} {
		set pagenum [lindex [split $pageid :] 1]
		$self stop-prefetching $client $pagenum
		$client media-disconnect $self $pageid
		# DEBUG only
		$self instvar ns_
#puts "At [$ns_ now] server [$self id] stops prefetching $pageid"
	} else {
		eval $self next $client $type $pageid $args
	}
}



#
# Multimedia web cache
#

Http/Cache/Media instproc create-pagepool {} {
	set pool [new PagePool/Client/Media]
	$self set-pagepool $pool
	return $pool
}

Http/Cache/Media instproc start-prefetch { server pageid } {
	$self instvar pref_ ns_
	# DEBUG
	if [info exists pref_($server/$pageid)] {
		# We are already connected to the server
		return
	} else {
		set pref_($server/$pageid) 1
	}
#	puts "At [$ns_ now] cache [$self id] starts prefetching to [$server id]"
	# XXX Do not use QA for prefetching!!
	Http instvar MEDIA_APP_
	set oldapp $MEDIA_APP_
	# XXX Do not use the default initial RTT of RAP. We must start sending
	# as soon as possible, then drop our rate if necessary. Instead, set
	# initial RTT estimation as 100ms.
	set oldsrtt [Agent/RAP set srtt_]
	Agent/RAP set srtt_ 0.1
	set MEDIA_APP_ MediaApp
	$self media-connect $server $pageid
	set MEDIA_APP_ $oldapp
	Agent/RAP set srtt_ $oldsrtt
}

Http/Cache/Media instproc media-connect { server pageid } {
	# DEBUG ONLY
	#puts "Cache [$self id] media-connect [$server id] $pageid"

	$self instvar s_mmapp_ ns_ node_ 
	Http instvar MEDIA_TRANSPORT_ MEDIA_APP_
	if [info exists s_mmapp_($server/$pageid)] {
		error "Media client [$self id] got a request for an existing \
stream"
	}
	set agent [new Agent/$MEDIA_TRANSPORT_]
	$ns_ attach-agent $node_ $agent
	set app [new Application/$MEDIA_APP_ $pageid]
	$app attach-agent $agent
	$app target $self
	$server alloc-mcon $self $pageid $agent
	set s_mmapp_($server/$pageid) $app
	$app set-layer [$self get-layer $pageid]
}

Http/Cache/Media instproc alloc-mcon { client pageid dst_agent } {
	$self instvar ns_ node_ c_mmapp_ 
	Http instvar MEDIA_TRANSPORT_ MEDIA_APP_
	if [info exists c_mmapp_($client/$pageid)] {
		error "Media cache [$self id] got a request for an existing \
stream $pageid from client [$client id]"
	}

	set agent [new Agent/$MEDIA_TRANSPORT_]
	$ns_ attach-agent $node_ $agent
	set app [new Application/$MEDIA_APP_ $pageid]
	$app attach-agent $agent
	$app target $self 
	set c_mmapp_($client/$pageid) $app
	# Set layers
	$app set-layer [$self get-layer $pageid]

	# Associate $app with $client and $pageid
	$self register-client $app $client $pageid

	# Logging MediaApps, only do it for sender-side media apps
#	set lf [$self log]
#	if {$lf != ""} {
#		$app log $lf
#	}

	# Connect two RAP agents and start data transmission
	$ns_ connect $agent $dst_agent
	$agent start
}

Http/Cache/Media instproc media-disconnect { host pageid } {
	$self instvar c_mmapp_ s_mmapp_ ns_ node_

	if [info exists c_mmapp_($host/$pageid)] {
		# Disconnect from a client
		set app $c_mmapp_($host/$pageid)
		set agent [$app agent]
		$ns_ detach-agent $node_ $agent
		$self unregister-client $app $host $pageid
		# DEBUG ONLY
#		puts "At [$ns_ now] Cache [$self id] disconnect from \
#				client [$host id]"
		delete $agent
		delete $app
		unset c_mmapp_($host/$pageid)

		# Dump status of all pages after serving a client
		$self instvar pool_
		foreach p [$pool_ list-pages] {
			$self dump-page $p
		}

	} elseif [info exists s_mmapp_($host/$pageid)] {
		# Disconnect from a server
		set app $s_mmapp_($host/$pageid)
		set agent [$app agent]
		$ns_ detach-agent $node_ $agent
		$host media-disconnect $self $pageid

		# DEBUG ONLY
#		puts "At [$ns_ now] Cache [$self id] disconnect from \
#				server [$host id]"

		delete $agent
		delete $app
		unset s_mmapp_($host/$pageid)
	} else {
		error "At [$ns_ now] Media cache [$self id] tries to \
			disconnect from a non-connected host [$host id]"
	}

	# Stop prefetching if there's any
	$self instvar pref_ ns_
	set server [lindex [split $pageid :] 0]
	if [info exists pref_($server/$pageid)] {
		# Theoretically we should stop our prefetching 
		# RAP connection now 
		if {$host == $server} {
			# If we are disconnected from the server, close 
			# prefetch
#			puts "At [$ns_ now] cache [$self id] stops prefetching to [$server id]"
			$self evTrace E STP s [$server id] p $pageid
			unset pref_($server/$pageid)
		} else { 
			$self send $server [$self get-reqsize] \
				"$server get-request $self STOPPREF $pageid"
		}
	}
	$self stream-received $pageid
}

# Tell the client that a stream has been completed. Assume that there is 
# an open connection between server and client
Http/Cache/Media instproc finish-stream { app } {
	$self instvar c_mmapp_ s_mmapp_
	foreach n [array names c_mmapp_] {
		if {$c_mmapp_($n) == $app} {
			set tmp [split $n /]
			set client [lindex $tmp 0]
			set pageid [lindex $tmp 1]
			$self send $client [$self get-reqsize] \
				"$client media-disconnect $self $pageid"
			return
		}
	}
}

Http/Cache/Media instproc get-response-GET { server pageid args } {
	eval $self next $server $pageid $args

	# DEBUG ONLY
	#puts "Cache [$self id] gets response"

	array set data $args
	if {[info exists data(pgtype)] && ($data(pgtype) == "MEDIA")} {
		# Create a multimedia connection to server
		$self media-connect $server $pageid
	}
}

# XXX If it's a media page, change page size and send back a small response. 
# This response is intended to establish a RAP connection from client to 
# cache. 
Http/Cache/Media instproc answer-request-GET { cl pageid args } {
	array set data $args
	if {[info exists data(pgtype)] && ($data(pgtype) == "MEDIA")} {
		set size [$self get-reqsize]
	} else {
		set size $data(size)
	}
	$self send $cl $size \
		"$cl get-response-GET $self $pageid $args"
	$self evTrace E SND c [$cl id] p $pageid z $data(size)
}

# $args is a list of segments, each of which is a list of 
# two elements: {start end}
Http/Cache/Media instproc pref-segment {pageid layer args} {
	# XXX directly send a request to the SERVER. Assuming that 
	# we are not going through another cache. 
	set server [lindex [split $pageid :] 0]
	set size [$self get-reqsize]
	$self send $server $size "$server get-request $self PREFSEG \
		$pageid $layer [join $args]"
}

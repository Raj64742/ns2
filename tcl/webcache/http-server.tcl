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
# Implementation of an HTTP server
#
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/webcache/http-server.tcl,v 1.3 1998/08/25 01:08:21 haoboy Exp $

Http/Server instproc init args {
	eval $self next $args
	$self instvar node_
	$node_ color "HotPink"
}

Http/Server instproc set-page-generator { pagepool } {
	$self instvar pgtr_
	set pgtr_ $pagepool
}

# Helper functions to generate randomized page size, page last-mod time,
# and estimated page lifetime
Http/Server instproc gen-pagesize { id } {
	# XXX should put a bimodal distribution for page id here
	$self instvar pgtr_
	if [info exists pgtr_] {
		return [$pgtr_ gen-size $id]
	} else {
		# Default value, for backward compatibility
		return 2000
	}
}

Http/Server instproc gen-age { id } {
	$self instvar pgtr_ ns_

	if [info exists pgtr_] {
		set mtime [$ns_ now]
		return [expr [$pgtr_ gen-modtime $id $mtime] - $mtime]
	} else { 
		return 50
	}
}

# XXX 
# This method to calculate staleness time isn't scalable!!! We have to have
# a garbage collection method to release unused portion of modtimes_ and 
# modseq_. That's not implemented yet because it requires the server to know
# the oldest version held by all other clients.
Http/Server instproc stale-time { pageid modtime } {
	$self instvar modseq_ modtimes_ ns_
	for {set i $modseq_($pageid)} {$i >= 0} {incr i -1} {
		if {$modtimes_($pageid:$i) <= $modtime} {
			break
		}
	}
	if {$i < 0} {
		error "Non-existent modtime $modtime for page $pageid"
	}
	set ii [expr $i + 1]
	set t1 [expr abs($modtimes_($pageid:$i) - $modtime)]
	set t2 [expr abs($modtimes_($pageid:$ii) - $modtime)]
	if {$t1 > $t2} {
		incr ii
	}
	return [expr [$ns_ now] - $modtimes_($pageid:$ii)]
}

Http/Server instproc modify-page { pageid } {
	# Set Last-Modified-Time to current time
	$self instvar ns_ id_

	set id [lindex [split $pageid :] end]

	# Change modtime and lifetime only, do not change page size
	set modtime [$ns_ now]
	set size [$self gen-pagesize $id]
	set age [$self gen-age $id]	;# retrieve next modtime
	$ns_ at [expr [$ns_ now] + $age] "$self modify-page $pageid"
	$self enter-page $pageid size $size modtime $modtime age $age

#	$ns_ trace-annotate "S $id_ INV $pageid"
	$self evTrace S MOD p $pageid m [$ns_ now] 

	$self instvar modtimes_ modseq_
	incr modseq_($pageid)
	set modtimes_($pageid:$modseq_($pageid)) $modtime
}

# XXX Assumes page doesn't exists before. 
Http/Server instproc gen-page { pageid } {
	$self instvar ns_ pgtr_ 

	if [$self exist-page $pageid] {
		error "$self: shouldn't use gen-page for existing pages"
	}

	set id [lindex [split $pageid :] end]

	set modtime [$ns_ now]
	set size [$self gen-pagesize $id]
	set age [$self gen-age $id]
	$ns_ at [expr [$ns_ now] + $age] "$self modify-page $pageid"

	$self instvar modtimes_ modseq_
	set modseq_($pageid) 0
	set modtimes_($pageid:0) $modtime

	#puts "Generated page $pageid age $age"
	$self enter-page $pageid size $size modtime $modtime age $age
	return "size $size modtime $modtime age $age"
}

Http/Server instproc disconnect { client } {
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

Http/Server instproc alloc-connection { client fid } {
	Http instvar TRANSPORT_
	$self instvar ns_ clist_ node_ fid_

	lappend clist_ $client
	set snk [new Agent/TCP/$TRANSPORT_]
	$snk set fid_ $fid
	$ns_ attach-agent $node_ $snk
	set wrapper [new Application/TcpApp $snk]
	$self cmd connect $client $wrapper
	return $wrapper
}

Http/Server instproc handle-request-GET { pageid args } {
	if [$self exist-page $pageid] {
		set pageinfo [$self get-page $pageid]
	} else {
		set pageinfo [$self gen-page $pageid]
	}

	lappend res [$self get-size $pageid]
	eval lappend res $pageinfo
}

Http/Server instproc handle-request-IMS { pageid args } {
	array set data $args
	set mt [$self get-modtime $pageid]
	if {$mt <= $data(modtime)} {
		# Send a not-modified since
		set size [$self get-invsize]
		# We don't need other information for a IMS of a 
		# valid page
		set pageinfo \
		  "size $size modtime $mt time [$self get-cachetime $pageid]"
		$self evTrace S SND p $pageid m $mt z $size t IMS
	} else {
		# Page modified, send the new one
		set size [$self get-size $pageid]
		set pageinfo [$self get-page $pageid]
	}
	lappend res $size
	eval lappend res $pageinfo
	return $res
}

Http/Server instproc get-request { client type pageid args } {
	$self instvar ns_ id_

	# XXX Here maybe we want to wait for a random time to model 
	# server response delay, it could be easily added in a derived class.

	set res [eval $self handle-request-$type $pageid $args]
	set size [lindex $res 0]
	set pageinfo [lrange $res 1 end]

	$self send $client $size \
		"$client get-response-$type $self $pageid $pageinfo"
}

Http/Server instproc set-parent-cache { cache } {
	# Dummy proc
}


#----------------------------------------------------------------------
# Base Http invalidation server
#----------------------------------------------------------------------
Http/Server/Inval instproc modify-page { pageid } {
	$self next $pageid
	$self instvar ns_ id_
	$self invalidate $pageid [$ns_ now]
}

Http/Server/Inval instproc handle-request-REF { pageid args } {
	return [eval $self handle-request-GET $pageid $args]
}


#----------------------------------------------------------------------
# Old unicast invalidation Http server. For compatibility
# Server with single unicast invalidation
#----------------------------------------------------------------------
Class Http/Server/Inval/Ucast -superclass Http/Server/Inval

# We need to maintain a list of all caches who have gotten a page from this
# server.
Http/Server/Inval/Ucast instproc get-request { client type pageid args } {
        eval $self next $client $type $pageid $args

        # XXX more efficient representation?
        $self instvar cacheList_
        if [info exists cacheList_($pageid)] {
                set pos [lsearch $cacheList_($pageid) $client]
        } else {
                set pos -1
        }

        # If it's a new cache, put it there
        # XXX we should eventually have a timer for each cache entry, so 
        # we can get rid of old cache entries
        if {$pos < 0 && [regexp "Cache" [$client info class]]} {
                lappend cacheList_($pageid) $client
        }
}

Http/Server/Inval/Ucast instproc invalidate { pageid modtime } {
        $self instvar cacheList_ 

        if ![info exists cacheList_($pageid)] {
                return
        }
        foreach c $cacheList_($pageid) {
                # Send invalidation to every cache, assuming a connection 
                # exists between the server and the cache
                set size [$self get-invsize]

		# Mark invalidation packet as another fid
		set agent [[$self get-cnc $c] agent]
		set fid [$agent set fid_]
		$agent_ set fid_ [Http set PINV_FID_]
                $self send $c $size \
                        "$c invalidate $pageid $modtime"
		$agent_ set fid_ $fid
                $self evTrace S INV p $pageid m $modtime z $size
        }
}


#----------------------------------------------------------------------
# (Y)et another (U)ni(C)ast invalidation server
#
# It has a single parent cache. Whenever a page is updated in this server
# it informs the parent cache, which will in turn propagate the update
# (or invalidation) to the whole cache hierarchy.
#----------------------------------------------------------------------
Http/Server/Inval/Yuc instproc set-tlc { tlc } {
	$self instvar tlc_
	set tlc_ $tlc
}

Http/Server/Inval/Yuc instproc get-tlc { tlc } {
	$self instvar tlc_
	return $tlc_
}

Http/Server/Inval/Yuc instproc next-hb {} {
	Http/Server/Inval/Yuc instvar hb_interval_ 
	return [expr $hb_interval_ * [uniform 0.9 1.1]]
}

# XXX Must do this when the caching hierachy is ready
Http/Server/Inval/Yuc instproc set-parent-cache { cache } {
	$self instvar pcache_
	set pcache_ $cache

	# Send JOIN
	#puts "[$self id] joins cache [$pcache_ id]"
	$self send $pcache_ [$self get-joinsize] \
		"$pcache_ server-join $self $self"

	# Start heartbeat after some time, otherwise TCP connection may 
	# not be well established...
	$self instvar ns_
	$ns_ at [expr [$ns_ now] + [$self next-hb]] "$self heartbeat"
}

Http/Server/Inval/Yuc instproc heartbeat {} {
	$self instvar pcache_ ns_
	$self send $pcache_ [$self get-hbsize] \
		"$pcache_ server-hb [$self id]"
	$ns_ at [expr [$ns_ now] + [$self next-hb]] \
		"$self heartbeat"
}

Http/Server/Inval/Yuc instproc get-request { cl type pageid args } {
	eval $self next $cl $type $pageid $args
	if {($type == "GET") || ($type == "REF")} {
		$self count-request $pageid
	}
}

Http/Server/Inval/Yuc instproc invalidate { pageid modtime } {
	$self instvar pcache_ id_ enable_upd_

	if ![info exists pcache_] {
		error "Server $id_ doesn't have a parent cache!"
	}

	# One more invalidation
	$self count-inval $pageid

	if [$self is-pushable $pageid] {
		$self push-page $pageid $modtime
		return
	}

	# Send invalidation to every cache, assuming a connection 
	# exists between the server and the cache
	set size [$self get-invsize]
	# Mark invalidation packet as another fid
	set agent [[$self get-cnc $pcache_] agent]
	set fid [$agent set fid_]
	$agent set fid_ [Http set PINV_FID_]
	$self send $pcache_ $size "$pcache_ invalidate $pageid $modtime"
	$agent set fid_ $fid
	$self evTrace S INV p $pageid m $modtime z $size
}

Http/Server/Inval/Yuc instproc push-page { pageid modtime } {
	$self instvar pcache_ id_

	if ![info exists pcache_] {
		error "Server $id_ doesn't have a parent cache!"
	}
	# Do not send invalidation, instead send the new page to 
	# parent cache
	set size [$self get-size $pageid]
	set pageinfo [$self get-page $pageid]

	# Mark invalidation packet as another fid
	set agent [[$self get-cnc $pcache_] agent]
	set fid [$agent set fid_]
	$agent set fid_ [Http set PINV_FID_]
	$self send $pcache_ $size \
		"$pcache_ push-update $pageid $pageinfo"
	$agent set fid_ $fid
	$self evTrace S UPD p $pageid m $modtime z $size
}

Http/Server/Inval/Yuc instproc get-req-notify { pageid } {
	$self count-request $pageid
}

Http/Server/Inval/Yuc instproc handle-request-TLC { pageid args } {
	$self instvar tlc_
	array set data $args
	lappend res $data(size)	;# Same size of queries
	lappend res $tlc_
	return $res
}


#----------------------------------------------------------------------
# server + support for compound pages. 
# 
# A compound page is considered to be a frequently changing main page
# and several component pages which are usually big static images.
#
# XXX This is a naive implementation, which assumes single page and 
# fixed page size for all pages
#----------------------------------------------------------------------
Class Http/Server/Compound -superclass Http/Server

Http/Server/Compound instproc gen-pagesize { id } {
	if {$id == 0} {
		return [$self next $id]
	} else {
		$self instvar pgtr_
		if [info exists pgtr_] {
			return [$pgtr_ gen-comp-size $id]
		} else { 
			error "Must have a pagepool to generate compound page"
		}
	}
}

Http/Server/Compound instproc gen-age { id } {
	if {$id == 0} {
		return [$self next $id]
	} else {
		$self instvar pgtr_ ns_
		if [info exists pgtr_] {
			set mtime [$ns_ now]
			return [expr [$pgtr_ gen-comp-modtime $id $mtime] - $mtime]
		} else { 
			error "Must have a pagepool to generate compound page"
		}
	}
}

Class Http/Server/Inval/MYuc -superclass \
		{ Http/Server/Inval/Yuc Http/Server/Compound}

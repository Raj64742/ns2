
#
# many_tcp.tcl
# $Id: many_tcp.tcl,v 1.5 1998/06/30 17:17:58 heideman Exp $
#
# Copyright (c) 1998 University of Southern California.
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

proc usage {} {
	puts stderr {usage: ns rbp_simulation.tcl [options]

This simulation demonstrates large numbers of TCP flows.
Comments/bugs to John Heidemann <johnh@isi.edu>.

Currently this simulation appears to be memory limited for most
practical purposes.  Each active flow consumes ~100KB of memory (very
roughly).

Why is a flow so expensive?  Primarly because We allocate a separate
pair of nodes and links per flow.  (I don't know how to do otherwise
and preserve some reasonable statement about the independence of
randomly selected client RTTs).

How can performance be improved?

- use multiple concurrent flows per node (should give much better
memory performance at the cost of node RTT independence)

- use session-level simulation techniques (see ``Enabling Large-scale
simulations: selective abstraction approach to the study of multicast
protocols'' by Huang, Estrin, and Heidemann in MASCOTS '98).

- improve the efficiency of the underlying representations (there are
some ways to optimize some otcl common cases)  (this work is in progress)

- use higher level representations of TCP (in progress)


Options (specify with -OPTIONNAME OPTIONVALUE):
}
	global raw_opt_info
	puts stderr $raw_opt_info
	exit 1
}

global raw_opt_info
set raw_opt_info {
	# how long to run the sim?
	duration 30

	# initilization: just start n clients at time 0
	# NEEDSWORK:  a more realistic ramp-up model
	# or some kind of autodetection on when we've reached
	# steady state would be nice.
	initial-client-count 10

	#
	# BASIC TOPOLOGY:
	#
	# (The basic n clients on the left and right going through a
	# bottleneck.)
	#
	# cl_1                                                     cr_1
	# ...     ---- bottleneck_left ---- bottleneck_right ---   ...
	# cl_n                                                     cr_n
	#
	# Currently all data traffic flows left-to-right.
	# NEEDSWORK: relax this assumption (but Poduri and Nichols
	# I-D suggests that relaxing it won't change things much).
	#

	#
	# CLIENT TRAFFIC MODEL:
	#
	# arrival rate per second (arrival is poisson)
	client-arrival-rate 1
	# Currently clients are either mice or elephants.
	# NEEDSWORK:  should better model http-like traffic patterns.
	# In particular, netscape's 4-connection model makes
	# a *big* difference in traffic patterns
	# and is not currently modeled at all.
	client-mouse-chance 90
	client-mouse-packets 10
	client-elephant-packets 100
	# Pkt size in bytes.
	# NEEDSWORK:  should check that everything is uniformly
	# specified (router queues are in packets of 1000B length?).
	client-pkt-size 576

	#
	# CLIENT NETWORK CONNECTION:
	#
	client-bw 56kb
	# client-server rtt is uniform over this range (currently)
	# NEEDSWORK:  does this need to be modeled more accurately?
	client-delay random
	client-delay-range 100ms
	client-queue-method DropTail
	# Insure that client routers are never a bottleneck.
	client-queue-length 100

	#
	# CLIENT/SERVER TCP IMPLEMENTATION:
	#
	# NEEDSWORK: should add HTTP model over TCP.
	source-tcp-method TCP/Reno
	sink-ack-method TCPSink/DelAck

	#
	# BOTTLENECK LINK MODEL:
	#
	bottle-bw 10Mb
	bottle-delay 4ms
	bottle-queue-method RED
	# bottle-queue-length is either in packets or
	# is "bw-delay-product" which does the currently
	# expected thing.
	bottle-queue-length bw-delay-product

	#
	# OUTPUT OPTIONS:
	#
	graph-results 0
	graph-join-queueing 1
	gen-map 0
	mem-trace 0
	debug 1
	title none
	# set test-suite to write the graph to opts(test-suite-file)
	test-suite 0
	test-suite-file temp.rands
		   
	# Random number seed; default is 0, so ns will give a 
	# diff. one on each invocation.
	ns-random-seed 0
	
	# Animation options; complete traces are useful
	# for nam only, so do those only when a tracefile
	# is being used for nam
	trace-filename out
	trace-all 0
	namtrace-some 0
	namtrace-all 0
	
	# Switch to generate the nam tcl file from here
	# itself
	nam-generate-cmdfile 0
}

Class Main

Main instproc default_options {} {
	global opts opt_wants_arg raw_opt_info
	set cooked_opt_info $raw_opt_info

	while {$cooked_opt_info != ""} {
		if {![regexp "^\[^\n\]*\n" $cooked_opt_info line]} {
			break
		}
		regsub "^\[^\n\]*\n" $cooked_opt_info {} cooked_opt_info
		set line [string trim $line]
		if {[regexp "^\[ \t\]*#" $line]} {
			continue
		}
		if {$line == ""} {
			continue
		} elseif [regexp {^([^ ]+)[ ]+([^ ]+)$} $line dummy key value] {
			set opts($key) $value
			set opt_wants_arg($key) 1
		} else {
			set opt_wants_arg($key) 0
			# die "unknown stuff in raw_opt_info\n"
		}
	}
}

Main instproc process_args {av} {
	global opts opt_wants_arg

	$self default_options
	for {set i 0} {$i < [llength $av]} {incr i} {
		set key [lindex $av $i]
		if {$key == "-?" || $key == "--help" || $key == "-help" || $key == "-h"} {
			usage
		}
		regsub {^-} $key {} key
		if {![info exists opt_wants_arg($key)]} {
			puts stderr "unknown option $key";
			usage
		}
		if {$opt_wants_arg($key)} {
			incr i
			set opts($key) [lindex $av $i]
		} else {
			set opts($key) [expr !opts($key)]
		}
	}
}


proc my-duplex-link {ns n1 n2 bw delay queue_method queue_length} {
	$ns duplex-link $n1 $n2 $bw $delay $queue_method
	$ns queue-limit $n1 $n2 $queue_length
	$ns queue-limit $n2 $n1 $queue_length
}

proc my-duplex-link-set-delay {ns n1 n2 delay} {
	[$ns nodes-to-link $n1 $n2] set delay_ $delay
	[$ns nodes-to-link $n2 $n1] set delay_ $delay
}


Main instproc init_network {} {
	global opts
	# nodes
	# build right to left
	$self instvar bottle_l_ bottle_r_ cs_l_ cs_r_ ns_ cs_count_ ns_

	#
	# Figure supported load.
	#
	set expected_load_per_client_in_bps [expr ($opts(client-mouse-chance)/100.0)*$opts(client-mouse-packets)*$opts(client-pkt-size)*8 + (1.0-$opts(client-mouse-chance)/100.0)*$opts(client-elephant-packets)*$opts(client-pkt-size)*8]
	set max_clients_per_second [expr [$ns_ bw_parse $opts(bottle-bw)]/$expected_load_per_client_in_bps]
	puts "maximum clients per second: $max_clients_per_second"

	# Compute optimal (?) bottleneck queue size
	# as the bw-delay product.
	if {$opts(bottle-queue-length) == "bw-delay-product"} {
		set opts(bottle-queue-length) [expr ([$ns_ bw_parse $opts(bottle-bw)] * ([$ns_ delay_parse $opts(bottle-delay)] + [$ns_ delay_parse $opts(client-delay-range)]) + $opts(client-pkt-size)*8 - 1)/ ($opts(client-pkt-size) * 8)]
		puts "optimal bw queue size: $opts(bottle-queue-length)"
	}

	# Do our own routing with expanded addresses (21 bits nodes).
	# (Basic routing limits us to 128 nodes == 64 clients).
	$ns_ rtproto Manual
	$ns_ set-address-format expanded

	# set up the bottleneck
	set bottle_l_ [$ns_ node]
	set bottle_r_ [$ns_ node]
	my-duplex-link $ns_ $bottle_l_ $bottle_r_ $opts(bottle-bw) $opts(bottle-delay) $opts(bottle-queue-method) $opts(bottle-queue-length)

	# Bottlenecks need large routing tables.
#	[$bottle_l_ set classifier_] resize 511
#	[$bottle_r_ set classifier_] resize 511

	# Default routes to the other.
	$bottle_l_ add-route-to-adj-node -default $bottle_r_
	$bottle_r_ add-route-to-adj-node -default $bottle_l_

	# Clients are built dynamically.
	set cs_count_ 0
}

# return the client index
Main instproc create_a_client {} {
	global opts
	$self instvar bottle_l_ bottle_r_ cs_l_ cs_r_ sources_lr_ cs_count_ ns_

	set i $cs_count_
	incr cs_count_

	set cs_l_($i) [$ns_ node]
	set cs_r_($i) [$ns_ node]

	# bw/delay are bogus for now
	my-duplex-link $ns_ $cs_l_($i) $bottle_l_ $opts(client-bw) 10ms $opts(client-queue-method) $opts(client-queue-length)
	my-duplex-link $ns_ $cs_r_($i) $bottle_r_ $opts(client-bw) 10ms $opts(client-queue-method) $opts(client-queue-length)

	# Add routing in all directions
	$cs_l_($i) add-route-to-adj-node -default $bottle_l_
	$cs_r_($i) add-route-to-adj-node -default $bottle_r_
	$bottle_l_ add-route-to-adj-node $cs_l_($i)
	$bottle_r_ add-route-to-adj-node $cs_r_($i)

	# create sources and sinks in both directions
	# (actually, only left-to-right for now)
	set sources_lr_($i) [$ns_ create-connection $opts(source-tcp-method) $cs_l_($i) $opts(sink-ack-method) $cs_r_($i) $i]

#	set sources_rl_($i) [$ns_ create-connection $opts(source-tcp-method) $cs_r_($i) $opts(sink-ack-method) $cs_l_($i) $i]
	$sources_lr_($i) set maxpkts_ 0
	$sources_lr_($i) set packetSize_ $opts(client-pkt-size)
#	$sources_lr_($i) set syn_ 1

	# Set up a callback when this client ends.
	$sources_lr_($i) proc done {} "$self finish_a_client $i"

	if {$opts(debug)} {
		# puts "t=[$ns_ now]: client $i created"
	}

	return $i
}

#
# Make a batch of clients to amortize the cost of routing recomputation
# (actually no longer improtant).
#
Main instproc create_some_clients {} {
	global opts
	$self instvar idle_clients_ ns_ cs_count_

	set step 16
	if {$opts(debug)} {
		puts "t=[$ns_ now]: creating clients $cs_count_ to [expr $cs_count_ + $step - 1]"
	}

	for {set i 0} {$i < $step} {incr i} {
		lappend idle_clients_ [$self create_a_client]
	}

	# debugging:
	# puts "after client_create:"
	# $ns_ gen-map
	# $self instvar bottle_l_ bottle_r_
	# puts "bottle_l_ classifier_:"
	# [$bottle_l_ set classifier_] dump
	# puts "bottle_r_ classifier_:"
	# [$bottle_r_ set classifier_] dump
}

Main instproc start_a_client {} {
	global opts
	$self instvar idle_clients_ ns_ cs_l_ bottle_l_ cs_r_ bottle_r_ sources_lr_ rng_ source_start_ source_size_

	set i ""
	# can we reuse a dead client?
	if {![info exists idle_clients_]} {
		set idle_clients_ ""
	}
	while {$idle_clients_ == ""} {
		$self create_some_clients
	}
	set i [lindex $idle_clients_ 0]
	set idle_clients_ [lrange $idle_clients_ 1 end]

	# Set delay.
	set delay $opts(client-delay)
	if {$delay == "random"} {
		set delay [$rng_ exponential [$ns_ delay_parse $opts(client-delay-range)]]
	}
	# Now divide the delay into the two haves and set up the network.
	set ldelay [$rng_ uniform 0 $delay]
	set rdelay [expr $delay - $ldelay]
	my-duplex-link-set-delay $ns_ $cs_l_($i) $bottle_l_ $ldelay
	my-duplex-link-set-delay $ns_ $cs_r_($i) $bottle_r_ $rdelay

	# Start traffic for that client.
	if {[$rng_ integer 100] < $opts(client-mouse-chance)} {
		set len $opts(client-mouse-packets)
	} else {
		set len $opts(client-elephant-packets)
	}

	$sources_lr_($i) advanceby $len

	set source_start_($i) [$ns_ now]
	set source_size_($i) $len
	if {$opts(debug)} {
#		puts "t=[$ns_ now]: client $i started, ldelay=$ldelay, rdelay=$rdelay"
#		puts "t=[$ns_ now]: client $i started, ldelay=[format %.6f $ldelay], rdelay=[format %.6f $rdelay]"
		puts "t=[$ns_ now]: client $i started"
	}
}

Main instproc finish_a_client {i} {
	global opts
	$self instvar ns_ idle_clients_ source_start_ source_size_

	if {$opts(debug)} {
		set delta [expr [$ns_ now] - $source_start_($i)]
		puts "t=[$ns_ now]: client $i finished ($source_size_($i) pkts, $delta s)"
	}

	lappend idle_clients_ $i
}

Main instproc schedule_continuing_traffic {} {
	global opts
	$self instvar ns_ rng_

	$self start_a_client
	# schedule the next one
	set next [expr [$ns_ now]+([$rng_ exponential]/$opts(client-arrival-rate))]
	if {$opts(debug)} {
		# puts "t=[$ns_ now]: next continuing traffic at $next"
	}
	$ns_ at $next "$self schedule_continuing_traffic"
}

Main instproc schedule_initial_traffic {} {
	global opts

	# Start with no pending clients.
	$self instvar idle_clients_

	# Start initial clients.
	for {set i 0} {$i < $opts(initial-client-count)} {incr i} {
		$self start_a_client
	}
}

Main instproc open_trace { stop_time } {
	global opts
	$self instvar ns_ trace_file_ nam_trace_file_ trace_filename_
	set trace_filename_ $opts(trace-filename)
	exec rm -f "$trace_filename_.tr"
	set trace_file_ [open "$trace_filename_.tr" w]
	set stop_actions "close $trace_file_"
	if {$opts(namtrace-some) || $opts(namtrace-all)} {
		exec rm -f "$trace_filename_.nam"
		set nam_trace_file_ [open "$trace_filename_.nam" w]
		set $stop_actions "$stop_actions; close $nam_trace_file_"
	} else {
		set nam_trace_file_ ""
	}
	$ns_ at $stop_time "$stop_actions; $self finish"
	return "$trace_file_ $nam_trace_file_"
}

# There seems to be a problem with the foll function, so quit plotting 
# with -a -q, use just -a.

Main instproc finish {} {
        global opts
	$self instvar trace_filename_ ns_

	if {$opts(graph-join-queueing)} {
		set q "-q"
	} else {
		set q ""
	}
	set title $opts(title)
	# Make sure that we run in place even without raw2xg in our path
	# (for the test suites).
	set raw2xg raw2xg
	if [file exists ../../raw2xg] {
		set raw2xg ../../raw2xg
	}
	exec $raw2xg -a $q < $trace_filename_.tr >$trace_filename_.xg
	if {$opts(test-suite)} {
		exec cp $trace_filename_.xg $opts(test-suite-file)
	}
	if {$opts(graph-results)} {
		exec xgraph -t $title  < $trace_filename_.xg &
	}
#	exec raw2xg -a < out.tr | xgraph -t "$opts(server-tcp-method)" &

	if {$opts(mem-trace)} {
		$ns_ clearMemTrace
	}
	exit 0
}

Main instproc trace_stuff {} {
	global opts

	$self instvar bottle_l_ bottle_r_ ns_ trace_file_ nam_trace_file_
	$self open_trace $opts(duration)

	if {$opts(trace-all)} {
		$ns_ trace-all $trace_file_
	}

	if {$opts(namtrace-all)} {
		$ns_ namtrace-all $nam_trace_file_
	} elseif {$opts(namtrace-some)} {
# xxx
		$bottle_l_ dump-namconfig
		$bottle_r_ dump-namconfig
		[$ns_ nodes-to-link $bottle_l_ $bottle_r_] dump-namconfig
		$ns_ namtrace-queue $bottle_l_ $bottle_r_ $nam_trace_file_
		$ns_ namtrace-queue $bottle_r_ $bottle_l_ $nam_trace_file_
	}

	# regular tracing.
	# trace left-to-right only
       	$ns_ trace-queue $bottle_l_ $bottle_r_ $trace_file_
       	$ns_ trace-queue $bottle_r_ $bottle_l_ $trace_file_
	
	# Currently tracing is somewhat broken because
	# of how the plumbing happens.
}

Main instproc init {av} {
	global opts

	$self process_args $av

	$self instvar ns_ 
	set ns_ [new Simulator]

        # Seed random no. generator; ns-random with arg of 0 heuristically
        # chooses a random number that changes on each invocation.
	$self instvar rng_
	set rng_ [new RNG]
	$rng_ seed $opts(ns-random-seed)
	$rng_ next-random

	$self init_network
	$self trace_stuff
	# xxx: hack (next line)
#	$self create_some_clients
	$ns_ at 0 "$self schedule_initial_traffic"
	if {$opts(client-arrival-rate) != 0} {
		$ns_ at 0 "$self schedule_continuing_traffic"
	}

	if {$opts(gen-map)} {
		$ns_ gen-map
	}       
	$ns_ run
}

global in_test_suite
if {![info exists in_test_suite]} {
	global $argv
	new Main $argv
}


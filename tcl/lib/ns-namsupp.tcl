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
# ns trace support for nam
#
# Author: Haobo Yu, haoboy@isi.edu
#
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-namsupp.tcl,v 1.1 1997/09/12 19:24:07 haoboy Exp $
#

#
# Changes in other files 
# ----------------------
#
# Functions changed: (i.e., all related to traceAllFile_)
#
# tcl/lib/ns-lib.tcl
#   Simulator::node { {shape "circle"} {color "black"} }
#   Simulator::duplex-link { n1 n2 bw delay type {ori "right"} {q_clrid 0} }
#   Simulator::simplex-link
#   Simulator::duplex-link-of-interfaces
#   Simulator::multi-link
#   Simulator::multi-link-of-interfaces
# tcl/lan/ns-lan.tcl
#   Simulator::make-lan
#
# Functions added
# 
# tcl/lib/ns-lib.tcl
#   Simulator::nnamtrace-all
#   Simulator::namtrace-queue
# tcl/lib/ns-link.tcl
#   SimpleLink::nam-trace

#----------------------------------------------------------------------
# Following should stay in this file
#----------------------------------------------------------------------

#
# Support for node coloring. Used in 
#
Node instproc trace { file shape color } {
	# write a configuration to file immediately
	$self instvar trace_ id_ color_
	set trace_ $file
	set color_ $color    ;# save this for later tracing
	puts $trace_ [eval list "n -t* -s$id_ -SUP -v$shape -c$color"]
}

Node instproc change-color { color } {
	$self instvar trace_ id_ color_
	if [info exists trace_] {
		set ns [Simulator instance]
		# we don't need shape info here, and don't need UP/DOWN?
		puts $trace_ [eval list "n -t[$ns now] -s$id_ -SCOLOR -c$color -o$color_"]
		set color_ $color
	}
}

Node instproc get-color {} {
	$self instvar trace_ color_
	if [info exists trace_] {
		return $color_
	}
}

Simulator instproc color { id name } {
	$self instvar Color_ namtraceAllFile_

	if [info exists namtraceAllFile_] {
		puts $namtraceAllFile_ "c -t* -i$id -n$name"
	}
}

#
# support for agent tracing
#

#
# agent trace is added when attaching to a traced node
# we need to keep a file handle in tcl so that var tracing can also be 
# done in tcl by manual inserting update-var-trace{}
#
Agent instproc attach-trace { file } {
	$self instvar namTrace_
	set namTrace_ $file 
	# add all traced var messages
	$self attach $file 
}

#
# keep the following because we can then use both TracedVar and 
# insert var tracing code wherever we like.
#
Agent instproc add-var-trace { name value {type "v"} } {
	$self instvar namTrace_

	if [info exists namTrace_] {
		set ns [Simulator instance]
		$self instvar addr_ dst_ ntName_ features_
		set addr [expr $addr_ >> 8]
		set dst [expr $dst_ >> 8]
		puts $namTrace_ "f -t[$ns now] -s$addr -d$dst -T$type -n$name -v$value -a$ntName_"
		set features_($name) $value
	}
}

Agent instproc update-var-trace { name value {type "v"} } {
	$self instvar namTrace_

	if [info exists namTrace_] {
		set ns [Simulator instance]
		$self instvar addr_ dst_ ntName_ features_
		set addr [expr $addr_ >> 8]
		set dst [expr $dst_ >> 8]
		puts $namTrace_ "f -t[$ns now] -s$addr -d$dst -T$type -n$name -v$value -a$ntName_ -o$features_($name)"
		set features_($name) $value
	}
}

Agent instproc delete-var-trace { name } {
	$self instvar namTrace_

	if [info exists namTrace_] {
		set ns [Simulator instance]
		$self instvar addr_ dst_ ntName_ features_
		set addr [expr $addr_ >> 8]
		set dst [expr $dst_ >> 8]
		puts $namTrace_ "f -t[$ns now] -s$addr -d$dst -n$name -a$ntName_ -o$features_($name) -x"
		unset features_($name)
	}
}

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
# Author: Haobo Yu (haoboy@isi.edu)
#
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-namsupp.tcl,v 1.31 1999/09/09 03:34:36 salehi Exp $
#

#
# Support for node tracing
#

# This will only work during initialization. Not possible to change shape 
# dynamically
Node instproc shape { shape } {
	$self instvar attr_ 
	set attr_(SHAPE) $shape
}

# Returns the current shape of the node
Node instproc get-shape {} {
	$self instvar attr_
	if [info exists attr_(SHAPE)] {
		return $attr_(SHAPE)
	} else {
		return ""
	}
}

Node instproc color { color } {
	$self instvar attr_ id_

	set ns [Simulator instance]

	if [$ns is-started] {
		# color must be initialized
		$ns puts-nam-config \
			[eval list "n -t [$ns now] -s $id_ -S COLOR -c $color -o $attr_(COLOR)"]
		set attr_(COLOR) $color
	} else {
		set attr_(COLOR) $color
	}
}

Node instproc label { str } {
	$self instvar attr_ id_

	set ns [Simulator instance]

	if [info exists attr_(DLABEL)] {
		$ns puts-nam-config \
		  "n -t [$ns now] -s $id_ -S DLABEL -l \"$str\" -L $attr_(DLABEL)"
	} else {
		$ns puts-nam-config \
			"n -t [$ns now] -s $id_ -S DLABEL -l \"$str\" -L \"\""
	}
	set attr_(DLABEL) \"$str\"
}

Node instproc dump-namconfig {} {
	$self instvar attr_ id_ address_
	set ns [Simulator instance]

	if ![info exists attr_(SHAPE)] {
		set attr_(SHAPE) "circle"
	} 
	if ![info exists attr_(COLOR)] {
		set attr_(COLOR) "black"
	}
	$ns puts-nam-config \
		[eval list "n -t * -a $address_ -s $id_ -S UP -v $attr_(SHAPE) -c $attr_(COLOR)"]
}

Node instproc change-color { color } {
	puts "Warning: Node::change-color is obsolete. Use Node::color instead"
	$self color $color
}

Node instproc get-attribute { name } {
	$self instvar attr_
	if [info exists attr_($name)] {
		return $attr_($name)
	} else {
		return ""
	}
}

Node instproc get-color {} {
	puts "Warning: Node::get-color is obsolete. Please use Node::get-attribute"
	return [$self get-attribute "COLOR"]
}

Node instproc add-mark { name color {shape "circle"} } {
	$self instvar id_ markColor_ shape_
	set ns [Simulator instance]

	$ns puts-nam-config "m -t [$ns now] -s $id_ -n $name -c $color -h $shape"
	set markColor_($name) $color
	set shape_($name) $shape
}

Node instproc delete-mark { name } {
	$self instvar id_ markColor_ shape_

	# Ignore if the mark $name doesn't exist
	if ![info exists markColor_($name)] {
		return
	}

	set ns [Simulator instance]
	$ns puts-nam-config \
		"m -t [$ns now] -s $id_ -n $name -c $markColor_($name) -h $shape_($name) -X"
}

#
# Support for link tracing
# XXX only SimpleLink (and its children) can dump nam config, because Link
# doesn't have bandwidth and delay.
#
SimpleLink instproc dump-namconfig {} {
	# make a duplex link in nam
	$self instvar link_ attr_ fromNode_ toNode_

	if ![info exists attr_(COLOR)] {
		set attr_(COLOR) "black"
	}

	if ![info exists attr_(ORIENTATION)] {
		set attr_(ORIENTATION) ""
	}

	set ns [Simulator instance]
	set bw [$link_ set bandwidth_]
	set delay [$link_ set delay_]

	$ns puts-nam-config \
		"l -t * -s [$fromNode_ id] -d [$toNode_ id] -S UP -r $bw -D $delay -c $attr_(COLOR) -o $attr_(ORIENTATION)"
}

Link instproc dump-nam-queueconfig {} {
	$self instvar attr_ fromNode_ toNode_

	if ![info exists attr_(COLOR)] {
		set attr_(COLOR) "black"
	}

	set ns [Simulator instance]
	if [info exists attr_(QUEUE_POS)] {
		$ns puts-nam-config "q -t * -s [$fromNode_ id] -d [$toNode_ id] -a $attr_(QUEUE_POS)"
	} else {
		set attr_(QUEUE_POS) ""
	}
}

#
# XXX
# This function should be called ONLY ONCE during initialization. 
# The order in which links are created in nam is determined by the calling 
# order of this function.
#
Link instproc orient { ori } {
	$self instvar attr_
	set attr_(ORIENTATION) $ori
	[Simulator instance] register-nam-linkconfig $self
}

Link instproc get-attribute { name } {
	$self instvar attr_
	if [info exists attr_($name)] {
		return $attr_($name)
	} else {
		return ""
	}
}

Link instproc queuePos { pos } {
	$self instvar attr_
	set attr_(QUEUE_POS) $pos
}

Link instproc color { color } {
	$self instvar attr_ fromNode_ toNode_ trace_

	set ns [Simulator instance]
	if [$ns is-started] {
		$ns puts-nam-config \
			[eval list "l -t [$ns now] -s [$fromNode_ id] -d [$toNode_ id] -S COLOR -c $color -o $attr_(COLOR)"]
		set attr_(COLOR) $color
	} else {
		set attr_(COLOR) $color
	}
}

# a link doesn't have its own trace file, write it to global trace file
Link instproc change-color { color } {
	puts "Warning: Link::change-color is obsolete. Please use Link::color."
	$self color $color
}

Link instproc get-color {} {
	puts "Warning: Node::get-color is obsolete. Please use Node::get-attribute"
	return [$self get-attribute "COLOR"]
}

#
# Support for agent tracing
#

# This function records agents being traced, so they will be written into nam
# trace when the simulator starts
Simulator instproc add-agent-trace { agent name {f ""} } {
	$self instvar tracedAgents_
	set tracedAgents_($name) $agent

	set trace [$self get-nam-traceall]
	if {$f != ""} {
		$agent attach-trace $f
	} elseif {$trace != ""} {
		$agent attach-trace $trace
	}
}

Simulator instproc delete-agent-trace { agent } {
	$agent delete-agent-trace
}

Simulator instproc monitor-agent-trace { agent } {
	$self instvar monitoredAgents_
	lappend monitoredAgents_ $agent
}

#
# Agent trace is added when attaching to a traced node
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
# nam initialization
#
Simulator instproc dump-namagents {} {
	$self instvar tracedAgents_ monitoredAgents_
	
	if {![$self is-started]} {
		return
	}
	if [info exists tracedAgents_] {
		foreach id [array names tracedAgents_] {
			$tracedAgents_($id) add-agent-trace $id
			$tracedAgents_($id) cmd dump-namtracedvars
		}
		unset tracedAgents_
	}
	if [info exists monitoredAgents_] {
		foreach a $monitoredAgents_ {
			$a show-monitor
		}
		unset monitoredAgents_
	}
}

Simulator instproc dump-namversion { v } {
	$self puts-nam-config "V -t * -v $v -a 0"
}

Simulator instproc dump-namcolors {} {
	$self instvar color_
	if ![$self is-started] {
		return 
	}
	foreach id [array names color_] {
		$self puts-nam-config "c -t * -i $id -n $color_($id)"
	}
}

Simulator instproc dump-namlans {} {
	if ![$self is-started] {
		return
	}
	$self instvar Node_
	foreach nn [array names Node_] {
		if [$Node_($nn) is-lan?] {
			$Node_($nn) dump-namconfig
		}
	}
}

Simulator instproc dump-namlinks {} {
	$self instvar linkConfigList_
	if ![$self is-started] {
		return
	}
	if [info exists linkConfigList_] {
		foreach lnk $linkConfigList_ {
			$lnk dump-namconfig
		}
		unset linkConfigList_
	}
}

Simulator instproc dump-namnodes {} {
	$self instvar Node_
	if ![$self is-started] {
		return
	}
	foreach nn [array names Node_] {
		if ![$Node_($nn) is-lan?] {
			$Node_($nn) dump-namconfig
		}
	}
}

Simulator instproc dump-namqueues {} {
	$self instvar link_
	if ![$self is-started] {
		return
	}
	foreach qn [array names link_] {
		$link_($qn) dump-nam-queueconfig
	}
}

# Write hierarchical masks/shifts into trace file
Simulator instproc dump-namaddress {} {
	AddrParams instvar hlevel_ NodeShift_ NodeMask_ McastShift_ McastMask_
	
	# First write number of hierarchies
	$self puts-nam-config \
	    "A -t * -n $hlevel_ -p 0 -o [AddrParams set ALL_BITS_SET] -c $McastShift_ -a $McastMask_"
	
	for {set i 1} {$i <= $hlevel_} {incr i} {
		$self puts-nam-config \
		    "A -t * -h $i -m $NodeMask_($i) -s $NodeShift_($i)"
	}
}

Simulator instproc init-nam {} {
	$self instvar annotationSeq_ 

	set annotationSeq_ 0

	# Setting nam trace file version first
	$self dump-namversion 1.0a5
	
	# Addressing scheme
	$self dump-namaddress
	
	# Color configuration for nam
	$self dump-namcolors
	
	# Node configuration for nam
	$self dump-namnodes
	
	# Lan and Link configurations for nam
	$self dump-namlinks 
	$self dump-namlans
	
	# nam queue configurations
	$self dump-namqueues
	
	# Traced agents for nam
	$self dump-namagents

}

#
# Other animation control support
# 
Simulator instproc trace-annotate { str } {
	$self instvar annotationSeq_
	$self puts-ns-traceall [format \
		"v %s %s {set sim_annotation {%s}}" [$self now] eval $str]
	incr annotationSeq_
	$self puts-nam-config \
	  "v -t [$self now] sim_annotation [$self now] $annotationSeq_ $str"
}

proc trace_annotate { str } {
	set ns [Simulator instance]

	$ns trace-annotate $str
}

proc flash_annotate { start duration msg } {
	set ns [Simulator instance]
	$ns at $start "trace_annotate {$msg}"
	$ns at [expr $start+$duration] "trace_annotate periodic_message"
}

# rate's unit is second
Simulator instproc set-animation-rate { rate } {
	# time_parse defined in tcl/rtp/session-rtp.tcl
	set r [time_parse $rate]
	$self puts-nam-config "v -t [$self now] set_rate [expr 10*log10($r)] 1"
}

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
# $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-namsupp.tcl,v 1.9 1998/03/03 02:01:42 haoboy Exp $
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

Node instproc color { color } {
	$self instvar attr_ id_

	set ns [Simulator instance]

	if [$ns is-started] {
		# color must be initialized
		$ns puts-nam-traceall \
			[eval list "n -t [$ns now] -s $id_ -S COLOR -c $color -o $attr_(COLOR)"]
		set attr_(COLOR) $color
	} else {
		set attr_(COLOR) $color
	}
}

Node instproc dump-namconfig {} {
	$self instvar attr_ id_
	set ns [Simulator instance]

	if ![info exists attr_(SHAPE)] {
		set attr_(SHAPE) "circle"
	} 
	if ![info exists attr_(COLOR)] {
		set attr_(COLOR) "black"
	}
	$ns puts-nam-traceall \
		[eval list "n -t * -s $id_ -S UP -v $attr_(SHAPE) -c $attr_(COLOR)"]
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

	$ns puts-nam-traceall "m -t [$ns now] -s $id_ -n $name -c $color -h $shape"
	set markColor_($name) $color
	set shape_($name) $shape
}

Node instproc delete-mark { name } {
	$self instvar id_ markColor_ shape_
	set ns [Simulator instance]

	$ns puts-nam-traceall \
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

	$ns puts-nam-traceall \
		"l -t * -s [$fromNode_ id] -d [$toNode_ id] -S UP -r $bw -D $delay -o $attr_(ORIENTATION)"
}

Link instproc dump-nam-queueconfig {} {
	$self instvar attr_ fromNode_ toNode_

	if ![info exists attr_(COLOR)] {
		set attr_(COLOR) "black"
	}

	set ns [Simulator instance]
	if [info exists attr_(QUEUE_POS)] {
		$ns puts-nam-traceall "q -t * -s [$fromNode_ id] -d [$toNode_ id] -a $attr_(QUEUE_POS)"
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
	$self instvar attr_ fromNode_ toNode_ 

	set ns [Simulator instance]
	if [$ns is-started] {
		$ns puts-nam-traceall \
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
# Lan
#
MultiLink instproc dump-namconfig {} {
	$self instvar attr_ nodes_ bw_ delay_ id_

	if ![info exists attr_(ORIENT)] {
		set attr_(ORIENT) "left"
	}

	set ns [Simulator instance]
	# X -t * -n <nodes> -r <band width> -D <delay> -o <orientation>
	$ns puts-nam-traceall \
		"X -t * -n $id_ -r $bw_ -D $delay_ -o $attr_(ORIENT)"
	# L -t * -s 10 -d 9 -o <orientation>
	foreach n $nodes_ {
		set nid [$n id]
		if ![info exists attr_($nid)] {
			set attr_($nid) "down"
		}
		$ns puts-nam-traceall \
			"L -t * -s $id_ -d $nid -o $attr_($nid)"
	}
}

# To get lan link orientation, give the node's id
MultiLink instproc get-attribute { name } {
	$self instvar attr_
	if [info exists attr_($name)] {
		return $attr_($name)
	} else {
		return ""
	}
}

MultiLink instproc orient {ori} {
	$self instvar attr_
	set attr_(ORIENT) $ori
}

MultiLink instproc nodePos { n pos } {
	$self instvar attr_
	set nid [$n id]
	set attr_($nid) $pos
}

#
# Ugly hack to produce a fully connected lan. The bus version of lan 
# doesn't work for now. :(
#
DummyLink instproc dump-namconfig {} {
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

	$ns puts-nam-traceall \
		"l -t * -s [$fromNode_ id] -d [$toNode_ id] -S UP -r $bw -D $delay -o $attr_(ORIENTATION)"
}

#
# Support for agent tracing
#

# This function records agents being traced, so they will be written into nam
# trace when the simulator starts
Simulator instproc add-agent-trace { agent name } {
	$self instvar tracedAgents_
	set tracedAgents_($name) $agent
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
# Keep the following because we can then use both TracedVar and 
# insert var tracing code wherever we like.
#
Agent instproc add-var-trace { name value {type "v"} } {
	$self instvar namTrace_

	if [info exists namTrace_] {
		set ns [Simulator instance]
		$self instvar addr_ dst_ ntName_ features_
		set addr [expr $addr_ >> 8]
		set dst [expr $dst_ >> 8]
		puts $namTrace_ "f -t [$ns now] -s $addr -d $dst -T $type -n $name -v $value -a $ntName_"
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
		puts $namTrace_ "f -t [$ns now] -s $addr -d $dst -T $type -n $name -v $value -a $ntName_ -o $features_($name)"
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
		puts $namTrace_ "f -t [$ns now] -s $addr -d $dst -n $name -a $ntName_ -o $features_($name) -x"
		unset features_($name)
	}
}

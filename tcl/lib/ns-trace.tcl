#
# Copyright (c) 1996-1997 Regents of the University of California.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
# 	This product includes software developed by the MASH Research
# 	Group at the University of California Berkeley.
# 4. Neither the name of the University nor of the Research Group may be
#    used to endorse or promote products derived from this software without
#    specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-trace.tcl,v 1.12 1997/11/04 22:26:46 haoboy Exp $
#

Trace instproc init type {
	$self next $type
	$self instvar type_
	set type_ $type
}

Trace instproc format args {
	# The strange puts construction below helps us write formats such as
	# 	$traceObject format {$src_} {$dst_} 
	# that will then put the source or destination id in the desired place.

	$self instvar type_ fp_ src_ dst_

	if [info exists fp_] {
		set ns [Simulator instance]
		puts $fp_ [eval list $type_ [$ns now] [eval concat $args]]
	}
}

Trace instproc attach fp {
	$self instvar fp_

	set fp_ $fp
	$self cmd attach $fp_
}

Class Trace/Hop -superclass Trace
Trace/Hop instproc init {} {
	$self next "h"
}

Class Trace/Enque -superclass Trace
Trace/Enque instproc init {} {
	$self next "+"
}

Trace/Deque instproc init {} {
	$self next "-"
}

Class Trace/Recv -superclass Trace 
Trace/Recv instproc init {} {
	$self next "r"
}

Class Trace/Drop -superclass Trace
Trace/Drop instproc init {} {
	$self next "d"
}

Class Trace/Generic -superclass Trace
Trace/Generic instproc init {} {
	$self next "v"
}

# var trace shouldn't be derived here because it shouldn't be a connector
# it's here only for backward compatibility
Class Trace/Var -superclass Trace
Trace/Var instproc init {} {
	$self next "f"
}

proc gc o {
	set ret "NULL_OBJECT"
	if { $o != "" } {
		set ret ""
		foreach i $o {
			if ![catch "$i info class" val] {
				lappend ret $val
			}
		}
	}
	set ret
}

Node instproc tn {} {
	$self instvar id_
	return "${self}(id $id_)"
}

Simulator instproc gen-map {} {
	# Did you ever see such uglier code? duh?
	#

	$self instvar Node_ link_

	set nn [Node set nn_]
	for {set i 0} {$i < $nn} {incr i} {
		set n $Node_($i)
		puts "Node [$n tn]"
		foreach nc [$n info vars] {
			switch $nc {
				ns_		continue
				id_		continue
				neighbor_	continue
				agents_		continue
				routes_		continue
				np_		continue
				default {
					if [$n array exists $nc] {
						puts "\t\t$nc\t[$n array get $nc]"
					} else {
						set v [$n set $nc]
						puts "\t\t$nc${v}([gc $v])"
					}
				}
			}
		}
		# Would be nice to dump agents attached to the dmux here?
		if {[llength [$n set agents_]] > 0} {
			puts "\n\tAgents at node (possibly in order of creation):"
			foreach a [$n set agents_] {
				puts "\t\t$a\t[gc $a]\t\tportID: [$a set portID_]([$a set addr_])"
			}
		}
		puts ""
		foreach li [array names link_ [$n id]:*] {
			set L [split $li :]
			set nbr [[$self get-node-by-id [lindex $L 1]] entry]
			set ln $link_($li)
			puts "\tLink $ln, fromNode_ [[$ln set fromNode_] tn] -> toNode_ [[$ln set toNode_] tn]"
			puts "\tComponents (in order) head first"
			for {set c [$ln head]} {$c != $nbr} {set c [$c target]} {
				puts "\t\t$c\t[gc $c]"
			}
		}
		puts "---"
	}
}

Simulator instproc trace-annotate { str } {
	$self puts-ns-traceall [format \
		"v %s %s {set sim_annotation {%s}}" [$self now] eval $str]
	$self puts-nam-traceall "v -t [$self now] sim_annotation [$self now] $str"
}

proc trace_annotate { str } {
	set ns [Simulator instance]

	$ns trace-annotate $str
}

proc flash_annotate { start duration msg } {
	global ns
	$ns at $start "trace_annotate {$msg}"
	$ns at [expr $start+$duration] "trace_annotate periodic_message"
}

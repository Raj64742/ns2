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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/test/test-suite-manual-routing.tcl,v 1.2 2001/11/28 23:04:26 sfloyd Exp $
#

#
# invoked as ns $file $t [QUIET]
# expected to pop up xgraph output (unless QUIET)
# and to leave the plot in temp.rands
#

proc usage {} {
	puts stderr {usage: ns test-suite-manual-routing.tcl test [QUIET]

Test suites for manual routing.
}
	exit 1
}

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.

global in_test_suite
set in_test_suite 1
source ../ex/many_tcp.tcl



# these are fakes for test-all

Class Test/one_client


proc main {} {
	global argv

	#
	# Icky icky icky.
	# We slap a test-suite-friendly interface over
	# the otherwise nice interface provided by
	# rbp_demo.tcl.
	#
	set graph 1
	foreach i $argv {
		switch $i {
			quiet {
				set graph 0
			}
			QUIET {
				set graph 0
			}
			one_client {
				set args "-duration 10 -initial-client-count 1 -client-arrival-rate 0"
				set title $i
			}
		}
	}
	# Always set ns-random-seed so we get the same results every time.
	new Main "-graph-results $graph -ns-random-seed 1 -test-suite 1 -title $title $args"
}

main




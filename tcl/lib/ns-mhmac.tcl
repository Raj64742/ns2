#
# Copyright (c) 1996 Regents of the University of California.
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
# @(#) $Header: /usr/src/mash/repository/vint/ns-2/tcl/lib/ns-shlink.tcl
#

Mac/Multihop set bandwidth_ 100Kb
Mac/Multihop set delay_ 10ms
Mac/Multihop set tx_rx_ 11.125ms
Mac/Multihop set rx_tx_ 13.25ms
Mac/Multihop set rx_rx_ 10.5625
Mac/Multihop set backoffBase_ 20ms
Mac/Multihop set hlen_ 16


# This should eventually be in ns-lib.tcl
Simulator instproc macnode {} {
	$self instvar Node_
	set node [new Node/MacNode]
	set Node_([$node id]) $node
	return $node
}

# In the long run, this should be done in the route logic
Class Node/MacNode -superclass Node
Node/MacNode instproc init args {
	eval $self next $args
	$self instvar macList_
	$self instvar numMacs_

	set macList_ {}
	set numMacs_ 0
}

# Hook up the MACs together at a given MacNode
Node/MacNode instproc linkmacs {} { 
	$self instvar macList_
	$self instvar numMacs_

	puts "#macs = $numMacs_"
	puts "[expr 1%1]"

	for {set i 0} {$i < $numMacs_} {incr i} {
		puts "MACLIST $macList_"
		puts "MACLIST($i) [lindex $macList_ $i]"
		[lindex $macList_ $i] set-maclist [lindex $macList_ [expr ($i+1)%$numMacs_]]
	}
}


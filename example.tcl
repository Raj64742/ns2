#
# Copyright (c) 1995 Regents of the University of California.
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
#	This product includes software developed by the Network Research
#	Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/example.tcl,v 1.1.1.1 1996/12/19 03:22:44 mccanne Exp $ (LBL)
#

#
# Create two nodes and connect them with a 1.5Mb link with a
# transmission delay of 10ms using FIFO drop-tail queueing
#
set n0 [ns node]
set n1 [ns node]
ns_duplex $n0 $n1 1.5Mb 10ms drop-tail

#
# Set up BSD Tahoe TCP connection in opposite directions.
#
set src1 [ns agent tcp $n0]
set snk1 [ns agent tcp-sink $n1]
set src2 [ns agent tcp $n1]
set snk2 [ns agent tcp-sink $n0]
ns_connect $src1 $snk1
ns_connect $src2 $snk2
$src1 set class 1
$src2 set class 2

#
# Create ftp sources at the each node
#
set ftp1 [$src1 source ftp]
set ftp2 [$src2 source ftp]

#
# Start up the first ftp at the time 0 and
# the second ftp staggered 1 second later
#
ns at 0.0 "$ftp1 start"
ns at 1.0 "$ftp2 start"

#
# Create a trace and arrange for all link
# events to be dumped to "out.tr"
#
set trace [ns trace]
$trace attach [open out.tr w]
foreach link [ns link] {
        $link trace $trace
}

#
# Dump the queueing delay on the n0->n1 link
# to stdout every second of simulation time.
#
proc dump { link interval } {
	ns at [expr [ns now] + $interval] "dump $link $interval"
	set delay [expr 8 * [$link integral qsize] / [$link get bandwidth]]
	puts "[ns now] delay=$delay"
}
ns at 0.0 "dump [ns link $n0 $n1] 1"

#
# run the simulation for 20 simulated seconds
#
ns at 20.0 "exit 0"
ns run

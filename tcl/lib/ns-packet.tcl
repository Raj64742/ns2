#
# Copyright (c) 1997 Regents of the University of California.
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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-packet.tcl,v 1.3.2.1 1997/04/12 07:14:46 padmanab Exp $
#
#
# set up the packet format for the simulation
# (initial version)
#

PacketHeaderManager set hdrlen_ 0

Simulator instproc create_packetformat { } {
	set pm [new PacketHeaderManager]
	foreach pair {
		{ Common off_cmn_ }
		{ LL off_ll_ }
		{ IP off_ip_ }
		{ TCP off_tcp_ }
		{ Flags off_flags_ }
		{ RTP off_rtp_ } 
		{ Message off_msg_ }
                { IVS off_ivs_ }
                { rtProtoDV off_DV_ }} {

		set cl [lindex $pair 0]
		set var [lindex $pair 1]
		set off [$pm allochdr $cl]
		TclObject set $var $off
	}
}

PacketHeaderManager instproc allochdr cl {
	set size [PacketHeader/$cl set hdrlen_]

	$self instvar hdrlen_
	set NS_ALIGN 8
	# round up to nearest NS_ALIGN bytes
	set incr [expr ($size + ($NS_ALIGN-1)) & ~($NS_ALIGN-1)]
	set base $hdrlen_
	incr hdrlen_ $incr

	return $base
}

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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-source.tcl,v 1.9 1997/07/22 09:06:40 padmanab Exp $
#

Class Source

#set ns_telnet(interval) 1000ms
#set ns_bursty(interval) 0
#set ns_bursty(burst-size) 2
#set ns_message(packet-size) 40

Source instproc init {} {
	$self next
	$self instvar maxpkts_
	#XXX Do not set this to 2^31; it breaks tcp-full.cc due to integer overflow
	# Currently set to 2^28
	set maxpkts_ 268435456
}
Class Source/FTP -superclass Source

Source/FTP instproc start {} {
	$self instvar agent_ maxpkts_
	$agent_ advance $maxpkts_
}

Source/FTP instproc stop {} {
	$self instvar agent_
	$agent_ advance 0
}

Source/FTP instproc produce { pktcnt } {
	$self instvar agent_ 
	$agent_ advance $pktcnt
}

Source/FTP instproc producemore { pktcnt } {
	$self instvar agent_ 
	$agent_ advanceby $pktcnt
}

Source/FTP instproc attach o {
	$self instvar agent_
	set agent_ $o
}

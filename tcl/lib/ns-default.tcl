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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-default.tcl,v 1.2 1997/01/24 17:53:02 mccanne Exp $


#
#
# Set up all the default paramters.  Each default parameter
# is stored in the OTcl class template and copied from the
# class into the instance when the object is created
# (this happens in the Tcl/tcl-object.tcl helper library)
#

trace set src 0
trace set dst 0

agent set cls 0
agent set addr 0
agent set dst 0
agent set seqno 0

agent/tcp set maxburst 0
agent/tcp set maxcwnd 0
agent/tcp set window 20
agent/tcp set window-init 1
agent/tcp set window-option 1
agent/tcp set window-constant 4
agent/tcp set window-thresh 0.002
agent/tcp set overhead 0
agent/tcp set ecn 0
agent/tcp set packet-size 1000
agent/tcp set bug-fix true
agent/tcp set tcp-tick 0.1

agent/tcp set dupacks 0
agent/tcp set ack 0
agent/tcp set cwnd 0
agent/tcp set awnd 0
agent/tcp set ssthresh 0
agent/tcp set rtt 0
agent/tcp set srtt 0
agent/tcp set rttvar 0
agent/tcp set backoff 0

queue set limit 10

queue/red set bytes false
queue/red set thresh 5
queue/red set maxthresh 15
queue/red set mean_pktsize 500
queue/red set q_weight 0.002
queue/red set wait true
queue/red set linterm 50
queue/red set setbit false
queue/red set drop-tail false
queue/red set doubleq false
queue/red set dqthresh 50
queue/red set subclasses 1

# These parameters below are only used for RED queues with RED subclasses.
queue/red set thresh1 5
queue/red set maxthresh1 15
queue/red set mean_pktsize1 500

#XXX other kinds of sinks -> should reparent
agent/tcp-sink set packet-size 40

#set ns_delsink(interval) 100ms
#set ns_sacksink(max-sack-blocks) 3

agent/cbr set interval_ 3.75ms
agent/cbr set random 0
agent/cbr set packet-size 210

agent/message set packet-size 180

delay/link set bandwidth 1.5Mb
delay/link set delay 100ms

#XXX
#set ns_lossy_uniform(loss_prob) 0.00

classifier/addr set shift 12
classifier/addr set mask 0xffffffff


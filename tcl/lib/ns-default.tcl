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
# @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcl/lib/ns-default.tcl,v 1.3 1997/01/26 23:26:32 mccanne Exp $


#
#
# Set up all the default paramters.  Each default parameter
# is stored in the OTcl class template and copied from the
# class into the instance when the object is created
# (this happens in the Tcl/tcl-object.tcl helper library)
#

Trace set src 0
Trace set dst 0

Agent set cls 0
Agent set addr 0
Agent set dst 0
Agent set seqno 0

Agent/TCP set maxburst 0
Agent/TCP set maxcwnd 0
Agent/TCP set window 20
Agent/TCP set window-init 1
Agent/TCP set window-option 1
Agent/TCP set window-constant 4
Agent/TCP set window-thresh 0.002
Agent/TCP set overhead 0
Agent/TCP set ecn 0
Agent/TCP set packet-size 1000
Agent/TCP set bug-fix true
Agent/TCP set tcp-tick 0.1

Agent/TCP set dupacks 0
Agent/TCP set ack 0
Agent/TCP set cwnd 0
Agent/TCP set awnd 0
Agent/TCP set ssthresh 0
Agent/TCP set rtt 0
Agent/TCP set srtt 0
Agent/TCP set rttvar 0
Agent/TCP set backoff 0

Queue set limit 10

Queue/RED set bytes false
Queue/RED set thresh 5
Queue/RED set maxthresh 15
Queue/RED set mean_pktsize 500
Queue/RED set q_weight 0.002
Queue/RED set wait true
Queue/RED set linterm 50
Queue/RED set setbit false
Queue/RED set drop-tail false
Queue/RED set doubleq false
Queue/RED set dqthresh 50
Queue/RED set subclasses 1

# These parameters below are only used for RED queues with RED subclasses.
Queue/RED set thresh1 5
Queue/RED set maxthresh1 15
Queue/RED set mean_pktsize1 500

#XXX other kinds of sinks -> should reparent
Agent/TCPSink set packet-size 40

#set ns_delsink(interval) 100ms
#set ns_sacksink(max-sack-blocks) 3

Agent/CBR set interval_ 3.75ms
Agent/CBR set random 0
Agent/CBR set packet-size 210

Agent/Message set packet-size 180

Delay/Link set bandwidth 1.5Mb
Delay/Link set delay 100ms

#XXX
#set ns_lossy_uniform(loss_prob) 0.00

Classifier/Addr set shift 12
Classifier/Addr set mask 0xffffffff

